
#if !defined(VC_F407VG)
    using AnalogOut = void;
#endif

#include <Term.hpp>
#include <etl/vector.h>
#include <cmath>
#include <Peripheral.hpp>
#include <AOut.hpp>
#include <DOut.hpp>
#include <ThermalSensor.hpp>
#include <Variable.hpp>
#include <time.hpp>
#include <EthernetInterface.h>
#include <stm32xx_emac.h>

/*    DigitalOut o1(PE_8), o2(PE_9), o3(PD_1), o4(PE_12);
    DigitalOut heater(PD_0), fan(PE_13), aux(PE_10);  
    AnalogIn input(PC_2);
    
    AnalogOut a1(PA_4), a2(PA_5);
*/

EthernetInterface net;

etl::vector<ventctl::PeripheralBase*, VC_PERIPH_CAP> ventctl::PeripheralBase::m_peripherals(0);

ventctl::DOut 
    cooler1("C_1", PD_0),
    cooler2("C_2", PE_13),
    aux("Aux", PE_10),
    heater("H", PD_1);

ventctl::AOut
    motor1("M_1", PA_4),
    motor2("M_2", PA_5);

ventctl::ThermalSensor
    temp1("T_1", PC_2);

ventctl::Variable<float>
    k_p("Kp", 0.5);

ventctl::Variable<bool>
    log_state("Log", false),
    manual_override("Manual", false);
    

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

void do_log()
{
    for(auto periph : ventctl::PeripheralBase::get_peripherals())
    {
        periph->print(stdout, true);
        printf(";");
    }
    printf("\n");
}



int main()
{
    etl::error_handler::set_callback<error_handler>();

    aux = true;
    motor1 = 0.5;
    motor2 = 0.5;

    const float temp_setting = 30.0;
    float last_time = ventctl::time();

    // Bring up the ethernet interface
    printf("Ethernet socket example\n");
    net.set_default_parameters();

    auto error = net.connect();
    printf("Error: %d\n", error);

    // Open a socket on the network interface, and create a TCP connection to mbed.org
    SocketAddress a;
    TLSSocket socket;
    socket.open(&net);

    net.gethostbyname("mqtt.googleapis.com", &a);
    a.set_port(8883);
    socket.connect(a);

    // Close the socket to return its memory and bring down the network interface
    socket.close();

    // Bring down the ethernet interface



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
                do_log();
            }
        }

        term.try_command();
    }
    net.disconnect();
}
