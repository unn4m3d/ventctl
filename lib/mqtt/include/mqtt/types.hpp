#pragma once

#include <mqtt/basic_types.hpp>
#include <mbed.h>

namespace mqtt
{

    struct FixedHeader
    {
        uint8_t type_and_flags;
        VariableByteInteger length;
    };

    template<MessageType Type>
    struct VariableHeader
    {
        size_t length(FixedHeader* hdr = nullptr) { return 0; }
    }; 


    template<>
    struct VariableHeader<MessageType::CONNECT>
    {
        etl::string<6> proto_name;
        uint8_t proto_version;
        uint8_t flags;
        uint16_t keep_alive_timer;
        Properties<MQTT_MAX_CONNECT_PROPERTY_COUNT> properties;

        size_t length(FixedHeader* hdr = nullptr)
        {
            return 2 + proto_name.length() + 4 + properties.get_length();
        }
    };


    enum class ConnectReasonCode : uint8_t
    {
        SUCCESS = 0x00,
        UNSPECIFIED = 0x80,
        MALFORMED_PACKET = 0x81,
        PROTOCOL_ERROR = 0x82,
        IMPLEMENTATION_SPECIFIC_ERROR = 0x83,
        UNSUPPORTED_PROTOCOL_VERSION = 0x84,
        CLIENT_IDENTIFIER_INVALID = 0x85,
        BAD_LOGIN = 0x86,
        NOT_AUTHORIZED = 0x87,
        SERVER_UNAVAILABLE = 0x88,
        SERVER_BUSY = 0x89,
        BANNED = 0x8A,
        BAD_AUTH_METHOD = 0x8C,
        TOPIC_NAME_INVALID = 0x90,
        PACKET_TOO_LARGE = 0x95,
        QUOTA_EXCEEDED = 0x96,
        PAYLOAD_FORMAT_INVALID = 0x99,
        RETAIN_NOT_SUPPORTED = 0x9A,
        QOS_NOT_SUPPORTED = 0x9B,
        USE_ANOTHER_SERVER = 0x9C,
        SERVER_MOVED = 0x9D,
        CONNECTION_RATE_EXCEEDED = 0x9F 
    };

    template<>
    struct VariableHeader<MessageType::CONNACK>
    {
        uint8_t connack_flags;
        ConnectReasonCode reason_code;
        Properties<MQTT_MAX_CONNACK_PROPERTY_COUNT> properties;

        size_t length(FixedHeader* hdr = nullptr)
        {
            return sizeof(connack_flags) + sizeof(reason_code) + properties.get_length();
        }
    };

    template<>
    struct VariableHeader<MessageType::PUBLISH>
    {
        etl::string<MQTT_MAX_TOPIC_NAME_LENGTH> topic;
        QoSOnly<uint16_t> packet_id;
        Properties<MQTT_MAX_PUBLISH_PROPERTY_COUNT> properties;

        size_t length(FixedHeader* hdr = nullptr)
        {
            auto len = topic.length() + 2 + properties.get_length();
            return (hdr && (hdr->type_and_flags & 6)) ? len + 2 : len; 
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
        Properties<MQTT_MAX_WILL_PROPERTY_COUNT> will_properties;
        etl::string<MQTT_MAX_TOPIC_NAME_LENGTH> will_topic;
        etl::string<MQTT_MAX_WILL_PAYLOAD_LENGTH> will_payload;
        etl::string<MQTT_MAX_USERNAME_LENGTH> username;
        etl::string<MQTT_MAX_PASSWORD_LENGTH> password;

        static bool read(Stream& s, Payload<MessageType::CONNECT>& payload, FixedHeader& fhdr, VariableHeader<MessageType::CONNECT>& vhdr);

        bool write(Stream& s);

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

    template<>
    struct Payload<MessageType::PUBLISH>
    {
        etl::vector<uint8_t, MQTT_MAX_PUBLISH_PAYLOAD_LENGTH> payload;

        static bool read(Stream& s, Payload<MessageType::PUBLISH>& payload, FixedHeader& fhdr, VariableHeader<MessageType::PUBLISH>& vhdr);

        bool write(Stream& s);

        size_t length()
        {
            return payload.size();
        }
    };

    enum class PubAckReasonCode
    {
        SUCCESS = 0x00,
        NO_SUBSCRIBERS = 0x10,
        UNSPECIFIED = 0x80,
        IMPL_SPECIFIC_ERROR = 0x83,
        NOT_AUTHORIZED = 0x87,
        TOPIC_NAME_INVALID = 0x90,
        PACKET_ID_IN_USE = 0x91,
        QUOTA_EXCEEDED = 0x97,
        PAYLOAD_FORMAT_INVALID = 0x99
    };

    template<>
    struct VariableHeader<MessageType::PUBACK>
    {
        uint16_t packet_id;
        PubAckReasonCode code;
        Properties<MQTT_MAX_PUBACK_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + sizeof(code) + properties.get_length();
        }
    };

    using PubRecReasonCode = PubAckReasonCode;

    template<>
    struct VariableHeader<MessageType::PUBREC>
    {
        uint16_t packet_id;
        PubRecReasonCode code;
        Properties<MQTT_MAX_PUBACK_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + sizeof(code) + properties.get_length();
        }
    };

    enum class PubRelReasonCode
    {
        SUCCESS = 0,
        PACKET_ID_NOT_FOUND = 0x92
    };

    template<>
    struct VariableHeader<MessageType::PUBREL>
    {
        uint16_t packet_id;
        PubRelReasonCode code;
        Properties<MQTT_MAX_PUBACK_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + sizeof(code) + properties.get_length();
        }
    };

    using PubCompReasonCode = PubRelReasonCode;

    template<>
    struct VariableHeader<MessageType::PUBCOMP>
    {
        uint16_t packet_id;
        PubCompReasonCode code;
        Properties<MQTT_MAX_PUBACK_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + sizeof(code) + properties.get_length();
        }
    };





}