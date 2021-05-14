#pragma once
#include <mbed.h>
#include <Peripheral.hpp>

namespace ventctl
{
    class AIn : public Peripheral<float>
    {
    public:
        AIn(const char* name, PinName pin);

        virtual bool accept_value(float&);
        virtual void print(file_t, bool sh = false);
        virtual float read_value();

        AIn& operator=(float b)
        {
            accept_value(b);
            return *this;
        }

    private:
        AnalogIn m_in;
    };

}