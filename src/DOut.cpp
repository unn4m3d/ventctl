#include <DOut.hpp>

ventctl::DOut::DOut(const char* name, PinName pin) :
    m_out(pin),
    Peripheral<bool>(name)
{}

bool ventctl::DOut::accept_value(bool& value)
{
    m_out.write(value);
    return true;
}

void ventctl::DOut::print(ventctl::file_t file)
{
    Peripheral<bool>::print(file);
    fprintf(file, "= %d", m_out.read());
}

bool ventctl::DOut::read_value()
{
    return m_out;
}