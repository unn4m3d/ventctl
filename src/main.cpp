
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
#include <mqtt/Client.hpp>
#include <ulog.hpp>
#include <stm32f4xx_hal_adc.h>


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

ventctl::DOut
    heater0 ("H_0", PE_8),
    heater1("H_1", PE_9),
    heater2("H_2", PE_10),
    heater3("H_3", PE_11),
    heater4("H_4", PE_12),
    heater5("H_5", PE_13);

ventctl::AOut
    motor1("M_1", PA_4),
    motor2("M_2", PA_5);

ventctl::AIn
    sensor1("P_1", PB_0),
    sensor2("P_2", PB_1);

ventctl::ThermalSensor
    temp_room("T_Room", PC_3),
    temp_iflow("T_IFlow", PC_2),
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

/*ventctl::PIDController<float, float>
    pid_room_temp(2, 0.5, 0.5, 0, 50, 1),
    pid_intake_temp(0.1, 0.0001, 0.1, -1, 1, 1),
    pid_correction(0.1, 0.01, 0.01, 0, 3, 0),
    pid_intake_flow(0.1, 0.5, 0.5, 0, 1, 1),
    pid_deicer(1,1,0,0,2,1),
    pid_exhaust_flow(0.1, 1, 0, 0, 1, 1);*/

ventctl::PIDController<float, float>
    pid_room_temp(2, 0.5, 0, 0, 50, 1),
    pid_intake_temp(0.1, 0.0001, 0, -1, 1, 1),
    pid_correction(0.1, 0.01, 0, 0, 3, 0),
    pid_intake_flow(0.1, 0.5, 0, 0, 1, 1),
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

ventctl::Unit<ventctl::PIDController<float, float>>
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
    heater0,
    heater1,
    heater2,
    heater3,
    heater4,
    heater5};

ventctl::SteppedOutputSink
    heater_power_sink(heater_power_limit, heater_ref, false),
    cooler_power_sink(cooler_power_limit, cooler, false);


FileHandle *mbed::mbed_override_console(int fd)
{
    return &pc;
}

