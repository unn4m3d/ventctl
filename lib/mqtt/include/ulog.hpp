#pragma once

#include <etl/string.h>
#include <mqtt/basic_types.hpp>

#ifdef VC_USE_MBED_COMPAT
    #include <mbed_compat.h>
#else
    #include <mbed.h>
#endif

namespace ulog
{
    enum class log_level
    {
        NONE,
        DEBUG,
        INFO,
        WARNING,
        SEVERE,
        FATAL
    };

    #ifdef LOG_CUSTOM_STRING_TYPE
        using string_t = LOG_CUSTOM_STRING_TYPE
    #else
        using string_t = etl::string<64>;
    #endif

    using callback_t = Callback<void(log_level, string_t)>;

    extern void set_callback(callback_t&);

    extern void log(log_level, const string_t&);
    
    inline void debug(const string_t& s)
    {
        log(log_level::DEBUG, s);
    }

    inline void info(const string_t& s)
    {
        log(log_level::INFO, s);
    }

    inline void warn(const string_t& s)
    {
        log(log_level::WARNING, s);
    }
    inline void severe(const string_t& s)
    {
        log(log_level::SEVERE, s);
    }
    inline void fatal(const string_t& s)
    {
        log(log_level::FATAL, s);
    }

    template<typename T, std::enable_if_t<!std::is_enum<T>::value, int> = 0>
    inline void add_impl(string_t& str, const T& value)
    {
        str += value;
    }

    template<typename T>
    inline void add_impl(string_t& str, const mqtt::QoSOnly<T>& value)
    {
        add_impl(str, value.value);
    }

    template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
    inline void add_impl(string_t& str, const T& value)
    {
        str += (int)value;
    }

    template<typename T>
    inline void join_impl(string_t& str, const T& first)
    {
        add_impl(str, first);
    }

    inline void join_impl(string_t& str)
    {}

    template<typename T, typename ... Ts>
    inline void join_impl(string_t& str, const T& first, const Ts& ... parts)
    {
        add_impl(str, first);
        join_impl(str, parts...);
    }

    


    template<typename T, typename ... Ts>
    inline string_t join(const T& first, const Ts& ... parts)
    {
        auto result = string_t(first);
        join_impl(result, parts...);
        return result;
    }

    

}