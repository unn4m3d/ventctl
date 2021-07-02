#pragma once

#ifndef MQTT_V5
    // #define MQTT_V5
#endif

#ifndef MQTT_MAX_PAYLOAD_LENGTH
    #define MQTT_MAX_PAYLOAD_LENGTH 512
#endif

#ifndef MQTT_MAX_CLIENT_ID_LENGTH
    #define MQTT_MAX_CLIENT_ID_LENGTH 23
#endif

#ifndef MQTT_MAX_CONNECT_PROPERTY_COUNT
    #define MQTT_MAX_CONNECT_PROPERTY_COUNT 8
#endif

#ifndef MQTT_MAX_CONNACK_PROPERTY_COUNT
    #define MQTT_MAX_CONNACK_PROPERTY_COUNT 17
#endif

#ifndef MQTT_MAX_WILL_PROPERTY_COUNT
    #define MQTT_MAX_WILL_PROPERTY_COUNT 8
#endif

#ifndef MQTT_MAX_PUBLISH_PROPERTY_COUNT
    #define MQTT_MAX_PUBLISH_PROPERTY_COUNT 7
#endif

#ifndef MQTT_MAX_PUBLISH_PAYLOAD_LENGTH
    #define MQTT_MAX_PUBLISH_PAYLOAD_LENGTH 1024
#endif

#ifndef MQTT_MAX_PUBACK_PROPERTY_COUNT
    #define MQTT_MAX_PUBACK_PROPERTY_COUNT 3
#endif

#ifndef MQTT_MAX_SUBSCRIBE_PROPERTY_COUNT
    #define MQTT_MAX_SUBSCRIBE_PROPERTY_COUNT 3
#endif

#ifndef MQTT_MAX_DISCONNECT_PROPERTY_COUNT
    #define MQTT_MAX_DISCONNECT_PROPERTY_COUNT 4
#endif

#ifndef MQTT_MAX_AUTH_PROPERTY_COUNT
    #define MQTT_MAX_AUTH_PROPERTY_COUNT 4
#endif

#ifndef MQTT_MAX_SUBSCRIBE_TOPIC_COUNT
    #define MQTT_MAX_SUBSCRIBE_TOPIC_COUNT 3
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

#ifndef MQTT_TIMEOUT
    #define MQTT_TIMEOUT 3.0
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

#ifndef MQTT_MAX_TOPIC_NAME_LENGTH
    #define MQTT_MAX_TOPIC_NAME_LENGTH 16
#endif

#ifdef MQTT_USE_VC_TIME
    namespace ventctl
    {
        extern float time();
    }

    namespace mqtt
    {
        inline float time()
        {
            return ventctl::time();
        }
    }
#else
    namespace mqtt { extern float time(); }
#endif

#ifndef MQTT_TIMEOUT
    #define MQTT_TIMEOUT 5.0
#endif

#ifdef MQTT_USE_PMR_ALLOCATOR

#include <memory_resource>

namespace mqtt
{
    extern std::pmr::memory_resource* memory_resource;

    class fixed_pool_resource : public std::pmr::memory_resource
    {
    public:

        fixed_pool_resource(unsigned char* buffer, size_t size) :
            m_buffer(buffer),
            m_size(size)
            {}

        virtual void* do_allocate(size_t sz, size_t alignment)
        {

        }

    private:
    };
}

#endif