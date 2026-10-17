#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

constexpr uint8_t CAN_CLASSIC_MAX_DATA_LENGTH = 8;
constexpr uint32_t CAN_STANDARD_ID_MASK = 0x7FF;

enum class CanBusStatus
{
    SUCCESS,
    INVALID_ARGUMENT,
    TIMEOUT,
    WAIT_ERROR,
    READ_ERROR,
    TRANSMISSION_ERROR,
    FILTER_ERROR,
    COMMAND_NOT_MATCHED
};

struct CanFrame
{
    uint32_t can_id = 0;
    uint8_t len = 0;
    std::array<uint8_t, CAN_CLASSIC_MAX_DATA_LENGTH> data{};
};

struct CanBusReceiveResult
{
    std::optional<CanFrame> frame;
    CanBusStatus status = CanBusStatus::SUCCESS;
};

class ICanBus
{
public:
    virtual ~ICanBus() = default;

    ICanBus(const ICanBus&) = delete;
    ICanBus& operator=(const ICanBus&) = delete;

    virtual CanBusStatus sendFrame(uint32_t id, const uint8_t* data, uint8_t length) = 0;
    virtual CanBusStatus setFilter(uint32_t id, uint32_t mask) = 0;
    virtual CanBusReceiveResult receiveFrame(std::chrono::milliseconds timeout) = 0;

protected:
    ICanBus() = default;
};