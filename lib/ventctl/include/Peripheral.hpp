#pragma once

#include <etl/vector.h>
#include <cstdio>
#include <loophole.hpp>


#ifndef VC_PERIPH_CAP
    #define VC_PERIPH_CAP 64
#endif

namespace ventctl
{
    using file_t = FILE*;
    using type_id_t = int;

    
    class PeripheralBase
    {
    public: 
        PeripheralBase(const char* name):
            m_name(name)
        {
            register_peripheral(this);
        }

        virtual void print(file_t file, bool short_info = false);

        virtual bool accepts_type_id(int type_id) = 0;
        
        template<typename T>
        bool accepts_type()
        {
            return accepts_type_id(util::type_id_v<T>);
        }

        template<typename T>
        bool set_value(T* value)
        {
            return accepts_type<T>() && set_value(static_cast<void*>(value), util::type_id_v<T>);
        }

        template<typename T>
        bool get_value(T* value)
        {
            return accepts_type<T>() && get_value(static_cast<void*>(value), util::type_id_v<T>);
        }

        virtual bool set_value(void* ptr, type_id_t value) = 0;

        virtual bool get_value(void* ptr, type_id_t value) = 0;

        static bool register_peripheral(PeripheralBase* p)
        {
            if(m_peripherals.full()) return false;
            m_peripherals.push_back(p);
            return true;
        }

        static etl::ivector<PeripheralBase*>& get_peripherals()
        {
            return m_peripherals;
        }

        ~PeripheralBase()
        {
            etl::erase(m_peripherals, this);
        }

        virtual void initialize() {}
        virtual void update() {}

    private:
        const char* m_name;

        static etl::vector<PeripheralBase*, VC_PERIPH_CAP> m_peripherals;

        
    };

    template<typename T>
    class Peripheral : public PeripheralBase
    {
    public:
        Peripheral(const char* name) :
            PeripheralBase(name)
        {}

        virtual bool accepts_type_id(type_id_t type_id);

        virtual bool set_value(void*, type_id_t);
        virtual bool get_value(void*, type_id_t);

        virtual bool accept_value(T& t) = 0;
        virtual T read_value() = 0;

        operator T()
        {
            return read_value();
        }

        Peripheral<T>& operator=(T& t)
        {
            accept_value(t);
            return *this;
        }

        Peripheral<T>& operator=(T t)
        {
            accept_value(t);
            return *this;
        }
    };

    extern template class Peripheral<float>;
    extern template class Peripheral<bool>;
    extern template class Peripheral<int>;
}