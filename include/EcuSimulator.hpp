#pragma once

#include "ICanBus.hpp"

class EcuSimulator
{
public:
    EcuSimulator(ICanBus& bus, uint32_t commandId, uint8_t commandCode, const CanFrame& responseFrame);

    CanBusStatus processOnce(std::chrono::milliseconds timeout);

private:
    ICanBus& bus_;
    uint32_t commandId_;
    uint8_t commandCode_;
    CanFrame responseFrame_;
};