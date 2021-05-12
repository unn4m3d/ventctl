#pragma once

#include <type_traits>
#include <mqtt/basic_types.hpp>
#include <mqtt/types.hpp>
#include <mbed.h>
#include <type-loophole.h>
#include <luple.h>
#include <tuple>
#include <cstring>
#include <algorithm>

namespace mqtt
{
    namespace detail
    {
        template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
        bool read(Stream& s, T& value, FixedHeader* fhdr = nullptr);

        template<typename T, std::enable_if_t<std::is_integral<T>::value, int> = 0>
        bool read(Stream& s, T& value, FixedHeader* fhdr = nullptr)
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
        bool read(Stream& s, T value[N], FixedHeader* fhdr = nullptr)
        {
            while(!s.readable());
            return s.read(value, N) == N;
        }

        template<size_t N>
        bool read(Stream& s, etl::string<N>& str, FixedHeader* fhdr = nullptr)
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

        bool read(Stream&s, VariableByteInteger& value, FixedHeader* fhdr = nullptr);

        template<typename T, typename ... Ts>
        bool read_variant(Stream& s, etl::variant<Ts...>& v)
        {
            T t = 0;
            if(!read(s, t)) return false;
            
            v = t;
            return true;
        }

        template<size_t N>
        bool read(Stream& s, Properties<N>& prop, FixedHeader* fhdr = nullptr)
        {
            if(!read(s, prop.length)) return false;

            prop.properties.clear();

            uint32_t byte_count = 0;

            for(size_t i = 0; byte_count < prop.length && i < N; i++)
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
                        if(!read_variant<VariableByteInteger>(s, p.value)) return false;
                        byte_count += p.value.get<VariableByteInteger>().length();
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

                        auto actual_len = len > MQTT_MAX_BINARY_DATA_LENGTH ? MQTT_MAX_BINARY_DATA_LENGTH : len;

                        typename Property::binary_type vec(len, 0);
                        for(uint16_t i = 0; i < len; i++)
                        {
                            uint8_t byte = 0;
                            if(!read(s, byte)) return false;
                            vec.push_back(byte);
                        }

                        p.value = vec;
                        byte_count += len + 2;

                        if(actual_len < len) s.seek(len - actual_len, SEEK_CUR);
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
        
        template<typename T>
        bool read(Stream& s, QoSOnly<T>& value, FixedHeader* fhdr = nullptr)
        {
            if(fhdr == nullptr) return false;

            if(fhdr->type_and_flags & 6)
            {
                if(!read(s, value.value)) return false;
            }

            return true;
        }
        
        template<typename T1, typename T2>
        bool read(Stream& s, std::pair<T1, T2>& value, FixedHeader* fhdr = nullptr)
        {
            return read(s, value.first) && read(s, value.second);
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

        template<typename T1, typename T2>
        bool write(Stream& s, std::pair<T1, T2>& value)
        {
            return write(s, value.first) && write(s, value.second);
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

        template<size_t N>
        bool write(Stream& s, Properties<N>& p)
        {
            VariableByteInteger len(p.get_length() - 1);

            if(!write(s, len)) return false;

            for(Property& prop : p.properties)
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
                else if(prop.value.is_type<VariableByteInteger>())
                {
                    if(!write(s, prop.value.get<VariableByteInteger>())) return false;
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

        template<typename T>
        bool write(Stream& s, QoSOnly<T>& value)
        {
            if(value) return write(s, value.value);
            return true;
        }

    }

    template<typename T, std::enable_if_t<std::is_class<T>::value, int> = 0>
    struct Serializer
    {

        using type_list = luple_ns::luple_t<loophole_ns::as_type_list<T> >;

        static bool read(Stream& s, T& hdr, FixedHeader* fhdr = nullptr)
        {
            auto& luple = reinterpret_cast<type_list&>(hdr);

            bool status = true;
            luple_do(luple, [&status, &s, &fhdr](auto& value){
                status = status && detail::read(s, value, fhdr);
                std::cout << "Read " << typeid(value).name() << " : " << status << std::endl; 
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

    template<MessageType Type>
    struct Message
    {
        FixedHeader fixed_header;
        VariableHeader<Type> variable_header;
        Payload<Type> payload;

        bool read(Stream& s)
        {
            if(!Serializer<FixedHeader>::read(s, fixed_header)) return false;
            if(!Serializer<VariableHeader<Type>>::read(s, variable_header, &fixed_header)) return false;
            if(!Payload<Type>::read(s, payload, fixed_header, variable_header)) return false;
            return true;
        }

        bool write(Stream& s)
        {
            if(!Serializer<FixedHeader>::write(s, fixed_header)) return false;
            if(!Serializer<VariableHeader<Type>>::write(s, variable_header)) return false;
            if(!Serializer<Payload<Type>>::write(s, payload)) return false;
            return true;
        }
    };
}