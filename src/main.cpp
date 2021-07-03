
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
#include <HiFiThermalSensor.hpp>
#include <settings.hpp>
#include <Aperiodic.hpp>
#include <ModbusMaster.h>
#include <MQTTClientMbedOs.h>
#include <NTPClient.h>


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
    cooler1("C_1", PD_0),
    cooler2("C_2", PD_1);

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

/*ventctl::HiFiThermalSensor
    temp_room("T_Room", 13),
    temp_iflow("T_IFlow", 12),
    temp_coolant("T_C", 0),
    temp_oflow("T_OFlow", 3);*/

ventctl::Variable<float>
    k_p("Kp", 0.5),
    temp_setting("S_Temp", 25.0),
    inflow_setting("S_IFlow", 3.0),
    outflow_setting("S_OFlow", 3.0);

ventctl::Variable<bool>
    log_state("Log", false),
    manual_override("Manual", false);
    
Serial pc(PC_12, PD_2);
RawSerial rs485(PA_9, PA_10);
DigitalOut rs485_de(PA_11), rs485_re(PA_12);

void pre_transmission()
{
    rs485_re = 0;
    rs485_de = 1;
}

void post_transmission()
{
    rs485_re = 0;
    rs485_de = 1;
}

ventctl::Term term(pc);
ModbusMaster modbus;

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
    pid_deicer(1,1,0,0,2,1);

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

ventctl::Aperiodic
    src_iflow_temp_f(src_iflow_temp, 1.0, 1.0),
    src_oflow_temp_f(src_oflow_temp, 1.0, 1.0);

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

ventctl::Aperiodic
    heater_power_filter(heater_power_limit, 1.0, 10.0),
    cooler_power_filter(cooler_power_limit, 1.0, 10.0);

ventctl::SteppedOutputSink
    heater_power_sink(heater_power_filter, heater_ref, false),
    cooler_power_sink(cooler_power_filter, cooler, false);


FileHandle *mbed::mbed_override_console(int fd)
{
    return &pc;
}

const char* ca_cert = "-----BEGIN CERTIFICATE-----\n"
"MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n"
"iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
"cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
"BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n"
"MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n"
"BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n"
"aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n"
"dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n"
"AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n"
"3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n"
"tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n"
"Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n"
"VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n"
"79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n"
"c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n"
"Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n"
"c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n"
"UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n"
"Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n"
"BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n"
"A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n"
"Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n"
"VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n"
"ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n"
"8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n"
"iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n"
"Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n"
"XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n"
"qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n"
"VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n"
"L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n"
"jjxDah2nGN59PRbxYvnKkKj9\n"
"-----END CERTIFICATE-----\n";


EthernetInterface eth;

AnalogIn vref(ADC_VREF);

int main()
{
    printf("Venctl Init...\n");
    etl::error_handler::set_callback<error_handler>();
    for(ventctl::PeripheralBase* p : ventctl::PeripheralBase::get_peripherals())
    {
        p->initialize();
    }

    auto cb = ulog::callback_t([](ulog::log_level l , ulog::string_t s){
        printf("[%d] %s\n", (int)l, s.c_str() );
    });

    ulog::set_callback(cb);

    modbus.preTransmission(&pre_transmission);
    modbus.postTransmission(&post_transmission);
    modbus.begin(228, rs485);

    auto result = ventctl::flash.init();

    printf("Flash init status: %d\n", (int)result);

    result = !ventctl::load_settings();


    printf("Settings load status : %d\n", (int)result);

    auto err = eth.connect();

    printf("Eth connection status: %d\n", (int)err);

    NTPClient ntp(&eth);

    time_t time = ntp.get_timestamp();

    printf("Timestamp : %lld\n (%s)", (long long signed) time, ctime(&time));

    if(time > 0) set_time(time);

    //TLSSocket socket;
    TCPSocket socket;
    
    //mqtt::Client client(&socket);
    MQTTClient client(&socket);

    err = socket.open(&eth);

    

    printf("Socket open status: %d\n", (int)err);

    //socket.set_hostname("api-demo.wolkabout.com");
    //err = socket.set_root_ca_cert(ca_cert);

    //printf("CA Cert status: %d\n", (int)err);

    SocketAddress addr;
    err = eth.gethostbyname("api-demo.wolkabout.com", &addr);

    printf("DNS query status: %d\n", (int)err);
    addr.set_port(2883);
    err = socket.connect(addr);

    printf("Connection status: %d\n", (int)err);

    MQTTPacket_connectData data MQTTPacket_connectData_initializer;
    data.clientID.cstring = "man";
    data.username.cstring = "test_0";
    data.password.cstring = "dude";

    result = client.connect(data);

    printf("MQTT C status %d'\n", (int)result);

    if(!result)
    {


        const char* topic = "d2p/sensor_reading/d/test_0/r/T0"; 
        ulog::string_t payload = ulog::join("{ \"utc\" : ", ::time(nullptr), ", \"data\":\"228.0\"}");
        MQTT::Message msg {
            .qos = MQTT::QOS1,
            .retained = false,
            .dup = false,
            .id = 1,
            .payload = payload.data(),
            .payloadlen = payload.size()
        };

        printf("Payload: %s\n", payload.c_str());

        result = client.publish(topic, msg);
        printf("Publish result: %d\n", (int)result);

        msg.payloadlen = 0;

        result = client.publish("ping/", msg);
    }

    /*result = client.connect_async("man","dude");
    auto start_time = ventctl::time();
    if(result)
    {
        printf("MQTT CONNECT packet sent\n");
        while(client.status() == mqtt::ConnectReasonCode::UNSPECIFIED && ventctl::time() < start_time + 10.0)
        {
            client.process(); // TODO : Thread
        }
        ulog::debug(ulog::join("MQTT client connection status : ", client.status()));
    }
    else
    {
        printf("Cannot send MQTT CONNECT packet\n");
    }*/
    
    motor1 = 0.5;
    motor2 = 0.5;

    while(1)
    {
        for(ventctl::PeripheralBase* p : ventctl::PeripheralBase::get_peripherals())
        {
            p->update();
        }

        if(!manual_override)
        {
            heater_power_sink.update(ventctl::time());
            cooler_power_sink.update(ventctl::time());
        }

        if(log_state)
        {
            static uint8_t i = 0;

            if(i++ == 0)
                printf("T = %.2f (%.4fV), H = %.3f, C = %.1f, Ref = %.4fV\n", temp_room.read_temperature(), temp_room.read_voltage(), heater_power_limit.getLast(), cooler_power_limit.getLast(), vref.read() * 3.3);
            //wait_ms(200);
        }

        term.try_command();
    }
}
