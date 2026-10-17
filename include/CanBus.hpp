
#pragma once

#include <linux/can.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

enum class CanBusStatus
{
    SUCCESS,
    INVALID_ARGUMENT,
    TIMEOUT,
    WAIT_ERROR,
    READ_ERROR,
    TRANSMISSION_ERROR,
    FILTER_ERROR
};

struct CanBusReceiveResult
{
    std::optional<can_frame> frame;
    CanBusStatus status = CanBusStatus::SUCCESS;
};

class CanBus {

public:
    explicit CanBus(const std::string& interfaceName);
    ~CanBus();

    CanBus(const CanBus&) = delete;
    CanBus& operator=(const CanBus&) = delete;

    CanBusStatus sendFrame(uint32_t id, const uint8_t *data, uint8_t length);
    CanBusStatus setFilter(uint32_t id, uint32_t mask);
    CanBusReceiveResult receiveFrame(std::chrono::milliseconds timeout);

private:
    int socketFd_ = -1;
};
