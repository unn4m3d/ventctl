#include <mqtt/types.hpp>

size_t mqtt::Properties::get_length()
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

bool mqtt::detail::read(Stream&s, VariableByteInteger& value)
{
    uint32_t inner_value = 0;
    char data, shift = 0;

    do
    {
        while(!s.readable());
        
        if(s.read(&data, 1) < 1) return false;
        inner_value |= (uint32_t)(data & 0x7F) << shift;
        shift += 7;

    } while ( (data & 0x80) && (shift < sizeof(uint32_t)) );

    value = inner_value;
    return true;
}

bool mqtt::detail::read(Stream& s, Properties& prop)
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

bool mqtt::detail::write(Stream&s, VariableByteInteger& value)
{
    uint32_t len = value;

    do
    {
        unsigned char digit = len & 0x7F;
        len >>= 7;

        if(len > 0) digit |= 0x80;

        if(s.write((char*)&digit, 1) < 1) return false;

    } while(len > 0);

    return true;
}

bool mqtt::detail::write(Stream& s, Properties& p)
{
    VariableByteInteger len(p.get_length() - 1);

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