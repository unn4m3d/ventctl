#pragma once
#include <Peripheral.hpp>
#include <type_traits>
#include <ModbusMaster.h>

namespace ventctl
{
    template<typename T>
    class MBRegister : public Peripheral<T>, public VariablePrinter<T>
    {
    public:
        MBRegister(const char* name, ModbusMaster& mb, uint16_t addr) :
            Peripheral<T>(name)
            m_mb(mb),
            m_addr(addr)
            {}

        virtual bool accept_value(T& value)
        {
            int16_t ivalue = 0;
            if constexpr(std::is_floating_point_v<T>)
            {
                ivalue = value * 0x7FFF; 
            }
            else
            {
                ivalue = static_cast<int16_t>(value);
            }
            return m_mb.writeSingleRegister(m_coil, static_cast<uint16_t>(ivalue)) == ModbusMaster::ku8MBSuccess;
        }

        virtual void print(file_t f, bool s = false)
        {
            Peripheral<T>::print(f, s);
            VariablePrinter<T>::print(f, read_value())
        }

        virtual T read_value()
        {
            auto result = m_mb.readHoldingRegisters(m_addr, 1) == ModbusMaster::ku8MBSuccess;

            if constexpr(std::is_floating_point_v<T>)
            {
                return static_cast<int16_t>(m_mb.getResponseBuffer(0)) / 32767.0;
            }
            else
            {
                return m_mb.getResponseBuffer(0);
            }
        }

        MBRegister& operator=(bool b)
        {
            accept_value(b);
            return *this;
        }
    
    private:
        ModbusMaster& m_mb;
        uint16_t m_addr;
    };
}