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
    class VariableLengthInteger
    {
    public:
        VariableLengthInteger(T v) : m_value(v){}
        VariableLengthInteger() : m_value(0){}

        operator T() { return m_value; }

        VariableLengthInteger<T>& operator=(T& value)
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
            VariableLengthInteger<uint32_t>> value;
    };
    
    struct Properties
    {
        VariableLengthInteger<uint32_t> length;
        etl::vector<Property, MQTT_MAX_PROPERTY_COUNT> properties;

        size_t get_length()
        {
            size_t len = length.length();

            for(auto& prop : properties)
            {
                len++;
                if(prop.value.is_type<uint8_t>())
                    len++;
                else if(prop.value.is_type<uint16_t>())
                    len += 2;
                else if(prop.value.is_type<uint32_t>())
                    len += 4;
                else if(prop.value.is_type<VariableLengthInteger<uint32_t>>())
                    len += prop.value.get<VariableLengthInteger<uint32_t>>().length();
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

        template<typename T>
        bool read(Stream&s, VariableLengthInteger<T>& value)
        {
            T inner_value = 0;
            char data, shift = 0;

            do
            {
                while(!s.readable());
                
                if(s.read(&data, 1) < 1) return false;
                inner_value |= (uint32_t)(data & 0x7F) << shift;
                shift += 7;

            } while ( (data & 0x80) && (shift < sizeof(T)) );

            value = inner_value;
            return true;
        }

        template<typename T, typename ... Ts>
        bool read_variant(Stream& s, etl::variant<Ts...>& v)
        {
            T t = 0;
            if(!read(s, t)) return false;
            
            v = t;
            return true;
        }

        bool read(Stream& s, Properties& prop)
        {
            if(!read(s, prop.length)) return false;

            prop.properties.clear();

            uint32_t byte_count = 0;

            for(size_t i = 0; byte_count < prop.length && i < MQTT_MAX_PROPERTY_COUNT; i++)
            {
                Property p;
                if(!read(s, p.type)) return false;
                byte_count++;

                switch(p.type)
                {
                    // Byte
                    case 0x01:
                    case 0x17:
                    case 0x19:
                    case 0x24:
                    case 0x25:
                    case 0x28:
                    case 0x29:
                    case 0x2A:
                        if(!read_variant<uint8_t>(s, p.value)) return false;
                        byte_count++;
                        break;

                    // Two Byte Integer
                    case 0x13:
                    case 0x21:
                    case 0x22:
                    case 0x23:
                        if(!read_variant<uint16_t>(s, p.value)) return false;
                        byte_count += 2;
                        break;

                    // Four Byte Integer
                    case 0x02:
                    case 0x11:
                    case 0x18:
                    case 0x27:
                        if(!read_variant<uint32_t>(s, p.value)) return false;
                        
                        byte_count += 4;
                        break;

                    // Variable Length Integer
                    case 0x0B:
                        if(!read_variant<VariableLengthInteger<uint32_t>>(s, p.value)) return false;
                        byte_count += p.value.get<VariableLengthInteger<uint32_t>>().length();
                        break;

                    // UTF-8 Encoded String
                    case 0x03:
                    case 0x08:
                    case 0x12:
                    case 0x15:
                    case 0x1A:
                    case 0x1C:
                    case 0x1F:
                        if(!read_variant<Property::string_type>(s, p.value)) return false;
                        byte_count += p.value.get<Property::string_type>().length() + 2;
                        break;
                    
                    // Binary Data
                    case 0x09:
                    case 0x16:
                    {
                        uint16_t len = 0;
                        if(!read(s, len)) return false;

                        if(len > MQTT_MAX_BINARY_DATA_LENGTH) len = MQTT_MAX_BINARY_DATA_LENGTH;

                        typename Property::binary_type vec(len, 0);
                        for(uint16_t i = 0; i < len; i++)
                        {
                            uint8_t byte = 0;
                            if(!read(s, byte)) return false;
                            vec.push_back(byte);
                        }

                        p.value = vec;
                        byte_count += len + 2;
                    }
                        break;

                    // String pair
                    case 0x26:
                    {
                        StringPair sp{};
                        if(!read(s, sp.first)) return false;
                        if(!read(s, sp.second)) return false;

                        p.value = sp;
                        byte_count += sp.first.length() + sp.second.length() + 4;
                    }
                        break;

                    default:
                        p.value = typename Property::string_type("Invalid");
                }

                prop.properties.push_back(p);
            }

            return true;
        }

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

        template<typename T>
        bool write(Stream&s, VariableLengthInteger<T>& value)
        {
            T len = value;

            do
            {
                unsigned char digit = len & 0x7F;
                len >>= 7;

                if(len > 0) digit |= 0x80;

                if(s.write((char*)&digit, 1) < 1) return false;

            } while(len > 0);

            return true;
        }

        bool write(Stream& s, Properties& p)
        {
            VariableLengthInteger<uint32_t> len(p.get_length() - 1);

            if(!write(s, len)) return false;

            for(auto& prop : p.properties)
            {
                if(!write(s, prop.type)) return false;
                if(prop.value.is_type<uint8_t>())
                {
                    if(!write(s, prop.value.get<uint8_t>())) return false;
                }
                else if(prop.value.is_type<uint16_t>())
                {
                    if(!write(s, prop.value.get<uint16_t>())) return false;
                }
                else if(prop.value.is_type<uint32_t>())
                {
                    if(!write(s, prop.value.get<uint32_t>())) return false;
                }
                else if(prop.value.is_type<VariableLengthInteger<uint32_t>>())
                {
                    if(!write(s, prop.value.get<VariableLengthInteger<uint32_t>>())) return false;
                }
                else if(prop.value.is_type<Property::string_type>())
                {
                    if(!write(s, prop.value.get<Property::string_type>())) return false;
                }
                else if(prop.value.is_type<Property::binary_type>())
                {
                    auto& pv = prop.value.get<Property::binary_type>();
                    if(!write(s, (uint16_t)pv.size())) return false;
                    for(auto& ch : pv)
                    {
                        if(!write(s, ch)) return false;
                    }
                }
                else if(prop.value.is_type<StringPair>())
                {
                    auto& sp = prop.value.get<StringPair>();
                    if(!write(s, sp.first) || !write(s, sp.second)) return false;
                }
            }

            return true;
        }

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
        template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
        bool read(Stream& s, T& value)
        {
            return Serializer<T>::read(s, value);
        }

        template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
        bool write(Stream& s, T& value)
        {
            return Serializer<T>::write(s, value);
        }
    }

    struct FixedHeader
    {
        uint8_t type_and_flags;
        VariableLengthInteger<uint32_t> length;
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