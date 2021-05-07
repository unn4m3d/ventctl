#include <mqtt/types.hpp>
#include <unity.h>


void test_mqtt_variable_int_size()
{
    using vi_t = mqtt::VariableByteInteger;

    vi_t a(0x36), b(0xFF), c(268435450);

    TEST_ASSERT_EQUAL(a.length(), 1);
    TEST_ASSERT_EQUAL(b.length(), 2);
    TEST_ASSERT_EQUAL(c.length(), 4);
}


void test_mqtt_connect_generation()
{
    mqtt::FixedHeader fhdr;
    mqtt::VariableHeader<mqtt::MessageType::CONNECT> vhdr;
    mqtt::Payload<mqtt::MessageType::CONNECT> payload;

    fhdr.type_and_flags = (uint8_t)mqtt::MessageType::CONNECT << 4;

    vhdr.proto_name.assign("MQTT");
    vhdr.proto_version = 5;
    vhdr.flags = 0b11001110;
    vhdr.keep_alive_timer = 16;
    vhdr.properties.properties.push_back(mqtt::Property{
        .type = 17,
        .value = (uint32_t)10
    });

    payload.client_id = "le_batya";
    payload.username = "sych";
    payload.password = "yoba";
    payload.will_properties.properties.push_back(mqtt::Property{
        .type = 24,
        .value = (uint32_t)10
    });
    payload.will_topic.assign("bugurt");
    payload.will_payload = "";

    fhdr.length = vhdr.length() + payload.length();

    Stream s;

    mqtt::Serializer<decltype(fhdr)>::write(s, fhdr);
    mqtt::Serializer<decltype(vhdr)>::write(s, vhdr);
    mqtt::Serializer<decltype(payload)>::write(s, payload);
    std::string str = s.get_buf();

    unsigned char check[] = {
        0x10, // CONNECT shifted to high half byte
        0, // Length 
        0, 4, 'M', 'Q', 'T', 'T', // Protocol name
        5, // Protocol version
        0b11001110, // Flags
        0, 16, // Keep alive
        5, 17, 0, 0, 0, 10, // prop type=15 value= 10u32 (5 bytes)
        0, 8, 'l', 'e', '_', 'b', 'a', 't', 'y', 'a',
        5, 24, 0, 0, 0, 10,
        0, 6, 'b', 'u', 'g', 'u', 'r', 't',
        0, 0,
        0, 4, 's', 'y', 'c', 'h',
        0, 4, 'y', 'o', 'b', 'a'
    };

    check[1] = sizeof(check) - 2;

    TEST_ASSERT_EQUAL_STRING_LEN(check, str.data(), sizeof(check));
}

void test_mqtt_connect_parsing()
{
    unsigned char check[] = {
        0x10, // CONNECT shifted to high half byte
        0, // Length 
        0, 4, 'M', 'Q', 'T', 'T', // Protocol name
        5, // Protocol version
        0b11001110, // Flags
        0, 16, // Keep alive
        5, 17, 0, 0, 0, 10, // prop type=15 value= 10u32 (5 bytes)
        0, 8, 'l', 'e', '_', 'b', 'a', 't', 'y', 'a',
        5, 24, 0, 0, 0, 10,
        0, 6, 'b', 'u', 'g', 'u', 'r', 't',
        0, 0,
        0, 4, 's', 'y', 'c', 'h',
        0, 4, 'y', 'o', 'b', 'a'
    };

    check[1] = sizeof(check) - 2;

    mqtt::FixedHeader fhdr;
    mqtt::VariableHeader<mqtt::MessageType::CONNECT> vhdr;
    mqtt::Payload<mqtt::MessageType::CONNECT> payload;

    Stream s;
    s.write((char*)check, sizeof(check));
    s.rewind();

    TEST_ASSERT(mqtt::Serializer<decltype(fhdr)>::read(s, fhdr));
    TEST_ASSERT(mqtt::Serializer<decltype(vhdr)>::read(s, vhdr));
    TEST_ASSERT(mqtt::Serializer<decltype(payload)>::read(s, payload));

    TEST_ASSERT_EQUAL(fhdr.type_and_flags, 0x10);
    TEST_ASSERT(vhdr.proto_name == "MQTT");
    TEST_ASSERT_EQUAL(vhdr.proto_version, 5);
    TEST_ASSERT_EQUAL(vhdr.keep_alive_timer, 16);
    TEST_ASSERT_EQUAL(vhdr.properties.properties[0].value.get<uint32_t>(), 10);
    TEST_ASSERT(payload.will_topic =="bugurt");
    TEST_ASSERT(payload.username == "sych");
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_variable_int_size);
    RUN_TEST(test_mqtt_connect_generation);
    RUN_TEST(test_mqtt_connect_parsing);
    UNITY_END();
}