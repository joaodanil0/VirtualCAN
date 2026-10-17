
#include "CanBus.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cerrno>

#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

CanBus::CanBus(const std::string &interfaceName)
{
    socketFd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (socketFd_ < 0)
    {
        throw std::runtime_error("Fail to create CAN socket");
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);

    if (ioctl(socketFd_, SIOCGIFINDEX, &ifr) < 0)
    {
        close(socketFd_);
        socketFd_ = -1;
        throw std::runtime_error("CAN Interface not found");
    }

    sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(
            socketFd_,
            reinterpret_cast<sockaddr *>(&addr),
            sizeof(addr)) < 0)
    {
        close(socketFd_);
        socketFd_ = -1;
        throw std::runtime_error("Fail to associate socket to an CAN interface");
    }
}

CanBus::~CanBus()
{
    if (socketFd_ >= 0)
    {
        close(socketFd_);
    }
}

bool CanBus::sendFrame(uint32_t id, const uint8_t *data, uint8_t length)
{
    // Este exemplo aceita somente CAN classico com ID padrao.
    if (id > CAN_SFF_MASK || length > CAN_MAX_DLEN || (length > 0 && data == nullptr))
    {
        return false;
    }

    can_frame frame{};
    frame.can_id = id;
    frame.len = length;

    if (length > 0)
    {
        std::copy(data, data + length, frame.data);
    }

    const auto written = write(socketFd_, &frame, sizeof(frame));

    return written == sizeof(frame);
}

std::optional<can_frame> CanBus::receiveFrame(std::chrono::milliseconds timeout)
{
    pollfd pfd{};
    pfd.fd = socketFd_;
    pfd.events = POLLIN;

    const int ready = poll(&pfd, 1, static_cast<int>(timeout.count()));

    if (ready <= 0 || !(pfd.revents & POLLIN))
    {
        return std::nullopt;
    }

    can_frame frame{};

    const auto received = read(socketFd_, &frame, sizeof(frame));

    if (received != sizeof(frame))
    {
        return std::nullopt;
    }

    return frame;
}

bool CanBus::setFilter(uint32_t id, uint32_t mask)
{
    can_filter filter{};
    filter.can_id = id;
    filter.can_mask = mask;

    return setsockopt(
               socketFd_,
               SOL_CAN_RAW,
               CAN_RAW_FILTER,
               &filter,
               sizeof(filter)) == 0;
}
