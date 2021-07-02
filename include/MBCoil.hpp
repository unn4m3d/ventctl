#pragma once
#include <Peripheral.hpp>
#include <type_traits>
#include <ModbusMaster.h>

namespace ventctl
{
    class MBCoil : public Peripheral<bool>
    {
    public:
        MBCoil(const char* name, ModbusMaster& mb, uint16_t coil) :
            Peripheral<bool>(name)
            m_mb(mb),
            m_coil(coil)
            {}

        virtual bool accept_value(bool& value)
        {
            return m_mb.writeSingleCoil(m_coil, value) == ModbusMaster::ku8MBSuccess;
        }

        virtual void print(file_t f, bool s = false)
        {
            Peripheral<bool>::print(f, s);
            if(s)
                printf("=%d", (int)read_value());
            else
                printf(" = %d", (int)read_value());
        }

        virtual bool read_value()
        {
            auto result = m_mb.readCoils(m_coil, 1) == ModbusMaster::ku8MBSuccess;

            return  result && (m_mb.getResponseBuffer(0) != 0);
        }

        MBCoil& operator=(bool b)
        {
            accept_value(b);
            return *this;
        }
    
    private:
        ModbusMaster& m_mb;
        uint16_t m_coil;
    };
}