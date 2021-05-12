#pragma once

namespace ventctl
{
    class Aperiodic
    {
    public:
        Aperiodic(float k, float tp) : m_k(k), m_tp(tp), m_last_time(0.0), m_integral(0.0){}

        void setLastTime(float t)
        {
            m_last_time = t;
        }

        float nextValue(float input, float time)
        {
            auto result = m_integral / m_tp;

            m_integral += (m_k * input - result) * (time - m_last_time);
            m_last_time = time;

            return result;
        }

    private:
        float m_k, m_tp, m_last_time, m_integral;
    };
}

