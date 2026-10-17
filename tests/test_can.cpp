
#include "CanBus.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace std::chrono_literals;

void testSendAndReceive()
{
    CanBus sender("vcan0");
    CanBus receiver("vcan0");

    const uint8_t payload[] = {0x11, 0x22, 0x33};

    assert(sender.sendFrame(0x123, payload, 3) == CanBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x123);
    assert(result.frame->len == 3);
    assert(result.frame->data[0] == 0x11);
    assert(result.frame->data[1] == 0x22);
    assert(result.frame->data[2] == 0x33);

    std::cout << "PASS: envio e recepcao\n";
}

void testFilter()
{
    CanBus sender("vcan0");
    CanBus receiver("vcan0");

    assert(receiver.setFilter(0x123, CAN_SFF_MASK) == CanBusStatus::SUCCESS);

    const uint8_t payload[] = {0xAA};

    // Este frame deve ser descartado pelo filtro.
    assert(sender.sendFrame(0x456, payload, 1) == CanBusStatus::SUCCESS);

    // Este frame deve ser recebido.
    assert(sender.sendFrame(0x123, payload, 1) == CanBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x123);
    assert(result.frame->data[0] == 0xAA);

    std::cout << "PASS: filtro CAN ID\n";
}

void testTimeout()
{
    CanBus receiver("vcan0");

    auto result = receiver.receiveFrame(100ms);

    assert(result.status == CanBusStatus::TIMEOUT);
    assert(!result.frame.has_value());

    std::cout << "PASS: timeout de recepcao\n";
}

void testInvalidTransmission()
{
    CanBus sender("vcan0");
    const uint8_t oversizedPayload[CAN_MAX_DLEN + 1]{};

    assert(sender.sendFrame(0x800, nullptr, 0) == CanBusStatus::INVALID_ARGUMENT);
    assert(sender.sendFrame(0x123, nullptr, 1) == CanBusStatus::INVALID_ARGUMENT);
    assert(sender.sendFrame(0x123, oversizedPayload, CAN_MAX_DLEN + 1) == CanBusStatus::INVALID_ARGUMENT);

    std::cout << "PASS: argumentos invalidos de transmissao\n";
}

void testMaximumPayload()
{
    CanBus sender("vcan0");
    CanBus receiver("vcan0");

    const uint8_t payload[CAN_MAX_DLEN] =
        {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};

    assert(sender.sendFrame(0x321, payload, CAN_MAX_DLEN) == CanBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x321);
    assert(result.frame->len == CAN_MAX_DLEN);

    for (uint8_t index = 0; index < CAN_MAX_DLEN; ++index)
    {
        assert(result.frame->data[index] == payload[index]);
    }

    std::cout << "PASS: payload maximo de 8 bytes\n";
}

void testFilterMask()
{
    CanBus sender("vcan0");
    CanBus receiver("vcan0");

    assert(receiver.setFilter(0x100, 0x700) == CanBusStatus::SUCCESS);

    const uint8_t payload[] = {0xAB};

    // O filtro aceita um frame quando:
    // (id_recebido & mascara) == (id_filtro & mascara).
    // A mascara 0x700 preserva os bits 8, 9 e 10 do ID CAN e ignora
    // os oito bits inferiores. Portanto, o filtro 0x100 com essa mascara
    // aceita IDs de 0x100 a 0x1FF.
    // Para este frame: 0x123 & 0x700 == 0x100 & 0x700 == 0x100.
    assert(sender.sendFrame(0x123, payload, 1) == CanBusStatus::SUCCESS);

    auto result = receiver.receiveFrame(1000ms);

    assert(result.status == CanBusStatus::SUCCESS);
    assert(result.frame.has_value());
    assert(result.frame->can_id == 0x123);

    std::cout << "PASS: mascara do filtro CAN ID\n";
}

void testConsecutiveMessages()
{
    CanBus sender("vcan0");
    CanBus receiver("vcan0");

    constexpr uint8_t MESSAGE_COUNT = 3;
    constexpr uint8_t PAYLOAD_SIZE = 2;
    const uint32_t ids[MESSAGE_COUNT] = {0x101, 0x202, 0x303};
    const uint8_t payloads[MESSAGE_COUNT][PAYLOAD_SIZE] = {
        {0x10, 0x11},
        {0x20, 0x21},
        {0x30, 0x31}};

    for (uint8_t index = 0; index < MESSAGE_COUNT; ++index)
    {
        assert(sender.sendFrame(ids[index], payloads[index], PAYLOAD_SIZE) == CanBusStatus::SUCCESS);
    }

    for (uint8_t index = 0; index < MESSAGE_COUNT; ++index)
    {
        auto result = receiver.receiveFrame(1000ms);

        assert(result.status == CanBusStatus::SUCCESS);
        assert(result.frame.has_value());
        assert(result.frame->can_id == ids[index]);
        assert(result.frame->len == PAYLOAD_SIZE);
        assert(result.frame->data[0] == payloads[index][0]);
        assert(result.frame->data[1] == payloads[index][1]);
    }

    std::cout << "PASS: mensagens consecutivas\n";
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

    std::cout << "Todos os testes passaram!\n";
    return 0;
}
