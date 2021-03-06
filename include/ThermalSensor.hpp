#pragma once
#include <Peripheral.hpp>
#include <mbed.h>

#define VC_TS_F 64

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
        
        static float read_voltage(float raw);

        float read_resistance();

        static float read_resistance(float voltage);

        float read_temperature();

        static float read_temperature(float resistance);

        virtual void print(file_t file, bool s = false);
        
        virtual void update() override;

    private:
        AnalogIn m_input;

        float m_values[VC_TS_F];
        uint8_t m_value_cnt;
    };
}