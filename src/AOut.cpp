#include <AOut.hpp>

ventctl::AOut::AOut(const char* name, PinName pin) :
    m_out(pin),
    Peripheral<float>(name)
{}

bool ventctl::AOut::accept_value(float& value)
{
    m_out.write(value);
    return true;
}

void ventctl::AOut::print(ventctl::file_t file, bool s)
{
    Peripheral<float>::print(file, s);
    if(s)
        fprintf(file, "=%1.2f", read_value());
    else
        fprintf(file, "= %1.2f (%1.2fV)", read_value(), read_value() * 10.0);
}

float ventctl::AOut::read_value()
{
    return m_out.read();
}