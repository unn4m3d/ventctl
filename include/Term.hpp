#include "mbed.h"
#include <etl/string_view.h>
#include <utility>
#include <charconv>
#include <type_traits>
#include <etl/vector.h> // ETL
#include <PT1000.hpp>
#include <cstring>
#include <cstdlib>
#include <charconv.hpp>
#include <Peripheral.hpp>

using Serial = mbed::Serial;
namespace ventctl
{
    class Term
    {
    public:

        bool match_cmd(etl::string_view& v, char* cmd)
        {
            if(v.starts_with(cmd))
            {
                v.remove_prefix(strlen(cmd));
                return true;
            }

            return false;
            
        }

        void parse_cmd()
        {
            etl::string_view view(m_cmdbuf);
            etl::exception exc("None", __FILE__, __LINE__);

            if(match_cmd(view, "set "))
            {
                VC_TRY
                {
                    auto number = parse_arg<int>(view, &exc);
                    if(exc.what() != "None") break;
                    if(number >= PeripheralBase::get_peripherals().size())
                    {
                        VC_THROW("Incorrect index");
                    }

                    auto output = PeripheralBase::get_peripherals().at(number);

                    bool result = false;

                    if(output->accepts_type<float>())
                    {
                        auto value = parse_arg<float>(view, &exc);
                        if(exc.what() != "None") break;
                        result = output->set_value(&value);
                    }
                    else if(output->accepts_type<int>())
                    {
                        auto value = parse_arg<int>(view, &exc);
                        if(exc.what() != "None") break;
                        result = output->set_value(&value);
                    }
                    else if(output->accepts_type<bool>())
                    {
                        auto value = parse_arg<bool>(view, &exc);
                        if(exc.what() != "None") break;
                        result = output->set_value(&value);
                    }
                    else
                    {
                        printf("Warning: couldn't determine value type\n");
                    }

                    if(!result)
                        printf("Couldn't set output\n");
                }
                VC_CATCH(exc)
                {
                    printf("Exception : %s at %s:%d\n", exc.what(), exc.file_name(), exc.line_number());
                }
            }
            else if(match_cmd(view, "v"))
            {
                printf("ventctl v 0.0.1\n");
            }
            else if(match_cmd(view, "state"))
            {
                int counter = 0;
                for(auto& periph : PeripheralBase::get_peripherals())
                {
                    printf("[%2d] ", counter);
                    periph->print(stdout);
                    printf("\n");
                    counter++;
                }
            }
            else
            {
                printf("Cannot parse command\n");
            }
        }

        Term(Serial& s) :
            m_idx(0),
            m_cmdbuf{0},
            m_serial(s)
            {}

        void try_command()
        {
            if(m_serial.readable())
            {
                char ch;
                auto r = m_serial.read(&ch, 1);
                VC_ASSERT(r >= 0, ETL_ERROR_TEXT("IOE", "1O"));
                if(r == 0) return;
                
                if(ch == '\n')
                {
                    m_cmdbuf[m_idx] = 0;
                    parse_cmd();
                    m_idx = 0;

                }
                else if(ch != '\r')
                {
                    m_cmdbuf[m_idx] = ch;
                    m_idx++;
                }
                
            }
        }

    private:
        Serial& m_serial;
        char m_cmdbuf[256];
        uint8_t m_idx;
    };
}