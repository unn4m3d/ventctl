#pragma once
#include <cstdint>
#include <FlashIAP.h>
#include <FlashIAPBlockDevice.h>
#include <LittleFileSystem.h>

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
        constexpr const static size_t SETTINGS_SIZE = 0x80000;
    #endif

    extern FlashIAPBlockDevice block_device;
    extern LittleFileSystem fs;

    extern int initialize_fs();


}