const char* ca_cert = "-----BEGIN CERTIFICATE-----\n"
"MIIGEzCCA/ugAwIBAgIQfVtRJrR2uhHbdBYLvFMNpzANBgkqhkiG9w0BAQwFADCB\n"
"iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
"cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
"BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTgx\n"
"MTAyMDAwMDAwWhcNMzAxMjMxMjM1OTU5WjCBjzELMAkGA1UEBhMCR0IxGzAZBgNV\n"
"BAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEYMBYGA1UE\n"
"ChMPU2VjdGlnbyBMaW1pdGVkMTcwNQYDVQQDEy5TZWN0aWdvIFJTQSBEb21haW4g\n"
"VmFsaWRhdGlvbiBTZWN1cmUgU2VydmVyIENBMIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
"AQ8AMIIBCgKCAQEA1nMz1tc8INAA0hdFuNY+B6I/x0HuMjDJsGz99J/LEpgPLT+N\n"
"TQEMgg8Xf2Iu6bhIefsWg06t1zIlk7cHv7lQP6lMw0Aq6Tn/2YHKHxYyQdqAJrkj\n"
"eocgHuP/IJo8lURvh3UGkEC0MpMWCRAIIz7S3YcPb11RFGoKacVPAXJpz9OTTG0E\n"
"oKMbgn6xmrntxZ7FN3ifmgg0+1YuWMQJDgZkW7w33PGfKGioVrCSo1yfu4iYCBsk\n"
"Haswha6vsC6eep3BwEIc4gLw6uBK0u+QDrTBQBbwb4VCSmT3pDCg/r8uoydajotY\n"
"uK3DGReEY+1vVv2Dy2A0xHS+5p3b4eTlygxfFQIDAQABo4IBbjCCAWowHwYDVR0j\n"
"BBgwFoAUU3m/WqorSs9UgOHYm8Cd8rIDZsswHQYDVR0OBBYEFI2MXsRUrYrhd+mb\n"
"+ZsF4bgBjWHhMA4GA1UdDwEB/wQEAwIBhjASBgNVHRMBAf8ECDAGAQH/AgEAMB0G\n"
"A1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAbBgNVHSAEFDASMAYGBFUdIAAw\n"
"CAYGZ4EMAQIBMFAGA1UdHwRJMEcwRaBDoEGGP2h0dHA6Ly9jcmwudXNlcnRydXN0\n"
"LmNvbS9VU0VSVHJ1c3RSU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDB2Bggr\n"
"BgEFBQcBAQRqMGgwPwYIKwYBBQUHMAKGM2h0dHA6Ly9jcnQudXNlcnRydXN0LmNv\n"
"bS9VU0VSVHJ1c3RSU0FBZGRUcnVzdENBLmNydDAlBggrBgEFBQcwAYYZaHR0cDov\n"
"L29jc3AudXNlcnRydXN0LmNvbTANBgkqhkiG9w0BAQwFAAOCAgEAMr9hvQ5Iw0/H\n"
"ukdN+Jx4GQHcEx2Ab/zDcLRSmjEzmldS+zGea6TvVKqJjUAXaPgREHzSyrHxVYbH\n"
"7rM2kYb2OVG/Rr8PoLq0935JxCo2F57kaDl6r5ROVm+yezu/Coa9zcV3HAO4OLGi\n"
"H19+24rcRki2aArPsrW04jTkZ6k4Zgle0rj8nSg6F0AnwnJOKf0hPHzPE/uWLMUx\n"
"RP0T7dWbqWlod3zu4f+k+TY4CFM5ooQ0nBnzvg6s1SQ36yOoeNDT5++SR2RiOSLv\n"
"xvcRviKFxmZEJCaOEDKNyJOuB56DPi/Z+fVGjmO+wea03KbNIaiGCpXZLoUmGv38\n"
"sbZXQm2V0TP2ORQGgkE49Y9Y3IBbpNV9lXj9p5v//cWoaasm56ekBYdbqbe4oyAL\n"
"l6lFhd2zi+WJN44pDfwGF/Y4QA5C5BIG+3vzxhFoYt/jmPQT2BVPi7Fp2RBgvGQq\n"
"6jG35LWjOhSbJuMLe/0CjraZwTiXWTb2qHSihrZe68Zk6s+go/lunrotEbaGmAhY\n"
"LcmsJWTyXnW0OMGuf1pGg+pRyrbxmRE1a6Vqe8YAsOf4vmSyrcjC8azjUeqkk+B5\n"
"yOGBQMkKW+ESPMFgKuOXwIlCypTPRpgSabuY0MLTDXJLR27lk8QyKGOHQ+SwMj4K\n"
"00u/I5sUKUErmgQfky3xxzlIPK1aEn8=\n"
"-----END CERTIFICATE-----";


EthernetInterface eth;

AnalogIn vref(ADC_VREF);

int main()
{
    printf("Nigga\n");

    auto cb = ulog::callback_t([](ulog::log_level l , ulog::string_t s){
        printf("[%d] %s\n", (int)l, s );
    });

    ulog::set_callback(cb);

    

     printf("3637\n");

    auto err = eth.connect();

    printf("7369 %d\n", (int)err);
    TLSSocket socket;
    mqtt::Client client(&socket);

    printf("ебет...\n");

    err = socket.open(&eth);

    printf("тебя %d\n", (int)err);
    err = socket.set_root_ca_cert(ca_cert);

    printf("!!!! %d\n", (int)err);

    SocketAddress addr;
    err = eth.gethostbyname("api-demo.wolkabout.com", &addr);

    printf("fuck %d\n", (int)err);
    addr.set_port(8883);
    socket.connect(addr);

    printf("Hello man\n");

    client.connect_async("man","dude");
    
    while(!client.connected());

    printf("Err code : %d\n", (int)client.status());

    etl::error_handler::set_callback<error_handler>();

    motor1 = 0.5;
    motor2 = 0.5;


    ADC1->SMPR1 = 0333333333;
    ADC1->SMPR2 = 03333333333;
    ADC2->SMPR1 = 0333333333;
    ADC2->SMPR2 = 03333333333;
    ADC3->SMPR1 = 0333333333;
    ADC3->SMPR2 = 03333333333;

    while(1)
    {
        if(!manual_override)
        {
            heater_power_sink.update(ventctl::time());
            cooler_power_sink.update(ventctl::time());
        }

        if(log_state)
        {
            printf("T = %.2f (%.4fV), H = %.3f, C = %.1f, Ref = %.4fV\n", temp_room.read_temperature(), temp_room.read_voltage(), heater_power_limit.getLast(), cooler_power_limit.getLast(), vref.read() * 3.3);
            wait_ms(200);
        }

        term.try_command();
    }
}
