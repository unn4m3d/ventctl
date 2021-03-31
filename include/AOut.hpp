#pragma once

#include <Peripheral.hpp>


namespace ventctl
{
    class AOut : public Peripheral<float>
    {
    public:
        AOut(const char* name, PinName pin);

        virtual bool accept_value(float&);
        virtual void print(file_t);
        virtual float read_value();

    
    private:
        AnalogOut m_out;

    };

}