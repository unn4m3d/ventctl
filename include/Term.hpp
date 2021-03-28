#include "mbed.h"
#include <etl/string_view.h>
#include <utility>
#include <charconv>
#include <type_traits>
#include <etl/vector.h> // ETL
#include <PT1000.hpp>
#include <cstring>
#include <cstdlib>

#define VC_ASSERT(cond, text) ETL_ASSERT(cond, etl::exception(text, __FILE__, __LINE__))

// A hack
namespace std
{
    std::from_chars_result from_chars(const char* b, const char* e, float& v)
    {
        std::from_chars_result result;

        constexpr std::size_t buf_size = 64;
        VC_ASSERT(e > b, "Invalid string");
        VC_ASSERT((e - b) < buf_size, "String is too long");
        char buffer[buf_size] = {0};
        char* end;
        std::memcpy(buffer, b, (e - b));
        v = std::strtof(buffer, &end);

        if(end == buffer) result.ec = std::errc::invalid_argument;

        if(errno == ERANGE) result.ec = std::errc::result_out_of_range;

        result.ptr = b + (end - buffer);

        return result;
    }
}

using Serial = mbed::Serial;
namespace ventctl
{
    class Term
    {
    public:

        bool matchCmd(etl::string_view& v, char* cmd)
        {
            if(v.starts_with(cmd))
            {
                v.remove_prefix(strlen(cmd));
                return true;
            }

            return false;
            
        }

        template<typename T> T parseArg(etl::string_view& v)
        {
            auto first = v.begin();
            auto last = v.end();

            T value;

            std::from_chars_result result = std::from_chars(first, last, value);

            VC_ASSERT(result.ec == std::errc(), ETL_ERROR_TEXT("Cannot parse number", "parse error"));

            v.remove_prefix(result.ptr - first);

            return value;
        }


        void parseCmd()
        {
            etl::string_view view(m_cmdbuf);

            if(matchCmd(view, "set "))
            {
                //auto pos = view.find(' ');
                //view.remove_prefix(pos);
                volatile auto idx = parseArg<uint16_t>(view);
                VC_ASSERT(idx < m_outputs.size(), ETL_ERROR_TEXT("Out of range", "1O"));
                volatile auto value = parseArg<uint16_t>(view);
                *m_outputs[idx] = value != 0;
                
            }
            #if defined(VC_F407VG)
                else if(matchCmd(view, "anal "))
                {
                    volatile auto idx = parseArg<uint16_t>(view);
                    VC_ASSERT(idx < m_analog.size(), ETL_ERROR_TEXT("Out of range", "1O"));
                    volatile auto value = parseArg<float>(view);
                    *m_analog[idx] = value;
                }
            #endif
            else if(matchCmd(view, "temp"))
            {
                auto flt = m_thermo->read();
                auto res = ventctl::pt1000_resistance(flt*3.3);
                auto temp = ventctl::pt1000_temp(res);
                //m_serial << "Raw: " << m_thermo->read_u16() << "\nFloat: " << flt << "\nVoltage: " << flt*3.3 << "\n"; 
                printf("Raw: %hu\nFloat: %1.4f\nVoltage:%1.2f\nResistance: %1.2f\nTemp: %1.2f\n",
                    m_thermo->read_u16(),
                    flt,
                    flt*3.3,
                    res,
                    temp    
                );
                //m_serial << "Resistance: " << res << "\nTemperature: " << temp << "\n"; 
            }
            else if(matchCmd(view, "v"))
            {
                printf("ventctl v 0.0.1\n");
            }
            else if(matchCmd(view, "state"))
            {
                printf("Digital Outputs:\n");
                for(int i = 0; i < m_outputs.size(); i++)
                    printf("[%d] = %d\n", i, (int)m_outputs[i]->read() );

                #if defined(VC_F407VG)
                    printf("Analog Outputs:\n");
                    for(int i = 0; i < m_analog.size(); i++)
                        printf("[%d] = %1.2f\n", i, m_analog[i]->read());
                #endif
            }
            else
            {
                printf("Cannot parse command\n");
            }
        }

        Term(Serial& s, etl::ivector<DigitalOut*>& o, etl::ivector<AnalogOut*>& ao, AnalogIn* input) :
            m_idx(0),
            m_cmdbuf{0},
            m_serial(s), 
            m_thermo(input),
            m_analog(ao),
            m_outputs(o)
            {}

        void tryCommand()
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
                    try
                    {
                        parseCmd();
                    }
                    catch(const etl::exception& e)
                    {
                        printf("Exc %s at %s:%d\n", e.what(), e.file_name(), e.line_number());
                    }
                    catch(...)
                    {
                        printf("Lol I am fucking leaving\n");
                    }
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
        etl::ivector<DigitalOut*>& m_outputs;
        AnalogIn* m_thermo;
        etl::ivector<AnalogOut*>& m_analog;
        Serial& m_serial;
        char m_cmdbuf[256];
        uint8_t m_idx;
    };
}