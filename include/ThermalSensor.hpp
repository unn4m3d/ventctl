#pragma once
#include <Peripheral.hpp>

namespace ventctl
{
    class ThermalSensor : public Peripheral<float>
    {
    public:
        ThermalSensor(const char* name, PinName pin);

        virtual bool accept_value(float&) { return false; } // This thing doesn't accept values to output

        // SUDDENLY, returns Celsius
        virtual float read_value();

        float read_raw();
        float read_voltage();
        float read_resistance();
        float read_temperature();

        virtual void print(file_t file);

    private:
        AnalogIn m_input;
    };
}