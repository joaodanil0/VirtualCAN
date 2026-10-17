#pragma once

#include "ICanBus.hpp"

#include <string>

class SocketCanBus : public ICanBus
{
public:
    explicit SocketCanBus(const std::string& interfaceName);
    ~SocketCanBus() override;

    SocketCanBus(const SocketCanBus&) = delete;
    SocketCanBus& operator=(const SocketCanBus&) = delete;

    CanBusStatus sendFrame(uint32_t id, const uint8_t* data, uint8_t length) override;
    CanBusStatus sendRemoteFrame(uint32_t id, uint8_t length) override;
    CanBusStatus setFilter(uint32_t id, uint32_t mask) override;
    CanBusReceiveResult receiveFrame(std::chrono::milliseconds timeout) override;

private:
    int socketFd_ = -1;
};