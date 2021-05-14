#pragma once

namespace ventctl
{
    class Saturation
    {
    public:
        Saturation(float low, float high) : m_low(low), m_high(high) {}

        void setLastTime(float t)
        {
        }

        float nextValue(float input, float time)
        {
            if(input > m_high) return m_high;
            if(input < m_low) return m_low;
            return input;
        }

    private:
        float m_low, m_high;
    };
}