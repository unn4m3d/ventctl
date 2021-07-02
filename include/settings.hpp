#pragma once
#include <cstdint>
#include <FlashIAP.h>

namespace ventctl
{

    using std::size_t;

    #ifdef VC_SETTINGS_VERSION
        constexpr const static size_t SETTINGS_VERSION = VC_SETTINGS_VERSION;
    #else
        constexpr const static size_t SETTINGS_VERSION = 1;
    #endif
    constexpr const static size_t SETTINGS_STRIDE = 0x200;

    constexpr const static uint16_t SETTINGS_VALID_MASK = 0x8000;
    constexpr const static uint16_t SETTINGS_OVERWRITTEN_MASK = 0x4000;

    #ifdef VC_SETTINGS_START
        constexpr const static uint32_t SETTINGS_START = VC_SETTINGS_START;
    #else
        constexpr const static uint32_t SETTINGS_START = 0x080E0000;
    #endif

    #ifdef VC_SETTINGS_SIZE
        constexpr const static size_t SETTINGS_SIZE = VC_SETTINGS_SIZE;
    #else
        constexpr const static size_t SETTINGS_SIZE = 0x20000;
    #endif

    struct alignas(4) settings
    {
        uint16_t flags;
        uint16_t size;
        uint8_t ip_addr[4];
        uint8_t api_addr[40];
        uint8_t client_id[40];
        uint8_t password[40];
        float calibration[6];
    };

    extern settings application_settings;

    extern bool load_settings();
    extern bool save_settings();
    extern bool settings_loaded();
    bool is_valid_settings(uint16_t);
    bool is_overwritten(uint16_t);

    extern mbed::FlashIAP flash;

}