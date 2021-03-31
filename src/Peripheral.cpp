#include <Peripheral.hpp>
#include <cxxabi.h>



ventctl::PeripheralBase::PeripheralBase(const char* name) :
    m_name(name)
{
    register_peripheral(this);
}

void ventctl::PeripheralBase::print(ventctl::file_t file)
{

        int status;
        const auto& ti = typeid(*this);
        const char* name = abi::__cxa_demangle(ti.name(), 0, 0, &status);

        if(status) name = ti.name();

        fprintf(file, "[%s] %s", name, m_name);

}

template<typename T>
ventctl::Peripheral<T>::Peripheral(const char* name) :
    PeripheralBase(name)
{}

template<typename T>
bool ventctl::Peripheral<T>::set_value(void* v , ventctl::type_id_t id)
{
    return accepts_type_id(id) && accept_value(*(static_cast<T*>(v)));
}

template<typename T>
bool ventctl::Peripheral<T>::get_value(void* v , ventctl::type_id_t id)
{
    if(accepts_type_id(id))
    {
        *(static_cast<T*>(v)) = read_value();
        return true;
    }
    return false;
}

template<typename T>
bool ventctl::Peripheral<T>::accepts_type_id(ventctl::type_id_t id)
{
    return util::type_id_v<T> == id;
}



namespace ventctl
{
    template class Peripheral<float>;
    template class Peripheral<bool>;
    template class Peripheral<int>;
}


etl::vector<ventctl::PeripheralBase*, VC_PERIPH_CAP> ventctl::PeripheralBase::m_peripherals(0);