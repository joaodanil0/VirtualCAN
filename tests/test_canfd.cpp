#include "EcuFdSimulator.hpp"
#include "SocketCanFdBus.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace std::chrono_literals;

void testSendAndReceive()
{
    SocketCanFdBus sender("vcanfd0");
    SocketCanFdBus receiver("vcanfd0");

    const uint8_t payload[] = {
        0x00, 0x11, 0x22, 0x33,
        0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB};

    assert(sender.sendFrame(0x1234567, payload, sizeof(payload)) == CanFdBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanFdBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x1234567);
    assert(result.frame->len == sizeof(payload));

    for (uint8_t index = 0; index < sizeof(payload); ++index)
    {
        assert(result.frame->data[index] == payload[index]);
    }

    std::cout << "PASS: envio e recepcao CAN FD\n";
}

void testFilter()
{
    SocketCanFdBus sender("vcanfd0");
    SocketCanFdBus receiver("vcanfd0");

    assert(receiver.setFilter(0x1234567, CAN_FD_EXTENDED_ID_MASK)
        == CanFdBusStatus::SUCCESS);

    const uint8_t payload[] = {0xAA};

    // Este frame deve ser descartado pelo filtro.
    assert(sender.sendFrame(0x1234568, payload, 1) == CanFdBusStatus::SUCCESS);

    // Este frame deve ser recebido.
    assert(sender.sendFrame(0x1234567, payload, 1) == CanFdBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanFdBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x1234567);
    assert(result.frame->data[0] == 0xAA);

    std::cout << "PASS: filtro CAN FD ID\n";
}

void testTimeout()
{
    SocketCanFdBus receiver("vcanfd0");

    auto result = receiver.receiveFrame(100ms);

    assert(result.status == CanFdBusStatus::TIMEOUT);
    assert(!result.frame.has_value());

    std::cout << "PASS: timeout de recepcao CAN FD\n";
}

void testInvalidTransmission()
{
    SocketCanFdBus sender("vcanfd0");
    const uint8_t oversizedPayload[CAN_FD_MAX_DATA_LENGTH + 1]{};

    assert(sender.sendFrame(0x20000000, nullptr, 0) == CanFdBusStatus::INVALID_ARGUMENT);
    assert(sender.sendFrame(0x1234567, nullptr, 1) == CanFdBusStatus::INVALID_ARGUMENT);
    assert(sender.sendFrame(0x1234567, oversizedPayload, CAN_FD_MAX_DATA_LENGTH + 1) == CanFdBusStatus::INVALID_ARGUMENT);

    std::cout << "PASS: argumentos invalidos de transmissao CAN FD\n";
}

void testMaximumPayload()
{
    SocketCanFdBus sender("vcanfd0");
    SocketCanFdBus receiver("vcanfd0");

    const uint8_t payload[CAN_FD_MAX_DATA_LENGTH]{};

    assert(sender.sendFrame(0x1234321, payload, CAN_FD_MAX_DATA_LENGTH) == CanFdBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanFdBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x1234321);
    assert(result.frame->len == CAN_FD_MAX_DATA_LENGTH);

    for (uint8_t index = 0; index < CAN_FD_MAX_DATA_LENGTH; ++index)
    {
        assert(result.frame->data[index] == payload[index]);
    }

    std::cout << "PASS: payload maximo CAN FD\n";
}

void testFilterMask()
{
    SocketCanFdBus sender("vcanfd0");
    SocketCanFdBus receiver("vcanfd0");

    assert(receiver.setFilter(0x12340000, 0x1FFF0000) == CanFdBusStatus::SUCCESS);

    const uint8_t payload[] = {0xAB};

    // O filtro aceita um frame quando:
    // (id_recebido & mascara) == (id_filtro & mascara).
    // A mascara 0x1FFF0000 compara os bits superiores do ID estendido e
    // ignora os bits inferiores. Portanto, 0x12340123 pertence ao grupo
    // configurado por 0x12340000.
    assert(sender.sendFrame(0x12340123, payload, 1) == CanFdBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanFdBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x12340123);

    std::cout << "PASS: mascara do filtro CAN FD ID\n";
}

