#include "EcuFdSimulator.hpp"

EcuFdSimulator::EcuFdSimulator(
    ICanFdBus& bus,
    uint32_t commandId,
    uint8_t commandCode,
    const CanFdFrame& responseFrame)
    : bus_(bus),
      commandId_(commandId),
      commandCode_(commandCode),
      responseFrame_(responseFrame)
{
}

CanFdBusStatus EcuFdSimulator::processOnce(
    std::chrono::milliseconds timeout)
{
    const auto receiveResult = bus_.receiveFrame(timeout);

    if (receiveResult.status != CanFdBusStatus::SUCCESS)
    {
        return receiveResult.status;
    }

    if (!receiveResult.frame.has_value())
    {
        return CanFdBusStatus::READ_ERROR;
    }

    const CanFdFrame& command = *receiveResult.frame;

    if (command.can_id != commandId_ ||
        command.len == 0 ||
        command.data[0] != commandCode_)
    {
        return CanFdBusStatus::COMMAND_NOT_MATCHED;
    }

    return bus_.sendFrame(
        responseFrame_.can_id,
        responseFrame_.data.data(),
        responseFrame_.len,
        responseFrame_.flags);
}