#include <Peripheral.hpp>
#include <cxxabi.h>

void ventctl::PeripheralBase::print(ventctl::file_t file, bool s)
{

        int status;
        const auto& ti = typeid(*this);
        const char* name = abi::__cxa_demangle(ti.name(), 0, 0, &status);

        if(status) name = ti.name();

        if(s)
            fprintf(file, "%s", m_name);
        else
            fprintf(file, "[%s] %s", name, m_name);

}

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
