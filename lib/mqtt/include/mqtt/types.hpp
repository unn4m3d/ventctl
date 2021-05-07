#pragma once
#include <stdint.h>
#include <mbed.h>
#include <type-loophole.h>
#include <luple.h>
#include <tuple>
#include <utility>
#include <cstring>
#include <algorithm>
#include <etl/string.h>
#include <etl/vector.h>
#include <etl/variant.h>

#ifndef MQTT_MAX_PAYLOAD_LENGTH
    #define MQTT_MAX_PAYLOAD_LENGTH 512
#endif

#ifndef MQTT_MAX_CLIENT_ID_LENGTH
    #define MQTT_MAX_CLIENT_ID_LENGTH 23
#endif

#ifndef MQTT_MAX_PROPERTY_COUNT
    #define MQTT_MAX_PROPERTY_COUNT 8
#endif

#ifndef MQTT_MAX_PROPERTY_LENGTH
    #if MQTT_MAX_CLIENT_ID_LENGTH > 32
        #define MQTT_MAX_PROPERTY_LENGTH MQTT_MAX_CLIENT_ID_LENGTH
    #else
        #define MQTT_MAX_PROPERTY_LENGTH 32
    #endif
#endif

#ifndef MQTT_MAX_BINARY_DATA_LENGTH
    #define MQTT_MAX_BINARY_DATA_LENGTH 64
#endif

#ifndef MQTT_VERSION
    #define MQTT_VERSION 5
#endif

#ifndef MQTT_MAX_WILL_TOPIC_LENGTH
    #define MQTT_MAX_WILL_TOPIC_LENGTH 16
#endif

#ifndef MQTT_MAX_WILL_PAYLOAD_LENGTH
    #define MQTT_MAX_WILL_PAYLOAD_LENGTH 16
#endif

#ifndef MQTT_MAX_USERNAME_LENGTH
    #define MQTT_MAX_USERNAME_LENGTH 16
#endif

#ifndef MQTT_MAX_PASSWORD_LENGTH
    #define MQTT_MAX_PASSWORD_LENGTH 16
