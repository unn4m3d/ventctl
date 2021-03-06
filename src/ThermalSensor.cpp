#include <ThermalSensor.hpp>
#include <PT1000.hpp>


ventctl::ThermalSensor::ThermalSensor(const char* name, PinName pin) :
    m_input(pin),
    Peripheral<float>(name),
    m_values({0}),
    m_value_cnt(0)
{}

float ventctl::ThermalSensor::read_value()
{
    return read_temperature();
}

float ventctl::ThermalSensor::read_raw()
{
    return m_input.read();
}

float ventctl::ThermalSensor::read_voltage()
{
    float result = 0.0;
    for(auto value : m_values) result += value;
    result /= VC_TS_F;
    return read_voltage(result);
}

float ventctl::ThermalSensor::read_voltage(float raw)
{
    return raw * 3.3;
}

float ventctl::ThermalSensor::read_resistance()
{
    return read_resistance(read_voltage());
}

float ventctl::ThermalSensor::read_resistance(float voltage)
{
    return ventctl::pt1000_resistance(voltage);
}

float ventctl::ThermalSensor::read_temperature()
{
    return read_temperature(read_resistance());
}

float ventctl::ThermalSensor::read_temperature(float resistance)
{
    return ventctl::pt1000_temp(resistance);
}

void ventctl::ThermalSensor::print(ventctl::file_t file, bool s)
{
    Peripheral<float>::print(file, s);
    float raw = read_raw();
    float voltage = read_voltage(raw);
    float res = read_resistance(voltage);
    float temp = read_temperature(res);
    if(s)
        fprintf(file, "=%1.1f", temp);
    else
        fprintf(file, "= %1.4f C (%1.4f, %1.4f V, %1.2f Ohm)", temp, raw, voltage, res);
}


void ventctl::ThermalSensor::update()
{
    m_values[m_value_cnt] = read_raw();
    m_value_cnt++;
    m_value_cnt %= VC_TS_F;
}