#pragma once
#include <cxxabi.h>

namespace util
{
    using time_function = float();

    extern time_function* time;

    template<typename T>
    inline const char* type_name(T& t)
    {
        int status;
        const auto& ti = typeid(t);
        const char* name = abi::__cxa_demangle(ti.name(), 0, 0, &status);

        if(status) name = ti.name();

        return name;
    }

    template<typename T>
    inline const char* type_name()
    {
        int status;
        const auto& ti = typeid(T);
        const char* name = abi::__cxa_demangle(ti.name(), 0, 0, &status);

        if(status) name = ti.name();

        return name;
    }

}