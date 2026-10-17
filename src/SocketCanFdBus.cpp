#include "SocketCanFdBus.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

SocketCanFdBus::SocketCanFdBus(const std::string& interfaceName)
{
    socketFd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socketFd_ < 0)
    {
        throw std::runtime_error("Fail to create CAN FD socket");
    }

    const int enableCanFd = 1;
    if (setsockopt(socketFd_, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enableCanFd, sizeof(enableCanFd)) < 0)
    {
        close(socketFd_);
        socketFd_ = -1;
        throw std::runtime_error("Fail to enable CAN FD frames");
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);

    if (ioctl(socketFd_, SIOCGIFINDEX, &ifr) < 0)
    {
        close(socketFd_);
        socketFd_ = -1;
        throw std::runtime_error("CAN FD interface not found");
    }

    sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socketFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        close(socketFd_);
        socketFd_ = -1;
        throw std::runtime_error("Fail to associate socket to CAN FD interface");
    }
}

SocketCanFdBus::~SocketCanFdBus()
{
    if (socketFd_ >= 0)
    {
        close(socketFd_);
    }
}

CanFdBusStatus SocketCanFdBus::sendFrame(uint32_t id, const uint8_t* data, uint8_t length, uint8_t flags)
{
    if (id > CAN_EFF_MASK || length > CAN_FD_MAX_DATA_LENGTH || (length > 0 && data == nullptr))
    {
        return CanFdBusStatus::INVALID_ARGUMENT;
    }

    canfd_frame frame{};
    frame.can_id = id;
    if (id > CAN_SFF_MASK)
    {
        frame.can_id |= CAN_EFF_FLAG;
    }
    frame.len = length;
    frame.flags = flags;

    if (length > 0)
    {
        std::copy(data, data + length, frame.data);
    }

    const auto written = write(socketFd_, &frame, CANFD_MTU);

    if (written == CANFD_MTU)
    {
        return CanFdBusStatus::SUCCESS;
    }

    return CanFdBusStatus::TRANSMISSION_ERROR;
}

CanFdBusReceiveResult SocketCanFdBus::receiveFrame(std::chrono::milliseconds timeout)
{
    pollfd pfd{};
    pfd.fd = socketFd_;
    pfd.events = POLLIN;

    const int ready = poll(&pfd, 1, static_cast<int>(timeout.count()));

    if (ready == 0)
    {
        return {{}, CanFdBusStatus::TIMEOUT};
    }

    if (ready < 0)
    {
        return {{}, CanFdBusStatus::WAIT_ERROR};
    }

    if (!(pfd.revents & POLLIN))
    {
        return {{}, CanFdBusStatus::READ_ERROR};
    }

    canfd_frame frame{};
    const auto received = read(socketFd_, &frame, sizeof(frame));

    if (received != CANFD_MTU && received != CAN_MTU)
    {
        return {{}, CanFdBusStatus::READ_ERROR};
    }

    CanFdFrame receivedFrame{};
    receivedFrame.can_id = frame.can_id & CAN_EFF_MASK;
    receivedFrame.len = frame.len;
    receivedFrame.flags = received == CANFD_MTU ? frame.flags : 0;
    std::copy(frame.data, frame.data + frame.len, receivedFrame.data.begin());

    return {receivedFrame, CanFdBusStatus::SUCCESS};
}

CanFdBusStatus SocketCanFdBus::setFilter(uint32_t id, uint32_t mask)
{
    can_filter filter{};
    filter.can_id = id;
    filter.can_mask = mask;
    if (id > CAN_SFF_MASK)
    {
        filter.can_id |= CAN_EFF_FLAG;
        filter.can_mask |= CAN_EFF_FLAG;
    }

    const bool result = setsockopt(socketFd_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) == 0;

    return result ? CanFdBusStatus::SUCCESS : CanFdBusStatus::FILTER_ERROR;
}