#include <mqtt/types.hpp>
#include <mqtt/serializer.hpp>
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
    mqtt::Message<mqtt::MessageType::CONNECT> message;

    message.fixed_header.type_and_flags = (uint8_t)mqtt::MessageType::CONNECT << 4;

    message.variable_header.proto_name.assign("MQTT");
    message.variable_header.proto_version = 5;
    message.variable_header.flags = 0b11001110;
    message.variable_header.keep_alive_timer = 16;
    message.variable_header.properties.properties.push_back(mqtt::Property{
        .type = 17,
        .value = (uint32_t)10
    });

    message.payload.client_id = "le_batya";
    message.payload.username = "sych";
    message.payload.password = "yoba";
    message.payload.will_properties.properties.push_back(mqtt::Property{
        .type = 24,
        .value = (uint32_t)10
    });
    message.payload.will_topic.assign("bugurt");
    message.payload.will_payload = "";

    message.fixed_header.length = message.variable_header.length() + message.payload.length();

    Stream s;

    message.write(s);
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

    mqtt::Message<mqtt::MessageType::CONNECT> msg;

    Stream s;
    s.write((char*)check, sizeof(check));
    s.rewind();

    TEST_ASSERT(msg.read(s));

    TEST_ASSERT_EQUAL(msg.fixed_header.type_and_flags, 0x10);
    TEST_ASSERT(msg.variable_header.proto_name == "MQTT");
    TEST_ASSERT_EQUAL(msg.variable_header.proto_version, 5);
    TEST_ASSERT_EQUAL(msg.variable_header.keep_alive_timer, 16);
    TEST_ASSERT_EQUAL(msg.variable_header.properties.properties[0].value.get<uint32_t>(), 10);
    TEST_ASSERT(msg.payload.will_topic =="bugurt");
    TEST_ASSERT(msg.payload.username == "sych");
}

void test_mqtt_qos_only_field_parsing()
{
    char check_no_qos[] = {
        0x30,
        0,
        0, 5, 't', 'o', 'p', 'i', 'c',
        0
    };

    check_no_qos[1] = sizeof(check_no_qos) - 2;

    Stream s;
    s.write(check_no_qos, sizeof(check_no_qos));
    s.rewind();

    mqtt::Message<mqtt::MessageType::PUBLISH> msg;

    TEST_ASSERT(msg.read(s));

    TEST_ASSERT_EQUAL(msg.variable_header.packet_id, 0);

    char check_qos[] = {
        0x32,
        0,
        0, 5, 't', 'o', 'p', 'i', 'c',
        0x12, 0x34,
        0
    };

    check_qos[1] = sizeof(check_qos) - 2;

    Stream s2;
    s.write(check_qos, sizeof(check_qos));

    //TEST_ASSERT(msg.read(s));
    //TEST_ASSERT_EQUAL(msg.variable_header.packet_id, 0x1234);
    //TEST_ASSERT(msg.variable_header.topic == "topic");
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_variable_int_size);
    RUN_TEST(test_mqtt_connect_generation);
    RUN_TEST(test_mqtt_connect_parsing);
    RUN_TEST(test_mqtt_qos_only_field_parsing);
    UNITY_END();
}