void testConsecutiveMessages()
{
    SocketCanFdBus sender("vcanfd0");
    SocketCanFdBus receiver("vcanfd0");

    constexpr uint8_t MESSAGE_COUNT = 3;
    constexpr uint8_t PAYLOAD_SIZE = 2;
    const uint32_t ids[MESSAGE_COUNT] = {0x10100001, 0x10200002, 0x10300003};
    const uint8_t payloads[MESSAGE_COUNT][PAYLOAD_SIZE] = {
        {0x10, 0x11},
        {0x20, 0x21},
        {0x30, 0x31}};

    for (uint8_t index = 0; index < MESSAGE_COUNT; ++index)
    {
        assert(sender.sendFrame(
            ids[index],
            payloads[index],
            PAYLOAD_SIZE) == CanFdBusStatus::SUCCESS);
    }

    for (uint8_t index = 0; index < MESSAGE_COUNT; ++index)
    {
        auto result = receiver.receiveFrame(1000ms);

        assert(result.status == CanFdBusStatus::SUCCESS);
        assert(result.frame.has_value());
        assert(result.frame->can_id == ids[index]);
        assert(result.frame->len == PAYLOAD_SIZE);
        assert(result.frame->data[0] == payloads[index][0]);
        assert(result.frame->data[1] == payloads[index][1]);
    }

    std::cout << "PASS: mensagens consecutivas CAN FD\n";
}

void testEcuCommandResponse()
{
    SocketCanFdBus ecuBus("vcanfd0");
    SocketCanFdBus clientBus("vcanfd0");

    constexpr uint32_t COMMAND_ID = 0x10000600;
    constexpr uint8_t COMMAND_CODE = 0x01;
    constexpr uint32_t RESPONSE_ID = 0x10000650;

    CanFdFrame responseFrame{};
    responseFrame.can_id = RESPONSE_ID;
    responseFrame.len = 2;
    responseFrame.data[0] = 0xAA;
    responseFrame.data[1] = 0x55;

    ICanFdBus& ecuTransport = ecuBus;
    EcuFdSimulator ecu(
        ecuTransport,
        COMMAND_ID,
        COMMAND_CODE,
        responseFrame);

    assert(clientBus.setFilter(RESPONSE_ID, CAN_FD_EXTENDED_ID_MASK)
        == CanFdBusStatus::SUCCESS);

    const uint8_t command[] = {COMMAND_CODE, 0x10};
    assert(clientBus.sendFrame(COMMAND_ID, command, 2)
        == CanFdBusStatus::SUCCESS);

    assert(ecu.processOnce(1000ms) == CanFdBusStatus::SUCCESS);

    auto result = clientBus.receiveFrame(1000ms);

    assert(result.status == CanFdBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == RESPONSE_ID);
    assert(result.frame->len == responseFrame.len);
    assert(result.frame->data[0] == 0xAA);
    assert(result.frame->data[1] == 0x55);

    std::cout << "PASS: simulacao de comando e resposta da ECU CAN FD\n";
}

void testEcuIgnoresUnknownCommand()
{
    SocketCanFdBus ecuBus("vcanfd0");
    SocketCanFdBus clientBus("vcanfd0");

    constexpr uint32_t COMMAND_ID = 0x10000600;
    constexpr uint8_t COMMAND_CODE = 0x01;
    constexpr uint32_t RESPONSE_ID = 0x10000650;

    CanFdFrame responseFrame{};
    responseFrame.can_id = RESPONSE_ID;
    responseFrame.len = 1;
    responseFrame.data[0] = 0xFF;

    EcuFdSimulator ecu(
        ecuBus,
        COMMAND_ID,
        COMMAND_CODE,
        responseFrame);

    assert(clientBus.setFilter(RESPONSE_ID, CAN_FD_EXTENDED_ID_MASK)
        == CanFdBusStatus::SUCCESS);

    const uint8_t unknownCommand[] = {0x02};
    assert(clientBus.sendFrame(COMMAND_ID, unknownCommand, 1)
        == CanFdBusStatus::SUCCESS);

    assert(ecu.processOnce(1000ms)
        == CanFdBusStatus::COMMAND_NOT_MATCHED);

    auto result = clientBus.receiveFrame(100ms);
    assert(result.status == CanFdBusStatus::TIMEOUT);
    assert(!result.frame.has_value());

    std::cout << "PASS: comando CAN FD desconhecido ignorado\n";
}

int main()
{
    testSendAndReceive();
    testFilter();
    testTimeout();
    testInvalidTransmission();
    testMaximumPayload();
    testFilterMask();
    testConsecutiveMessages();
    testEcuCommandResponse();
    testEcuIgnoresUnknownCommand();

    std::cout << "Todos os testes CAN FD passaram!\n";
    return 0;
}
