#pragma once

#include "ICanFdBus.hpp"

class EcuFdSimulator
{
public:
    EcuFdSimulator(
        ICanFdBus& bus,
        uint32_t commandId,
        uint8_t commandCode,
        const CanFdFrame& responseFrame);

    CanFdBusStatus processOnce(std::chrono::milliseconds timeout);

private:
    ICanFdBus& bus_;
    uint32_t commandId_;
    uint8_t commandCode_;
    CanFdFrame responseFrame_;
};