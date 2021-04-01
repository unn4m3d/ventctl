
#if !defined(VC_F407VG)
    using AnalogOut = void;
#endif

#include <Term.hpp>
#include <etl/vector.h>
#include <cmath>
#include <AOut.hpp>
#include <DOut.hpp>
#include <ThermalSensor.hpp>
#include <Variable.hpp>
#include <time.hpp>

/*    DigitalOut o1(PE_8), o2(PE_9), o3(PD_1), o4(PE_12);
    DigitalOut heater(PD_0), fan(PE_13), aux(PE_10);  
    AnalogIn input(PC_2);
    
    AnalogOut a1(PA_4), a2(PA_5);
*/

ventctl::DOut 
    cooler1("Cooler_1", PD_0),
    cooler2("Cooler_2", PE_13),
    aux("Auxiliary", PE_10),
    heater("Heater", PD_1);

ventctl::AOut
    motor1("Motor_1", PA_4),
    motor2("Motor_2", PA_5);

ventctl::ThermalSensor
    temp1("Temp_1", PC_2);

ventctl::Variable<float>
    k_p("Kp", 0.5);

ventctl::Variable<bool>
    log_state("Log", false);
    

Serial pc(PA_9, PA_10); 
ventctl::Term term(pc);



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

    printf("Man dude\n");

    aux = true;
    motor1 = 0.5;
    motor2 = 0.5;

    const float temp_setting = 30.0;
    float last_time = ventctl::time();

    while(1)
    {

        if(ventctl::time() > 10.0)
        {
            // TODO PID
            heater = ((temp_setting - temp1) * k_p) > 0.7;
            cooler1 = (bool) heater; // FIXME
        }
        else
        {
            heater = false;
            cooler1 = false;
        }

        if(ventctl::time() - last_time > 2.0)
        {
            last_time = ventctl::time();

            if(log_state)
            {
                temp1.print(stdout);
                printf("\n");
            }
        }

        term.try_command();
    }
}
