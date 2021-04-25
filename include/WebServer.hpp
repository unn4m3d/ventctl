#pragma once
#include <mbed.h>

namespace ventctl
{
    class WebServer : public NonCopyable<WebServer>
    {
    public:
        WebServer(Socket&);
    private:
        Socket& m_socket;
    };
}