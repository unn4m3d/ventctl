#pragma once

#include <Peripheral.hpp>
#include <mbed.h>


namespace ventctl
{
    class AOut : public Peripheral<float>
    {
    public:
        AOut(const char* name, PinName pin);

        virtual bool accept_value(float&);
        virtual void print(file_t, bool sh = false);
        virtual float read_value();

        AOut& operator=(float b)
        {
            accept_value(b);
            return *this;
        }

    
    private:
        AnalogOut m_out;

    };

}