#include <mqtt/serializer.hpp>
#include <ulog.hpp>

using namespace mqtt;
using namespace mqtt::detail;

bool mqtt::detail::read(Socket&s, VariableByteInteger& value, FixedHeader*)
{
    uint32_t inner_value = 0;
    char data, shift = 0;

    do
    {

        if(detail::read_raw(s, data, 1)) return false;
        inner_value |= (uint32_t)(data & 0x7F) << shift;
        shift += 7;

    } while ( (data & 0x80) && (shift < sizeof(uint32_t)) );

    value = inner_value;
    return true;
}

bool mqtt::detail::write(Socket &s, VariableByteInteger& value)
{
    uint32_t len = value;

    do
    {
        unsigned char digit = len & 0x7F;
        len >>= 7;

        if(len > 0) digit |= 0x80;

        if(s.send((char*)&digit, 1) < 1) return false;

    } while(len > 0);

    return true;
}
