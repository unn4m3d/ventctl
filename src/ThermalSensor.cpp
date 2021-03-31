#include <ThermalSensor.hpp>
#include <PT1000.hpp>

ventctl::ThermalSensor::ThermalSensor(const char* name, PinName pin) :
    m_input(pin),
    Peripheral<float>(name)
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
    return read_raw() * 3.3;
}

float ventctl::ThermalSensor::read_resistance()
{
    return ventctl::pt1000_resistance(read_voltage());
}

float ventctl::ThermalSensor::read_temperature()
{
    return ventctl::pt1000_temp(read_resistance());
}

void ventctl::ThermalSensor::print(ventctl::file_t file)
{
    Peripheral<float>::print(file);
    fprintf(file, "= %1.2f C (%1.2f, %1.2f V, %1.2f Ohm)", read_temperature(), read_raw(), read_voltage(), read_resistance());
}
