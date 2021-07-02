#include <settings.hpp>


namespace ventctl
{
    mbed::FlashIAP flash;

    bool is_valid_settings(uint16_t flags)
    {

        if(flags & 0x3FFF) return false; //TODO custom flags
        
        return !(flags & SETTINGS_VALID_MASK);
    }

    bool is_overwritten(uint16_t flags)
    {
        return !(flags & SETTINGS_OVERWRITTEN_MASK);
    }

    constexpr const static uint16_t settings_size = sizeof(settings);

    settings application_settings{
        .flags = 0,
        .size = settings_size,
        .ip_addr = {0},
        .api_addr = {'a','p','i','-','d','e','m','o','.','w','o','l','k','a','b','o','u','t','.','c','o','m'},
        .client_id = {'m','a','n'},
        .password = {'d','u','d','e'},
        .calibration = {-250.0}
    };

    static bool loaded = false;

    bool settings_loaded()
    {
        return loaded;
    }

    bool load_settings()
    {
        uint32_t flags_and_size;
        uint32_t addr = SETTINGS_START;

        volatile auto erased = flash.get_erase_value();

        while(addr < SETTINGS_START + SETTINGS_SIZE)
        {
            if(flash.read(&flags_and_size, addr, sizeof(flags_and_size)))
                return false;

            if(is_valid_settings(flags_and_size & 0xFFFF))
            {
                if(is_overwritten(flags_and_size & 0xFFFF))
                {
                    if((flags_and_size >> 16) == 0) return false;
                    addr += (flags_and_size >> 16);
                }
                else
                {
                    auto result = flash.read(&application_settings, addr, flags_and_size >> 16) == 0;                    
                    loaded = result;
                    return result;
                }
            }
            else
            {
                return false;
            }

        }

        return false;
    }

    bool save_settings()
    {
        uint32_t addr = SETTINGS_START;

        while(addr + sizeof(application_settings) < SETTINGS_START + SETTINGS_SIZE)
        {

            uint32_t flags_and_size;

            if(flash.read(&flags_and_size, addr, sizeof(flags_and_size)))
                return false;

            if(is_valid_settings(flags_and_size & 0xFFFF))
            {
                if((flags_and_size >> 16) == 0)
                {
                    if(flash.erase(SETTINGS_START, SETTINGS_SIZE)) return false;
                    addr = SETTINGS_START;
                    break;
                }

                if(!is_overwritten(flags_and_size & 0xFFFF) )
                {
                    uint8_t flags = 0; //valid & overwritten
                    if(flash.program(&flags, addr, 1))
                        return false;
                }

                addr += (flags_and_size >> 16);
            }
            else
            {
                addr = SETTINGS_START;
                if(flash.erase(SETTINGS_START, SETTINGS_SIZE)) return false;
                break;
            }
        }

        if(addr + sizeof(application_settings) >= SETTINGS_START + SETTINGS_SIZE)
        {
            if(flash.erase(SETTINGS_START, SETTINGS_SIZE)) return false;
            addr = SETTINGS_START;
        }

        application_settings.flags = SETTINGS_OVERWRITTEN_MASK;
        application_settings.size = sizeof(application_settings);

        return flash.program(&application_settings, addr, sizeof(application_settings)) == 0;
    }
}