#pragma once
#include <Peripheral.hpp>
#include <mbed.h>
#include <stm32f4xx_hal_adc.h>

namespace ventctl
{
    class HiFiThermalSensor : public Peripheral<float>
    {
    public:
        enum SamplingTime
        {
            S3_CYCLES,
            S15_CYCLES,
            S28_CYCLES,
            S56_CYCLES,
            S84_CYCLES,
            S112_CYCLES,
            S144_CYCLES,
            S480_CYCLES
        };

        HiFiThermalSensor(const char* name, uint8_t channel, SamplingTime time = S15_CYCLES);

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

        virtual void initialize();
        virtual void update();

    private:
        uint8_t m_channel;
        float m_voltage;
        SamplingTime m_samplingTime;
    };
}