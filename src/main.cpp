
#if !defined(VC_F407VG)
    using AnalogOut = void;
#endif

#include <Term.hpp>
#include <etl/vector.h>
#include <cmath>
#include <Peripheral.hpp>
#include <AOut.hpp>
#include <AIn.hpp>
#include <DOut.hpp>
#include <ThermalSensor.hpp>
#include <Variable.hpp>
#include <time.hpp>
#include <EthernetInterface.h>
#include <PID.hpp>
#include <Unit.hpp>
#include <Saturation.hpp>


/*
    // Test board rev. 0

    Digital Outputs (* - PWM capable)
    PE9  *
    PE10 * (N)
    PE11 *
    PE12 * (N)
    PE13 * 
    PE14 *
    PE15
    PB10
    PD0
    PD1

    Digital Inputs
    PE2
    PE3
    PE4

    Thermal Inputs (PT1000)
    PC2
    PC3
    PA0
    PA3
    PA6

    Analog Inputs (0 - 10V)
    PB0
    PB1

    Analog Outputs (0 - 10V)
    PA4
    PA5
*/

EthernetInterface net;

etl::vector<ventctl::PeripheralBase*, VC_PERIPH_CAP> ventctl::PeripheralBase::m_peripherals(0);

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


ventctl::DOut 
    cooler1("C_1", PD_0);

etl::vector<ventctl::DOut, 6> heater = {
    ventctl::DOut("H_0", PE_8),
    ventctl::DOut("H_1", PE_9),
    ventctl::DOut("H_2", PE_10),
    ventctl::DOut("H_3", PE_11),
    ventctl::DOut("H_4", PE_12),
    ventctl::DOut("H_5", PE_13)
};

ventctl::AOut
    motor1("M_1", PA_4),
    motor2("M_2", PA_5);

ventctl::AIn
    sensor1("P_1", PB_0),
    sensor2("P_2", PB_1);

ventctl::ThermalSensor
    temp_room("T_Room", PC_2),
    temp_iflow("T_IFlow", PC_3),
    temp_coolant("T_C", PA_0),
    temp_oflow("T_OFlow", PA_3);

ventctl::Variable<float>
    k_p("Kp", 0.5),
    temp_setting("S_Temp", 25.0),
    inflow_setting("S_IFlow", 3.0),
    outflow_setting("S_OFlow", 3.0);

ventctl::Variable<bool>
    log_state("Log", false),
    manual_override("Manual", false);
    

Serial pc(PA_9, PA_10); 
ventctl::Term term(pc);

ventctl::PIDController<float, float>
    pid_room_temp(2, 0.5, 0.5, 0, 50, 1),
    pid_intake_temp(0.1, 0.0001, 0.1, -1, 1, 1),
    pid_correction(0.1, 0.01, 0.01, 0, 3, 0),
    pid_intake_flow(0.1, 0.5, 0.5, 0, 1, 1),
    pid_deicer(1,1,0,0,2,1),
    pid_exhaust_flow(0.1, 1, 0, 0, 1, 1);

ventctl::Source
    src_room_temp(temp_room),
    src_iflow_temp(temp_iflow),
    src_oflow_temp(temp_oflow),
    src_coolant_temp(temp_coolant),
    src_iflow_sensor(sensor1),
    src_oflow_sensor(sensor2),
    /* Variables */
    src_temp_setting(temp_setting),
    src_iflow_setting(inflow_setting),
    src_oflow_setting(outflow_setting);

ventctl::Sum
    temp_error({{src_temp_setting, false}, {src_room_temp, true}});

ventctl::Unit<ventctl::PIDController<float, float>>
    room_temp_ctl(pid_room_temp, temp_error);

ventctl::Sum
    iflow_temp_setting({{src_temp_setting, false}, {room_temp_ctl, false}});

ventctl::Saturation
    iflow_temp_lim(0, 60);

ventctl::Unit<ventctl::Saturation>
    iflow_temp_limit(iflow_temp_lim, iflow_temp_setting);

ventctl::Sum
    iflow_temp_error({{iflow_temp_limit, false}, {src_iflow_temp, true}});

ventctl::Unit<ventctl::PIDController<float,float>>
    iflow_temp_ctl(pid_intake_temp, iflow_temp_error);

ventctl::Saturation
    heater_power_lim(0, 1),
    cooler_power_lim(0, 1);

ventctl::Gain
    cooler_power_flip(-1, iflow_temp_ctl);

ventctl::Unit<ventctl::Saturation>
    heater_power_limit(heater_power_lim, iflow_temp_ctl),
    cooler_power_limit(cooler_power_lim, cooler_power_flip);

etl::vector<ventctl::PeriphRef<bool>, 1> cooler = {cooler1};
etl::vector<ventctl::PeriphRef<bool>, 6> heater_ref = {
    heater[0],
    heater[1],
    heater[2],
    heater[3],
    heater[4],
    heater[5]
};

ventctl::SteppedOutputSink
    heater_power_sink(heater_power_limit, heater_ref, false),
    cooler_power_sink(cooler_power_limit, cooler, false);



FileHandle *mbed::mbed_override_console(int fd)
{
    return &pc;
}


int main()
{
    etl::error_handler::set_callback<error_handler>();

    motor1 = 0.5;
    motor2 = 0.5;




    while(1)
    {
        if(!manual_override)
        {
            heater_power_sink.update(ventctl::time());
            cooler_power_sink.update(ventctl::time());
        }
    }
}
