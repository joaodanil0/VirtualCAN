#include "EcuFdSimulator.hpp"
#include "SocketCanFdBus.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

namespace
{
constexpr uint32_t TELEMETRY_REQUEST_ID = 0x10000600;
constexpr uint32_t TELEMETRY_RESPONSE_ID = 0x10000650;
constexpr uint8_t READ_TELEMETRY_COMMAND = 0x01;
constexpr uint8_t TELEMETRY_PAYLOAD_LENGTH = 4;

void writeLittleEndian(uint8_t* destination, uint16_t value)
{
    destination[0] = static_cast<uint8_t>(value & 0xFF);
    destination[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

uint16_t readLittleEndian(const uint8_t* source)
{
    return static_cast<uint16_t>(source[0]) |
           static_cast<uint16_t>(source[1] << 8);
}

std::string statusToString(CanFdBusStatus status)
{
    switch (status)
    {
    case CanFdBusStatus::SUCCESS:
        return "SUCCESS";
    case CanFdBusStatus::INVALID_ARGUMENT:
        return "INVALID_ARGUMENT";
    case CanFdBusStatus::TIMEOUT:
        return "TIMEOUT";
    case CanFdBusStatus::WAIT_ERROR:
        return "WAIT_ERROR";
    case CanFdBusStatus::READ_ERROR:
        return "READ_ERROR";
    case CanFdBusStatus::TRANSMISSION_ERROR:
        return "TRANSMISSION_ERROR";
    case CanFdBusStatus::FILTER_ERROR:
        return "FILTER_ERROR";
    case CanFdBusStatus::COMMAND_NOT_MATCHED:
        return "COMMAND_NOT_MATCHED";
    }

    return "UNKNOWN";
}

void printFrame(const std::string& owner, const CanFdFrame& frame)
{
    std::cout << owner << " frame: ID=0x"
              << std::hex << frame.can_id
              << " DLC=" << std::dec << static_cast<int>(frame.len)
              << " DATA=";

    for (uint8_t index = 0; index < frame.len; ++index)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(frame.data[index]) << ' ';
    }

    std::cout << std::dec << std::setfill(' ') << '\n';
}
}

int main()
{
    std::cout << "[START] Iniciando simulacao CAN FD\n";

    SocketCanFdBus ecuBus("vcanfd0");
    SocketCanFdBus infotainmentBus("vcanfd0");
    std::cout << "[SETUP] ECU e infotainment conectados a vcanfd0\n";

    const uint16_t speedTenthsKmh = 873;
    const uint16_t temperatureTenthsCelsius = 223;

    CanFdFrame telemetryResponse{};
    telemetryResponse.can_id = TELEMETRY_RESPONSE_ID;
    telemetryResponse.len = TELEMETRY_PAYLOAD_LENGTH;
    writeLittleEndian(telemetryResponse.data.data(), speedTenthsKmh);
    writeLittleEndian(
        telemetryResponse.data.data() + 2,
        temperatureTenthsCelsius);

    EcuFdSimulator ecu(
        ecuBus,
        TELEMETRY_REQUEST_ID,
        READ_TELEMETRY_COMMAND,
        telemetryResponse);

    std::cout << "[INFOTAINMENT] Configurando filtro para ID 0x"
              << std::hex << TELEMETRY_RESPONSE_ID << std::dec << '\n';
    if (infotainmentBus.setFilter(
            TELEMETRY_RESPONSE_ID,
            CAN_FD_EXTENDED_ID_MASK) != CanFdBusStatus::SUCCESS)
    {
        std::cerr << "[ERRO] Falha ao configurar filtro CAN FD\n";
        return 1;
    }
    std::cout << "[INFOTAINMENT] Filtro configurado: SUCCESS\n";

    const uint8_t request[] = {READ_TELEMETRY_COMMAND};
    std::cout << "[INFOTAINMENT] Enviando requisicao de telemetria\n";
    if (infotainmentBus.sendFrame(
            TELEMETRY_REQUEST_ID,
            request,
            sizeof(request)) != CanFdBusStatus::SUCCESS)
    {
        std::cerr << "[ERRO] Falha ao solicitar telemetria CAN FD\n";
        return 1;
    }
    std::cout << "[INFOTAINMENT] Requisicao enviada: ID=0x"
              << std::hex << TELEMETRY_REQUEST_ID
              << " OPCODE=0x" << static_cast<int>(READ_TELEMETRY_COMMAND)
              << std::dec << '\n';

    std::cout << "[ECU] Aguardando comando por 1 segundo\n";
    const CanFdBusStatus ecuStatus = ecu.processOnce(std::chrono::seconds(1));
    std::cout << "[ECU] Processamento: " << statusToString(ecuStatus) << '\n';
    if (ecuStatus != CanFdBusStatus::SUCCESS)
    {
        std::cerr << "[ERRO] A ECU CAN FD nao processou o comando\n";
        return 1;
    }

    std::cout << "[INFOTAINMENT] Aguardando resposta por 1 segundo\n";
    const auto result = infotainmentBus.receiveFrame(
        std::chrono::seconds(1));
    std::cout << "[INFOTAINMENT] Recepcao: "
              << statusToString(result.status) << '\n';

    if (result.status != CanFdBusStatus::SUCCESS ||
        !result.frame.has_value() ||
        result.frame->len != TELEMETRY_PAYLOAD_LENGTH)
    {
        std::cerr << "[ERRO] Falha ao receber telemetria CAN FD\n";
        return 1;
    }

    printFrame("[INFOTAINMENT] Resposta recebida", *result.frame);

    const uint16_t receivedSpeed = readLittleEndian(result.frame->data.data());
    const uint16_t receivedTemperature = readLittleEndian(
        result.frame->data.data() + 2);

    std::cout << "[DISPLAY] Velocidade: " << receivedSpeed / 10.0 << " km/h\n";
    std::cout << "[DISPLAY] Temperatura: " << receivedTemperature / 10.0 << " C\n";
    std::cout << "[END] Simulacao CAN FD concluida\n";

    return 0;
}