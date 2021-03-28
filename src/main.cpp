
#if !defined(VC_F407VG)
    using AnalogOut = void;
#endif

#include <Term.hpp>
#include <etl/vector.h>
#include <cmath>

#if defined(VC_F407VG)
    DigitalOut o1(PE_8), o2(PE_9), o3(PD_0), o4(PD_1);
    AnalogIn input(PC_2);
    Serial pc(PA_9, PA_10); 
    AnalogOut a1(PA_4), a2(PA_5);
#else
    DigitalOut o1(PA_1), o2(PA_2), o3(PA_3), o4(PA_4);
    AnalogIn input(PA_0);
    Serial pc(PB_6, PA_10, 9600);
#endif

etl::vector<AnalogOut*, 2> a_outputs(0);
etl::vector<DigitalOut*, 4> outputs(0);
ventctl::Term term(pc, outputs, a_outputs, &input);
DigitalOut tact(LED2);

FileHandle *mbed::mbed_override_console(int fd)
{
    return &pc;
}

void error_handler(const etl::exception& e)
{
    printf("Exc %s at %s:%d\n", e.what(), e.file_name(), e.line_number());
}

int main()
{
    etl::error_handler::set_callback<error_handler>();
    outputs.push_back(&o1);
    outputs.push_back(&o2);
    outputs.push_back(&o3);
    outputs.push_back(&o4);
    a_outputs.push_back(&a1);
    a_outputs.push_back(&a2);

    printf("Man dude\n");

    unsigned cnt = 0;

    while(1)
    {
        term.tryCommand();
        if(!(cnt % 256)) tact = !tact;

        a1.write(std::sin(us_ticker_read() / 1000000.0) / 2 + 0.5);
    }
}
