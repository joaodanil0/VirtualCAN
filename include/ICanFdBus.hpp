#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

constexpr uint8_t CAN_FD_MAX_DATA_LENGTH = 64;
constexpr uint8_t CAN_FD_BITRATE_SWITCH = 0x01;
constexpr uint32_t CAN_FD_STANDARD_ID_MASK = 0x7FF;
constexpr uint32_t CAN_FD_EXTENDED_ID_MASK = 0x1FFFFFFF;

enum class CanFdBusStatus
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

struct CanFdFrame
{
    uint32_t can_id = 0;
    uint8_t len = 0;
    uint8_t flags = 0;
    std::array<uint8_t, CAN_FD_MAX_DATA_LENGTH> data{};
};

struct CanFdBusReceiveResult
{
    std::optional<CanFdFrame> frame;
    CanFdBusStatus status = CanFdBusStatus::SUCCESS;
};

class ICanFdBus
{
public:
    virtual ~ICanFdBus() = default;

    ICanFdBus(const ICanFdBus&) = delete;
    ICanFdBus& operator=(const ICanFdBus&) = delete;

    virtual CanFdBusStatus sendFrame(uint32_t id, const uint8_t* data, uint8_t length, uint8_t flags = 0) = 0;
    virtual CanFdBusStatus setFilter(uint32_t id, uint32_t mask) = 0;
    virtual CanFdBusReceiveResult receiveFrame(std::chrono::milliseconds timeout) = 0;

protected:
    ICanFdBus() = default;
};