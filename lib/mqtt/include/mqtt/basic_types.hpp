#pragma once

#include <stdint.h>
#include <utility>
#if defined(CAPACITY)
    #undef CAPACITY
#endif
#include <etl/string.h>
#include <etl/vector.h>
#include <etl/variant.h>
#include <mqtt/config.hpp>

namespace mqtt
{
    enum class MessageType
    {
        RESERVED = 0,
        CONNECT,
        CONNACK,
        PUBLISH,
        PUBACK,
        PUBREC,
        PUBREL,
        PUBCOMP,
        SUBSCRIBE,
        SUBACK,
        UNSUBSCRIBE,
        UNSUBACK,
        PINGREQ,
        PINGRESP,
        DISCONNECT,
        AUTH
    };
    
    enum class QoS
    {
        AT_MOST_ONCE,
        AT_LEAST_ONCE,
        EXACTLY_ONCE,
        RESERVED
    };

    template<typename T = uint32_t>
    class VariableLengthIntegerImpl
    {
    public:
        VariableLengthIntegerImpl(T v) : m_value(v){}
        VariableLengthIntegerImpl() : m_value(0){}

        operator T() { return m_value; }

        VariableLengthIntegerImpl<T>& operator=(T& value)
        {
            m_value = value;
            return *this;
        }

        uint8_t length()
        {
            if(m_value < 128)
                return 1;
            else if(m_value < 16384)
                return 2;
            else if(m_value < 2097152)
                return 3;
            else if(m_value < 268435456)
                return 4;
            else
                return 4;
        }

    private:
        T m_value;
    };

    using VariableByteInteger = VariableLengthIntegerImpl<uint32_t>;

    using StringPair = std::pair<
        etl::string<MQTT_MAX_CLIENT_ID_LENGTH>,
        etl::string<MQTT_MAX_CLIENT_ID_LENGTH>
        >;

    struct Property 
    {
        using string_type = etl::string<MQTT_MAX_PROPERTY_LENGTH>;
        using binary_type = etl::vector<uint8_t, MQTT_MAX_BINARY_DATA_LENGTH>; 

        uint8_t type;
        etl::variant<
            uint8_t, 
            uint16_t,
            uint32_t, 
            string_type, 
            StringPair, 
            binary_type,
            VariableByteInteger> value;
    };

    template<typename T>
    struct QoSOnly
    {
        operator T()
        {
            return value;
        }

        T value;
    };
    
    template<size_t N>
    struct Properties
    {
        #ifdef MQTT_V5
        VariableByteInteger length;
        etl::vector<Property, N> properties;

        size_t get_length()
        {
            size_t len = length.length();

            for(Property& prop : properties)
            {
                len++;
                if(prop.value.is_type<uint8_t>())
                    len++;
                else if(prop.value.is_type<uint16_t>())
                    len += 2;
                else if(prop.value.is_type<uint32_t>())
                    len += 4;
                else if(prop.value.is_type<VariableByteInteger>())
                    len += prop.value.get<VariableByteInteger>().length();
                else if(prop.value.is_type<Property::string_type>())
                    len += prop.value.get<Property::string_type>().length() + 2;
                else if(prop.value.is_type<Property::binary_type>())
                    len += prop.value.get<Property::binary_type>().size() + 2;
                else if(prop.value.is_type<StringPair>())
                {
                    auto& sp = prop.value.get<StringPair>();
                    len += sp.first.length() + sp.second.length() + 4;
                }
            }
            
            return len;
        }
        #else
        size_t get_length() { return 0; }
        #endif
    };
}