#pragma once
#include <Peripheral.hpp>

namespace ventctl
{

    template<typename T>
    class VariablePrinter
    {
    protected:
        void print(file_t file, T)
        {
            fprintf(file, "= <unknown>");
        }
    };

    template<>
    class VariablePrinter<int>
    {
    protected:
        void print(file_t file, int value)
        {
            fprintf(file, "= %d", value);
        }
    };

    template<>
    class VariablePrinter<float>
    {
    protected:
        void print(file_t file, float value)
        {
            fprintf(file, "= %1.2f", value);
        }
    };

    template<>
    class VariablePrinter<bool>
    {
    protected:
        void print(file_t file, bool value)
        {
            fprintf(file, "= %d", (int)value);
        }
    };

    template<typename T>
    class VariablePrinter<T*>
    {
    protected:
        void print(file_t file, T value)
        {
            fprintf(file, "= (%s*) %p", typeid(value).name(), value);
        }
    };

    template<typename T>
    class Variable : public Peripheral<T>, public VariablePrinter<T>
    {
    public:
        Variable(const char* name, T initial) :
            Peripheral<T>(name),
            m_value(initial)
        {     
        }

        virtual bool accept_value(T& t)
        {
            m_value = t;

            return true;
        }

        virtual T read_value()
        {
            return m_value;
        }

        virtual void print(file_t file, bool s = false)
        {
            Peripheral<T>::print(file, s);
            VariablePrinter<T>::print(file, m_value);
        }

        Variable<T>& operator=(T t)
        {
            accept_value(t);
            return *this;
        }
    private:
        T m_value;

    };
}