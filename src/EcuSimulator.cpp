#include "EcuSimulator.hpp"

EcuSimulator::EcuSimulator(ICanBus& bus, uint32_t commandId, uint8_t commandCode, const CanFrame& responseFrame)
    : bus_(bus), commandId_(commandId), commandCode_(commandCode), responseFrame_(responseFrame)
{
}

CanBusStatus EcuSimulator::processOnce(std::chrono::milliseconds timeout)
{
    const auto receiveResult = bus_.receiveFrame(timeout);

    if (receiveResult.status != CanBusStatus::SUCCESS)
    {
        return receiveResult.status;
    }

    if (!receiveResult.frame.has_value())
    {
        return CanBusStatus::READ_ERROR;
    }

    const CanFrame& command = *receiveResult.frame;

    if (command.can_id != commandId_ || command.len == 0 || command.data[0] != commandCode_)
    {
        return CanBusStatus::COMMAND_NOT_MATCHED;
    }

    return bus_.sendFrame(responseFrame_.can_id, responseFrame_.data.data(), responseFrame_.len);
}