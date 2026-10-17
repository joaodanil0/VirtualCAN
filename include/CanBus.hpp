
#pragma once

#include <linux/can.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

class CanBus {

public:
    explicit CanBus(const std::string& interfaceName);
    ~CanBus();

    CanBus(const CanBus&) = delete;
    CanBus& operator=(const CanBus&) = delete;

    bool sendFrame(uint32_t id, const uint8_t* data, uint8_t length);
    bool setFilter(uint32_t id, uint32_t mask);
    std::optional<can_frame> receiveFrame(std::chrono::milliseconds timeout);

private:
    int socketFd_ = -1;
};
