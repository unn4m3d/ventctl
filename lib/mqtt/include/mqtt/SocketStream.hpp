#pragma once

#include <mbed.h>

namespace mqtt
{
    class SocketStream final : public Stream
    {
    public:
        SocketStream(Socket* s) : m_socket(s){}

        virtual ssize_t read (void *buffer, size_t length)
        {
            return m_socket->recv(buffer, length);
        }

        virtual ssize_t write(void *buffer, size_t length)
        {
            return m_socket->send(buffer, length);
        }

        virtual off_t seek(off_t offset ,int whence)
        {
            if(whence != SEEK_CUR || offset < 0)
            {
                return -1;
            }

            char buffer[64];

            int ctr = 0;
            while(ctr < offset)
            {
                auto read = (offset - ctr) < 64 ? (offset - ctr) : 64;

                ctr += m_socket->recv(buffer, read);
            }

            return 0;
        }

        virtual int _putc(int c)
        {
            write(&c, 1);
            return c;
        }

        virtual int _getc()
        {
            int c;
            read(&c, 1);
            return c;
        }

    private:
        Socket* m_socket;
    };
}