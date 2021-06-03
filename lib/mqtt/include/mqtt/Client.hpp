#pragma once

#include <mqtt/types.hpp>
#include <mqtt/serializer.hpp>
#include <mqtt/SocketStream.hpp>
#if defined(CAPACITY)
    #undef CAPACITY
#endif
#include <etl/map.h>

namespace mqtt
{
    #ifdef MQTT_CUSTOM_SOCKET_TYPE
        using socket_t = MQTT_CUSTOM_SOCKET_TYPE;
    #else
        #include <mbed.h>

        using socket_t = Socket;
    #endif

    #ifdef MQTT_CUSTOM_MUTEX_TYPE
        using mutex_t = MQTT_CUSTOM_MUTEX_TYPE;
    #else
        using mutex_t = Mutex;
    #endif

    class Client;

    template<MessageType Type>
    struct pimpl;

    class Client
    {
    public:
        template<MessageType Type>
        using rx_cb_t = Callback<bool(Message<Type>&)>;

        Client(socket_t* sock) : m_socket(sock), m_conn_status(ConnectReasonCode::UNSPECIFIED), m_stream(sock){}
        
        template<mqtt::MessageType Type>
        bool send(mqtt::Message<Type>& msg)
        {
            m_mutex.lock();
            msg.fixed_header.length = msg.variable_header.length() + msg.payload.length();
            auto result = msg.write(m_stream);
            m_mutex.unlock();
            return result;
        }

        bool connect_async(const Payload<MessageType::CONNECT>& payload); 
        bool connect_async(const char* username, const char* password);

        bool publish(const etl::istring& topic, const etl::istring& data, uint8_t qos, bool dup = false);

        void set_rx_cb(const rx_cb_t<MessageType::PUBLISH>& cb)
        {
            m_rx_cb = cb;
        }

        void process();

        Stream& get_stream()
        {
            return m_stream;
        }

        bool connected()
        {
            return m_conn_status == ConnectReasonCode::SUCCESS;
        }

        ConnectReasonCode status()
        {
            return m_conn_status;
        }


    private:
        socket_t* m_socket;
        SocketStream m_stream;
        mutex_t m_mutex;
        rx_cb_t<MessageType::PUBLISH> m_rx_cb;
        ConnectReasonCode m_conn_status;
        uint16_t m_pid_counter;

        template<MessageType Type>
        friend struct mqtt::pimpl;
    };
}