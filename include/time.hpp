#pragma once
#include <mbed.h>

namespace ventctl
{
    uint64_t micros()
    {
        return us_ticker_read();
    }

    uint64_t millis()
    {
        return us_ticker_read() / 1000;
    }

    float time()
    {
        return us_ticker_read() / 1000000.0;
    }
}