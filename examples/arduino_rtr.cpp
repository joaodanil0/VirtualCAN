#include "SocketCanBus.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include <linux/can.h>

using namespace std::chrono_literals;

namespace
{
constexpr uint32_t ENGINE_MESSAGE_ID = 0x12;
constexpr uint8_t RTR_DLC = 3;
constexpr uint8_t MESSAGE_COUNT = 5;

void printReceivedFrame(const CanFrame& frame)
{
    const uint32_t id = frame.can_id & CAN_EFF_MASK;

    if ((frame.can_id & CAN_RTR_FLAG) != 0)
    {
        std::cout << "[MONITOR] RTR recebido: ID=0x"
                  << std::hex << id
                  << " DLC=" << std::dec << static_cast<int>(frame.len)
                  << "\n";
        return;
    }

    const std::string message(
        reinterpret_cast<const char*>(frame.data.data()),
        frame.len);

    std::cout << "[MONITOR] Dados recebidos: ID=0x"
              << std::hex << id
              << " DLC=" << std::dec << static_cast<int>(frame.len)
              << " DATA=\"" << message << "\"\n";
}
}

int main()
{
    SocketCanBus arduinoBus("vcan0");
    SocketCanBus monitorBus("vcan0");

    const uint8_t engineMessage[] = {'E', 'V', '-', 'E', 'N', 'G', 'N', 'R'};

    std::cout << "[START] Simulacao Arduino com frame de dados e RTR\n";

    for (uint8_t cycle = 1; cycle <= MESSAGE_COUNT; ++cycle)
    {
        std::cout << "[ARDUINO] Ciclo " << static_cast<int>(cycle)
                  << ": enviando EV-ENGNR\n";
        if (arduinoBus.sendFrame(
                ENGINE_MESSAGE_ID,
                engineMessage,
                sizeof(engineMessage)) != CanBusStatus::SUCCESS)
        {
            std::cerr << "[ERRO] Falha ao enviar mensagem do Arduino\n";
            return 1;
        }

        std::cout << "[ARDUINO] Enviando RTR com DLC/comando "
                  << static_cast<int>(RTR_DLC) << "\n";
        if (arduinoBus.sendRemoteFrame(
                ENGINE_MESSAGE_ID,
                RTR_DLC) != CanBusStatus::SUCCESS)
        {
            std::cerr << "[ERRO] Falha ao enviar RTR\n";
            return 1;
        }

        for (uint8_t frameIndex = 0; frameIndex < 2; ++frameIndex)
        {
            const auto result = monitorBus.receiveFrame(1000ms);

            if (result.status != CanBusStatus::SUCCESS ||
                !result.frame.has_value())
            {
                std::cerr << "[ERRO] Falha ao receber frame no monitor\n";
                return 1;
            }

            printReceivedFrame(*result.frame);
        }

        if (cycle < MESSAGE_COUNT)
        {
            std::this_thread::sleep_for(1000ms);
        }
    }

    std::cout << "[END] Simulacao concluida\n";
    return 0;
}