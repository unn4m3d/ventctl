#pragma once

#include <mbed.h>

namespace mqtt
{
    class SocketStream final : public Stream
    {
    public:
        SocketStream(Socket* s) : m_socket(s){}

        virtual bool readable()
        {
            if(m_count > 0) return true;
            if(!m_count) m_offset = 0;

            if(m_offset + m_count >= 63)
            {
                return false;
            }

            int cnt = m_socket->recv(m_buffer, 64 - m_offset - m_count);

            if(!cnt) return false;

            m_count += cnt;
            return true;
        }

        virtual ssize_t read (void *buffer, size_t length)
        {
            //return m_socket->recv(buffer, length);

            if(m_count >= length)
            {
                std::memcpy(buffer, m_buffer + m_offset,  length);
                m_count -= length;
                m_offset += length;
                return length;
            }
            else
            {
                auto off = 0;
                auto old_length = length;
                while(length)
                {
                    auto cpl = (length > m_count) ? m_count : length;
                    std::memcpy((char*)buffer + off, m_buffer + m_offset, cpl);
                    length -= cpl;
                    off += cpl;
                    m_count -= cpl;
                    if(m_count) m_offset += cpl;
                    else m_offset = 0;

                    m_count = m_socket->recv(m_buffer, 64);
                }

                return old_length;
            }
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

        char m_buffer[16];
        size_t m_count, m_offset;
    };
}