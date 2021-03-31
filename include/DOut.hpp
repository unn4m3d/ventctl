#pragma once

#include <Peripheral.hpp>


namespace ventctl
{
    class DOut : public Peripheral<bool>
    {
    public:
        DOut(const char* name, PinName pin);

        virtual bool accept_value(bool&);
        virtual void print(file_t);
        virtual bool read_value();
    
    private:
        DigitalOut m_out;

    };

}