#pragma once

#include <Peripheral.hpp>
#include <mbed.h>

namespace ventctl
{
    class DOut : public Peripheral<bool>
    {
    public:
        DOut(const char* name, PinName pin);

        virtual bool accept_value(bool&);
        virtual void print(file_t, bool s = false);
        virtual bool read_value();

        DOut& operator=(bool b)
        {
            accept_value(b);
            return *this;
        }
    
    private:
        DigitalOut m_out;

    };

}