#endif

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
        PUBCOMP,
        SUBSCRIBE,
        SUBACK,
        UNSUBSCRIBE,
        UNSUBACK,
        PINGREQ,
        PINGRESP,
        DISCONNECT,
        RESERVED1
    };
    
    enum class QoS
    {
        AT_MOST_ONCE,
        AT_LEAST_ONCE,
        EXACTLY_ONCE,
        RESERVED
    };

    enum class ConnectReturnCode : uint8_t
    {
        ACCEPTED,
        UNACCEPTABLE_PROTO,
        ID_REJECTED,
        SERVER_UNAVAILABLE,
        BAD_LOGIN,
        NOT_AUTHORIZED
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
    
    struct Properties
    {
        VariableByteInteger length;
        etl::vector<Property, MQTT_MAX_PROPERTY_COUNT> properties;

        size_t get_length();
    };


    namespace detail
    {
        template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
        bool read(Stream& s, T& value);

        template<typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
        bool read(Stream& s, T& value)
        {
            auto size = sizeof(T);

            char* buf = reinterpret_cast<char*>(&value);

            while(!s.readable());

            auto len = s.read(buf, size);
            
            if(len < size) return false;

            std::reverse(buf, buf + size);

            return true;
        }

        template<typename T, size_t N>
        bool read(Stream& s, T value[N])
        {
            while(!s.readable());
            return s.read(value, N) == N;
        }

        template<size_t N>
        bool read(Stream& s, etl::string<N>& str)
        {
            while(!s.readable());
            uint16_t length;
            if(!read(s, length)) return false;

            char buf[N];

            auto actual_length = length < N ? length : N;
            if(actual_length == 0) return true;

            while(!s.readable());

            auto result = s.read(buf, actual_length);

            if(actual_length < length) s.seek(length - actual_length, SEEK_CUR);

            str.assign(buf, result);

            return result == actual_length;
        }

        bool read(Stream&s, VariableByteInteger& value);

        template<typename T, typename ... Ts>
        bool read_variant(Stream& s, etl::variant<Ts...>& v)
        {
            T t = 0;
            if(!read(s, t)) return false;
            
            v = t;
            return true;
        }

        bool read(Stream& s, Properties& prop);

        template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
        bool write(Stream& s, T& value);

        template<typename T, std::enable_if_t<std::is_integral<T>::value, char> = 0>
        bool write(Stream& s, T value)
        {
            auto size = sizeof(T);

            char buf[sizeof(T)];

            std::memcpy(reinterpret_cast<void*>(buf), reinterpret_cast<void*>(&value), size);
            std::reverse(buf, buf + size);

            auto result =  s.write(buf, size) == size;

            return result;
        }

        template<typename T, size_t N>
        bool write(Stream& s, T value[N])
        {
            return s.write(value, N) == N;
        }

        template<size_t N>
        bool write(Stream& s, etl::string<N>& str)
        {
            if(!write(s, (uint16_t)str.length())) return false;

            auto result = s.write(str.data(), str.size());

            return result == str.length();
        }

        bool write(Stream&s, VariableByteInteger& value);

        bool write(Stream& s, Properties& p);

    }

    template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
    struct Serializer
    {

        using type_list = luple_ns::luple_t<loophole_ns::as_type_list<T> >;

        static bool read(Stream& s, T& hdr)
        {
            auto& luple = reinterpret_cast<type_list&>(hdr);

            bool status = true;
            luple_do(luple, [&status, &s](auto& value){
                status = status && detail::read(s, value);
            });

            return status;
        }

        static bool write(Stream& s, T& value)
        {
            static_assert(std::is_aggregate<T>::value, "T is not aggregate");
            auto& luple = reinterpret_cast<type_list&>(value);

            bool status = true;
            luple_do(luple, [&status, &s](auto& value){
                status = status && detail::write(s, value);
            });

            return status;
        }
    };

    namespace detail
    {
        template<typename T>
        std::enable_if_t<std::is_class<T>::value, bool> read(Stream& s, T& value)
        {
            return Serializer<T>::read(s, value);
        }

        template<typename T>
        std::enable_if_t<std::is_class<T>::value, bool> write(Stream& s, T& value)
        {
            return Serializer<T>::write(s, value);
        }
    }

    struct FixedHeader
    {
        uint8_t type_and_flags;
        VariableByteInteger length;
    };

    template<MessageType Type>
    struct VariableHeader
    {
        size_t length() { return 0; }
    }; 


    template<>
    struct VariableHeader<MessageType::CONNECT>
    {
        etl::string<6> proto_name;
        uint8_t proto_version;
        uint8_t flags;
        uint16_t keep_alive_timer;
        Properties properties;

        size_t length()
        {
            return 2 + proto_name.length() + 4 + properties.get_length();
        }
    };


    template<MessageType Type>
    struct Payload
    {
        static bool read(Stream& s, Payload<Type>& payload, FixedHeader& fhdr, VariableHeader<Type>& vhdr)
        {
            return true;
        }

        bool write(Stream& s)
        {
            return true;
        }

        size_t length()
        {
            return 0;
        }
    };

    template<>
    struct Payload<MessageType::CONNECT>
    {

        etl::string<MQTT_MAX_CLIENT_ID_LENGTH> client_id;
        Properties will_properties;
        etl::string<MQTT_MAX_WILL_TOPIC_LENGTH> will_topic;
        etl::string<MQTT_MAX_WILL_PAYLOAD_LENGTH> will_payload;
        etl::string<MQTT_MAX_USERNAME_LENGTH> username;
        etl::string<MQTT_MAX_PASSWORD_LENGTH> password;

        static bool read(Stream& s, Payload<MessageType::CONNECT>& payload, FixedHeader& fhdr, VariableHeader<MessageType::CONNECT>& vhdr)
        {
            if((uint32_t)fhdr.length - vhdr.length() > 0)
            {
                if(!detail::read(s, payload.client_id)) return false;

                if(vhdr.flags & 0x4) {
                    if(!detail::read(s, payload.will_properties)) return false;
                    if(!detail::read(s, payload.will_topic)) return false;
                    if(!detail::read(s, payload.will_payload)) return false;
                }

                if(vhdr.flags & 0x80)
                {
                    if(!detail::read(s, payload.username)) return false;
                }

                if(vhdr.flags & 0x40)
                {
                    if(!detail::read(s, payload.password)) return false;
                }
            }
            return true;
        }

        bool write(Stream& s)
        {
            if(!detail::write(s, client_id)) return false;

            if(will_properties.properties.size() > 0 || !will_topic.empty() || !will_payload.empty())
            {
                if(!detail::write(s, will_properties)) return false;
                if(!detail::write(s, will_topic)) return false;
                if(!detail::write(s, will_payload)) return false;
            }

            if(!username.empty())
                if(!detail::write(s, username)) return false;
            
            if(!password.empty())
                if(!detail::write(s, password)) return false;

            return true;
        }

        size_t length()
        {
            size_t len = 0;
            len += client_id.length() + 2;
            if(will_properties.properties.size() > 0 || !will_topic.empty() || !will_payload.empty())
            {
                len += will_properties.get_length() + will_topic.length() + will_payload.length() + 4;
            }

            if(!username.empty())
            {
                len += username.length() + 2;
            }

            if(!password.empty())
            {
                len += password.length() + 2;
            }

            return len;
        }
    };


}