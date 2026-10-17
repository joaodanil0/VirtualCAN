
#include "CanBus.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace std::chrono_literals;

void testSendAndReceive()
{
    CanBus sender("vcan0");
    CanBus receiver("vcan0");

    const uint8_t payload[] = {0x11, 0x22, 0x33};

    assert(sender.sendFrame(0x123, payload, 3));

    auto frame = receiver.receiveFrame(1000ms);

    assert(frame.has_value());
    assert(frame->can_id == 0x123);
    assert(frame->len == 3);
    assert(frame->data[0] == 0x11);
    assert(frame->data[1] == 0x22);
    assert(frame->data[2] == 0x33);

    std::cout << "PASS: envio e recepcao\n";
}

void testFilter()
{
    CanBus sender("vcan0");
    CanBus receiver("vcan0");

    assert(receiver.setFilter(0x123, CAN_SFF_MASK));

    const uint8_t payload[] = {0xAA};

    // Este frame deve ser descartado pelo filtro.
    assert(sender.sendFrame(0x456, payload, 1));

    // Este frame deve ser recebido.
    assert(sender.sendFrame(0x123, payload, 1));

    auto frame = receiver.receiveFrame(1000ms);

    assert(frame.has_value());
    assert(frame->can_id == 0x123);
    assert(frame->data[0] == 0xAA);

    std::cout << "PASS: filtro CAN ID\n";
}

void testTimeout()
{
    CanBus receiver("vcan0");

    auto frame = receiver.receiveFrame(100ms);

    assert(!frame.has_value());

    std::cout << "PASS: timeout de recepcao\n";
}

int main()
{
    testSendAndReceive();
    testFilter();
    testTimeout();

    std::cout << "Todos os testes passaram!\n";
    return 0;
}
