#include <mqtt/config.hpp>

#ifdef MQTT_USE_PMR_ALLOCATOR


namespace mqtt
{
    extern std::pmr::memory_resource* memory_resource;

}

#endif