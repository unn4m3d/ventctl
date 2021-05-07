#pragma once
#include <sstream>
#include <iostream>

#define SEEK_SET std::ios_base::beg
#define SEEK_CUR std::ios_base::cur
#define SEEK_END std::ios_base::end

class Stream
{
public:

    bool readable()
    {
        return m_stream.tellg() < m_stream.str().size();
    }

    ssize_t read(char* buf, size_t size)
    {
        auto pos = m_stream.tellg();

        m_stream.read(buf, size);

        return m_stream.tellg() - pos;
    }

    ssize_t write(const char* buf, size_t size)
    {
        auto pos = m_stream.tellp();

        m_stream.write(buf, size);
        return m_stream.tellp() - pos;
    }

    void rewind()
    {
        m_stream.seekg(std::ios_base::beg);
        m_stream.seekp(std::ios_base::beg);
    }
    
    std::string get_buf()
    {
        return m_stream.str();
    }

    std::streampos seek(ssize_t off, std::ios_base::seekdir whence)
    {
        m_stream.seekg(off, whence);
        return m_stream.tellg();
    }

private:
    std::stringstream m_stream;
};