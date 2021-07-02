#include <mqtt/Client.hpp>
#include <ulog.hpp>
#include <memory>

using namespace mqtt;

#define IMPL(x) case x : pimpl<x>::process_impl(this, hdr); break

template<MessageType Type>
static Message<Type> message;

#ifndef MQTT_DISABLE_QOS
    static etl::map<uint16_t, Message<MessageType::PUBLISH>, 16> persistence;

#endif

template<MessageType Type>
Message<Type>& read_msg(Client* c, FixedHeader& hdr)
{
    message<Type>.fixed_header = hdr;
    if(!message<Type>.read_without_fixed_header(c->get_stream()))
        ulog::warn(ulog::join("Cannot read packet of type ", Type));
    return message<Type>;
}

namespace mqtt
{

    template<MessageType Type>
    struct pimpl
    {
        static void process_impl(Client* client, FixedHeader& hd)
        {
            ulog::warn(ulog::join("No actions for Packet type : ", Type));
        }
    };

    template<>
    struct pimpl<MessageType::CONNACK>
    {
        static void process_impl(Client* c, FixedHeader& hdr)
        {
            auto& msg = read_msg<MessageType::CONNACK>(c, hdr);

            c->m_conn_status = msg.variable_header.reason_code;
        }
    };

    template<>
    struct pimpl<MessageType::PUBLISH>
    {
        static void process_impl(Client* c, FixedHeader& hdr)
        {
            auto& msg = read_msg<MessageType::PUBLISH>(c, hdr);

            auto result = c->m_rx_cb(msg);

            switch ((hdr.type_and_flags >> 1) & 3)
            {
            case 1:
                message<MessageType::PUBACK>.fixed_header.type_and_flags = (uint8_t)MessageType::PUBACK << 4;
                message<MessageType::PUBACK>.variable_header.packet_id = msg.variable_header.packet_id;
                message<MessageType::PUBACK>.variable_header.code = result ? PubAckReasonCode::SUCCESS : PubAckReasonCode::IMPL_SPECIFIC_ERROR;
                #ifdef MQTT_V5
                message<MessageType::PUBACK>.variable_header.properties.properties.clear();
                #endif
                if(!c->send(message<MessageType::PUBACK>))
                    ulog::warn(ulog::join("Cannot send PUBACK for packet #", msg.variable_header.packet_id));
                break;

            case 2:
                ulog::warn("QoS 2 is not supported");
                break;
            }
        }
    };
    #ifndef MQTT_DISABLE_QOS
    template<>
    struct pimpl<MessageType::PUBACK>
    {
        static void process_impl(Client* c, FixedHeader& hdr)
        {

            auto& msg = read_msg<MessageType::PUBACK>(c, hdr);

            auto pid = msg.variable_header.packet_id;

            if(msg.variable_header.code != PubAckReasonCode::SUCCESS)
                ulog::warn(ulog::join("Cannot deliver packet #", pid, " (", msg.variable_header.code, ")"));

            persistence.erase(persistence.find(pid));
        }
    };
    #endif
}

void Client::process()
{
    FixedHeader hdr;
    
    if(!Serializer<FixedHeader>::read(get_stream(), hdr))
    {
        ulog::warn("Cannot read MQTT packet");
        return;
    }

    auto msg_type = (MessageType)(hdr.type_and_flags >> 4);

    switch(msg_type)
    {
        IMPL(MessageType::RESERVED);
        IMPL(MessageType::CONNECT);
        IMPL(MessageType::CONNACK);
        IMPL(MessageType::PUBLISH);
        IMPL(MessageType::PUBACK);
        IMPL(MessageType::PUBREC);
        IMPL(MessageType::PUBREL);
        IMPL(MessageType::PUBCOMP);
        IMPL(MessageType::SUBSCRIBE);
        IMPL(MessageType::SUBACK);
        IMPL(MessageType::UNSUBSCRIBE);
        IMPL(MessageType::UNSUBACK);
        IMPL(MessageType::PINGREQ);
        IMPL(MessageType::PINGRESP);
        IMPL(MessageType::DISCONNECT);
        IMPL(MessageType::AUTH);
    }
}

bool Client::connect_async(const Payload<MessageType::CONNECT>& payload)
{
    // TODO Customization

    auto& msg = message<MessageType::CONNECT>;
    if(std::addressof(msg.payload) != std::addressof(payload))
        msg.payload = payload;
    msg.fixed_header.type_and_flags = (uint8_t)MessageType::CONNECT << 4;
    msg.variable_header.proto_name = "MQTT";
    msg.variable_header.proto_version = 5;
    
    uint8_t flags = 1 << 1;
    if(!payload.username.empty()) flags |= 1 << 7;
    if(!payload.password.empty()) flags |= 1 << 6;
    if(!payload.will_topic.empty()) flags |= 1 << 2;

    msg.variable_header.flags = flags;
    msg.variable_header.keep_alive_timer = 100;

    #ifdef MQTT_V5
    msg.variable_header.properties.properties.clear();
    #endif

    if(!send(msg))
    {
        ulog::warn("Cannot send connect message");
        return false;
    }

    return true;
}

bool Client::connect_async(const char* username, const char* password)
{
    auto& msg = message<MessageType::CONNECT>;
    msg.payload.username = username;
    msg.payload.password = password;
    #ifdef MQTT_V5
    msg.payload.will_properties.properties.clear();
    #endif
    msg.payload.will_topic.clear();

    return connect_async(msg.payload);
}

bool Client::publish(const etl::istring& topic, const etl::istring& data, uint8_t qos, bool dup)
{
    if(qos > 1)
    {
        ulog::warn("QoS > 1 is not supported");
        qos = 1;
    }

    auto& msg = message<MessageType::PUBLISH>;

    uint8_t flags = (uint8_t)MessageType::PUBLISH << 4;

    flags |= qos << 1;
    if(dup)
        flags |= 1 << 3;

    msg.fixed_header.type_and_flags = flags;
    msg.variable_header.topic = topic;

    if(qos)
        msg.variable_header.packet_id.value = ++m_pid_counter;
    else
        msg.variable_header.packet_id.value = 0;

    #ifdef MQTT_V5
    msg.variable_header.properties.properties.clear();
    #endif
    msg.payload.payload.assign((const uint8_t*)data.cbegin(), (const uint8_t*)data.cend());

    if(qos)
        persistence.insert({m_pid_counter, msg});

    return send(msg);
}