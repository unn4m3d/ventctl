#pragma once
#include <Peripheral.hpp>
#include <type_traits>
#include <ModbusMaster.h>

namespace ventctl
{
    class MBInput : public Peripheral<bool>, public VariablePrinter<bool>
    {
    public:
        MBInputRegister(const char* name, ModbusMaster& mb, uint16_t addr) :
            Peripheral<bool>(name)
            m_mb(mb),
            m_addr(addr)
            {}

        virtual bool accept_value(T& value)
        {
            return false;
        }

        virtual void print(file_t f, bool s = false)
        {
            Peripheral<bool>::print(f, s);
            VariablePrinter<bool>::print(f, read_value())
        }

        virtual bool read_value()
        {
            auto result = m_mb.readDiscreteInputs(m_addr, 1) == ModbusMaster::ku8MBSuccess;

            if constexpr(std::is_floating_point_v<T>)
            {
                return static_cast<int16_t>(m_mb.getResponseBuffer(0)) / 32767.0;
            }
            else
            {
                return m_mb.getResponseBuffer(0);
            }
        }
    
    private:
        ModbusMaster& m_mb;
        uint16_t m_addr;
    };
}