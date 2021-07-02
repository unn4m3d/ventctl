#pragma once
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <etl/error_handler.h>

namespace ventctl
{

    template<typename T>
    T convert(const char* buf, char ** end)
    {
        *end = buf;
        return T();
    }

    template<>
    float convert<float>(const char* buf, char** end)
    {
        return std::strtof(buf, end);
    }

    template<>
    double convert<double>(const char* buf, char** end)
    {
        return std::strtod(buf, end);
    }

    template<>
    int convert<int>(const char* buf, char** end)
    {
        return std::strtol(buf, end, 10); // TODO
    }

    template<typename T>
    std::from_chars_result from_chars(const char* b, const char* e, T& v)
    {
        std::from_chars_result result;

        constexpr std::size_t buf_size = 64;
        VC_ASSERT(e > b, "Invalid string");
        VC_ASSERT((e - b) < buf_size, "String is too long");
        char buffer[buf_size] = {0};
        char* end;
        std::memcpy(buffer, b, (e - b));
        v = convert<T>(buffer, &end);

        result.ec = std::errc();

        if(end == buffer) result.ec = std::errc::invalid_argument;

        if(errno == ERANGE) result.ec = std::errc::result_out_of_range;

        result.ptr = b + (end - buffer);

        return result;
    }

    template<typename T> 
    T parse_arg(etl::string_view& v, etl::exception* exc)
    {
        auto first = v.begin();
        auto last = v.end();

        T value;

        std::from_chars_result result = ventctl::from_chars<T>(first, last, value);

        if(result.ec == std::errc::invalid_argument)
            new(exc) VC_EXC("Invalid argument");
        else if(result.ec == std::errc::result_out_of_range)
            new(exc) VC_EXC("Out of range");
        else
            v.remove_prefix(result.ptr - first);

        return value;
    }

    template<>
    bool parse_arg<bool>(etl::string_view& v, etl::exception* exc)
    {
        if(v.starts_with("true"))
        {
            v.remove_prefix(4);
            return true;
        }
        else if(v.starts_with("false"))
        {
            v.remove_prefix(5);
            return false;
        }
        else if(v.starts_with("0"))
        {
            v.remove_prefix(1);
            return false;
        }
        else
        {
            return true;
        }
    }

    template<>
    etl::string_view parse_arg<etl::string_view>(etl::string_view& v, etl::exception* exc)
    {
        auto pos = v.find_first_of(" \t\r\n");

        if(pos != etl::string_view::npos)
        {
            etl::string_view ret = v;
            if(ret.starts_with(" "))
                ret.remove_prefix(1);
            return ret;
        }
        else
            return v.substr(0, pos);
    }
}