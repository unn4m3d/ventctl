#include <PWMOut.hpp>


ventctl::PWMOut::PWMOut(const char* name, PinName pin) :
    m_out(pin),
    Peripheral<float>(name)
{}

bool ventctl::PWMOut::accept_value(float& value)
{
    m_out.write(value);
    return true;
}

void ventctl::PWMOut::print(ventctl::file_t file, bool s)
{
    Peripheral<float>::print(file, s);
    if(s)
        fprintf(file, "=%1.2f", read_value());
    else
        fprintf(file, "= %1.2f (%1.2fV)", read_value(), read_value() * 10.0);
}

float ventctl::PWMOut::read_value()
{
    return m_out.read();
}