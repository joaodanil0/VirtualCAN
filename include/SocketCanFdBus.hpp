#pragma once

#include "ICanFdBus.hpp"

#include <string>

class SocketCanFdBus : public ICanFdBus
{
public:
    explicit SocketCanFdBus(const std::string& interfaceName);
    ~SocketCanFdBus() override;

    SocketCanFdBus(const SocketCanFdBus&) = delete;
    SocketCanFdBus& operator=(const SocketCanFdBus&) = delete;

    CanFdBusStatus sendFrame(uint32_t id, const uint8_t* data, uint8_t length, uint8_t flags = 0) override;
    CanFdBusStatus setFilter(uint32_t id, uint32_t mask) override;
    CanFdBusReceiveResult receiveFrame(std::chrono::milliseconds timeout) override;

private:
    int socketFd_ = -1;
};