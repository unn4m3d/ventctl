#pragma once

#include <mbed.h>
#include <Peripheral.hpp>

namespace ventctl
{
    class PWMOut : public Peripheral<float>
    {
    public:
        PWMOut(const char* name, PinName pin);

        virtual bool accept_value(float&);
        virtual void print(file_t, bool sh = false);
        virtual float read_value();

        PWMOut& operator=(float b)
        {
            accept_value(b);
            return *this;
        }

    
    private:
        PwmOut m_out;

    };

}