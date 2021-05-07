#include <mqtt/types.hpp>
#include <mqtt/serializer.hpp>

using namespace mqtt;

bool Payload<MessageType::CONNECT>::write(Stream& s)
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