#include <AIn.hpp>


ventctl::AIn::AIn(const char* name, PinName pin) :
    Peripheral<float>(name),
    m_in(pin)
    {}

bool ventctl::AIn::accept_value(float&)
{
    return false;
}

float ventctl::AIn::read_value()
{
    return m_in.read();
}

void ventctl::AIn::print(file_t file, bool s)
{
    Peripheral<float>::print(file, s);
    if(s)
        fprintf(file, "=%1.2f", read_value());
    else
        fprintf(file, "= %1.2f (%1.2fV)", read_value(), read_value() * 10.0);
}

