#include <mqtt/types.hpp>
#include <mqtt/serializer.hpp>

using namespace mqtt;

bool Payload<MessageType::CONNECT>::write(Stream& s)
{
    if(!detail::write(s, client_id)) return false;

    #ifdef MQTT_V5
    if(will_properties.properties.size() > 0 || !will_topic.empty() || !will_payload.empty())
    {
        if(!detail::write(s, will_properties)) return false;
    #else
    if(!will_topic.empty() || !will_payload.empty())
    {
    #endif
        if(!detail::write(s, will_topic)) return false;
        if(!detail::write(s, will_payload)) return false;
    }

    if(!username.empty())
        if(!detail::write(s, username)) return false;
    
    if(!password.empty())
        if(!detail::write(s, password)) return false;

    return true;
}

bool Payload<MessageType::CONNECT>::read(Stream& s, Payload<MessageType::CONNECT>& payload, FixedHeader& fhdr, VariableHeader<MessageType::CONNECT>& vhdr)
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

bool Payload<MessageType::PUBLISH>::write(Stream& s)
{
    return s.write((char*)payload.data(), payload.size()) == payload.size();
}

bool Payload<MessageType::PUBLISH>::read(Stream& s, Payload<MessageType::PUBLISH>& payload, FixedHeader& fhdr, VariableHeader<MessageType::PUBLISH>& vhdr)
{
    auto length = fhdr.length - vhdr.length(&fhdr);
    
    while(!s.readable());

    if(!length) return true;
    if(length > payload.payload.max_size()) length = payload.payload.max_size();

    return s.read((char*)payload.payload.data(), length) == length;
}

bool Payload<MessageType::SUBSCRIBE>::write(Stream& s)
{
    for(subscription_type& sub : subscriptions)
    {
        if(!detail::write(s, sub.first)) return false;
        if(!detail::write(s, sub.second)) return false;
    }

    return true;
}

bool Payload<MessageType::SUBSCRIBE>::read(Stream& s, Payload<MessageType::SUBSCRIBE>& payload, FixedHeader& fhdr, VariableHeader<MessageType::SUBSCRIBE>& vhdr)
{
    size_t bc = 0;
    auto length = fhdr.length - vhdr.length();

    payload.subscriptions.clear();

    while(bc < length && !payload.subscriptions.full())
    {
        subscription_type sub;
        if(!Serializer<subscription_type>::read(s, sub, &fhdr)) return false;
        payload.subscriptions.push_back(sub);
        bc += sub.first.length() + 2 + 1;
    }

    if(bc < length) s.seek(length - bc, SEEK_CUR);

    return true;
}

bool Payload<MessageType::SUBACK>::read(Stream& s, Payload<MessageType::SUBACK>& payload, FixedHeader& fhdr, VariableHeader<MessageType::SUBACK>& vhdr)
{
    auto length = fhdr.length - vhdr.length();
    payload.reasons.clear();

    for(int i = 0; i < length && !payload.reasons.full(); i++)
    {
        uint8_t reason;
        if(!detail::read(s, reason)) return false;

        payload.reasons.push_back((SubAckReasonCode) reason);
    }

    if(payload.reasons.size() < length) s.seek(length - payload.reasons.size(), SEEK_CUR); 

    return true;
}

bool Payload<MessageType::SUBACK>::write(Stream& s)
{
    for(SubAckReasonCode& r : reasons)
    {
        if(!detail::write(s, (uint8_t)r)) return false;
    }
    
    return true;
}

bool Payload<MessageType::UNSUBSCRIBE>::read(Stream& s, Payload<MessageType::UNSUBSCRIBE>& payload, FixedHeader& fhdr, VariableHeader<MessageType::UNSUBSCRIBE>& vhdr)
{
    payload.topics.clear();

    auto length = fhdr.length - vhdr.length();
    size_t bc = 0;

    while(bc < length && !payload.topics.full())
    {
        etl::string<MQTT_MAX_TOPIC_NAME_LENGTH> topic;
        if(!detail::read(s, topic)) return false;

        payload.topics.push_back(topic);
        bc += topic.length() + 2;
    }

    if(bc < length) s.seek(length - bc, SEEK_CUR);

    return true;
}



bool Payload<MessageType::UNSUBSCRIBE>::write(Stream& s)
{
    for(auto& str : topics)
    {
        if(!detail::write(s, str)) return false;
    }

    return true;
}

bool Payload<MessageType::UNSUBACK>::read(Stream& s, Payload<MessageType::UNSUBACK>& payload, FixedHeader& fhdr, VariableHeader<MessageType::UNSUBACK>& vhdr)
{
    auto length = fhdr.length - vhdr.length();
    payload.reasons.clear();

    for(int i = 0; i < length && !payload.reasons.full(); i++)
    {
        uint8_t reason;
        if(!detail::read(s, reason)) return false;

        payload.reasons.push_back((UnSubAckReasonCode) reason);
    }

    if(payload.reasons.size() < length) s.seek(length - payload.reasons.size(), SEEK_CUR); 

    return true;
}

bool Payload<MessageType::UNSUBACK>::write(Stream& s)
{
    for(UnSubAckReasonCode& r : reasons)
    {
        if(!detail::write(s, (uint8_t)r)) return false;
    }
    
    return true;
}