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
        static bool read(Socket& s, Payload<Type>& payload, FixedHeader& fhdr, VariableHeader<Type>& vhdr)
        {
            return true;
        }

        bool write(Socket& s)
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

        static bool read(Socket& s, Payload<MessageType::CONNECT>& payload, FixedHeader& fhdr, VariableHeader<MessageType::CONNECT>& vhdr);

        bool write(Socket& s);

        size_t length()
        {
            size_t len = 0;
            len += client_id.length() + 2;
            #if MQTT_VERSION >= 5
            if(will_properties.properties.size() > 0 || !will_topic.empty() || !will_payload.empty())
            {
                len += will_properties.get_length() + will_topic.length() + will_payload.length() + 4;
            }
            #else
            if(!will_topic.empty() || !will_payload.empty())
            {
                len += will_topic.length() + will_payload.length() + 4;
            }
            #endif

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

        static bool read(Socket& s, Payload<MessageType::PUBLISH>& payload, FixedHeader& fhdr, VariableHeader<MessageType::PUBLISH>& vhdr);

        bool write(Socket& s);

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

    template<>
    struct VariableHeader<MessageType::SUBSCRIBE>
    {
        uint16_t packet_id;
        Properties<MQTT_MAX_SUBSCRIBE_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + properties.get_length();
        }
    };

    template<>
    struct Payload<MessageType::SUBSCRIBE>
    {

        using subscription_type = std::pair<etl::string<MQTT_MAX_TOPIC_NAME_LENGTH>, uint8_t>;

        etl::vector<subscription_type, MQTT_MAX_SUBSCRIBE_TOPIC_COUNT> subscriptions;

        static bool read(Socket& s, Payload<MessageType::SUBSCRIBE>& payload, FixedHeader& fhdr, VariableHeader<MessageType::SUBSCRIBE>& vhdr);

        bool write(Socket& s);

        size_t length()
        {
            size_t l = 0;

            for(subscription_type& sub : subscriptions)
            {
                l += sizeof(subscription_type::second_type) + sub.first.length() + 2;
            } 

            return l;
        }
    };

    template<>
    struct VariableHeader<MessageType::SUBACK>
    {
        uint16_t packet_id;
        Properties<MQTT_MAX_SUBSCRIBE_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + properties.get_length();
        }
    };

    enum class SubAckReasonCode
    {
        QOS_0 = 0x00,
        QOS_1 = 0x01,
        QOS_2 = 0x02,
        UNSPECIFIED = 0x80,
        IMPL_SPECIFIC_ERROR = 0x83,
        NOT_AUTHORIZED = 0x87,
        TOPIC_FILTER_INVALID = 0x8F,
        PACKET_ID_IN_USE = 0x91,
        QUOTA_EXCEEDED = 0x97,
        SHARED_SUBS_NOT_SUPPORTED = 0xA1,
        WILDCARD_SUBS_NOT_SUPPORTED = 0xA2
    };

    template<>
    struct Payload<MessageType::SUBACK>
    {
        etl::vector<SubAckReasonCode, MQTT_MAX_SUBSCRIBE_TOPIC_COUNT> reasons;

        size_t length()
        {
            return reasons.size();
        }

        static bool read(Socket& s, Payload<MessageType::SUBACK>& payload, FixedHeader& fhdr, VariableHeader<MessageType::SUBACK>& vhdr);

        bool write(Socket& s);
    };

    template<>
    struct VariableHeader<MessageType::UNSUBSCRIBE>
    {
        uint16_t packet_id;
        Properties<MQTT_MAX_SUBSCRIBE_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + properties.get_length();
        }
    };

    template<>
    struct Payload<MessageType::UNSUBSCRIBE>
    {
        etl::vector<etl::string<MQTT_MAX_TOPIC_NAME_LENGTH>, MQTT_MAX_SUBSCRIBE_TOPIC_COUNT> topics;

        size_t length()
        {
            size_t len = 0;
            for(auto& str : topics)
            {
                len += str.length() + 2;
            }

            return len;
        }

        static bool read(Socket& s, Payload<MessageType::UNSUBSCRIBE>& payload, FixedHeader& fhdr, VariableHeader<MessageType::UNSUBSCRIBE>& vhdr);

        bool write(Socket& s);
    };

    template<>
    struct VariableHeader<MessageType::UNSUBACK>
    {
        uint16_t packet_id;
        Properties<MQTT_MAX_SUBSCRIBE_PROPERTY_COUNT> properties;

        size_t length()
        {
            return sizeof(packet_id) + properties.get_length();
        }
    };

    enum class UnSubAckReasonCode
    {
        SUCCESS = 0x00,
        NO_SUB_EXISTED = 0x11,
        UNSPECIFIED = 0x80,
        IMPL_SPECIFIC_ERROR = 0x83,
        NOT_AUTHORIZED = 0x87,
        TOPIC_FILTER_INVALID = 0x8F,
        PACKET_ID_IN_USE = 0x91
    };

    template<>
    struct Payload<MessageType::UNSUBACK>
    {
        etl::vector<UnSubAckReasonCode, MQTT_MAX_SUBSCRIBE_TOPIC_COUNT> reasons;

        size_t length()
        {
            return reasons.size();
        }

        static bool read(Socket& s, Payload<MessageType::UNSUBACK>& payload, FixedHeader& fhdr, VariableHeader<MessageType::UNSUBACK>& vhdr);

        bool write(Socket& s);
    };

    enum class DisconnectReasonCode
    {
        NORMAL = 0x00,
        WITH_WILL = 0x04,
        UNSPECIFIED = 0x80,
        MALFORMED_PACKET = 0x81,
        PROTOCOL_ERROR = 0x82,
        IMPL_SPECIFIC_ERROR = 0x83,
        NOT_AUTHORIZED = 0x87,
        SERVER_BUSY = 0x89,
        SERVER_SHUTTING_DOWN = 0x8B,
        KEEP_ALIVE_TIMEOUT = 0x8D,
        SESSION_TAKEN_OVER = 0x8E,
        TOPIC_FILTER_INVALID = 0x8F,
        TOPIC_NAME_INVALID = 0x90,
        RECEIVE_MAXIMUM_EXCEEDED = 0x93,
        TOPIC_ALIAS_INVALID = 0x94,
        PACKET_TOO_LARGE = 0x95,
        MESSAGE_RATE_TOO_HIGH = 0x96,
        QUOTA_EXCEEDED = 0x97,
        ADMINISTRATIVE_ACTION = 0x98,
        PAYLOAD_FORMAT_INVALID = 0x99,
        RETAIN_NOT_SUPPORTED = 0x9A,
        QOS_NOT_SUPPORTED = 0x9B,
        USE_ANOTHER_SERVER = 0x9C,
        SERVER_MOVED = 0x9D,
        SHARED_SUBS_NOT_SUPPORTED = 0x9E,
        CONNECTION_RATE_EXCEEDED = 0x9F,
        MAX_CONNECT_TIME = 0xA0,
        SUB_IDS_NOT_SUPPORTED = 0xA1,
        WILDCARD_SUBS_NOT_SUPPORTED = 0xA2
    };

    template<>
    struct VariableHeader<MessageType::DISCONNECT>
    {
        DisconnectReasonCode reason;
        Properties<MQTT_MAX_DISCONNECT_PROPERTY_COUNT> properties;

        size_t length()
        {
            return properties.get_length() + 1;
        }
    };
    
    enum class AuthReasonCode
    {
        SUCCESS = 0,
        CONTINUE = 0x18,
        REAUTHENTICATE = 0x19
    };


    template<>
    struct VariableHeader<MessageType::AUTH>
    {
        AuthReasonCode reason;
        Properties<MQTT_MAX_AUTH_PROPERTY_COUNT> properties;

        size_t length()
        {
            return properties.get_length() + 1;
        }
    };
}