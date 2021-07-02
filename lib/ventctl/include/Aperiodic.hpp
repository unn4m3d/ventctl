#pragma once
#include <Unit.hpp>

namespace ventctl
{
    class Aperiodic : public UnitBase
    {
    public:
        Aperiodic(UnitBase& input, float k, float tp) : 
            m_input(input),
            m_k(k), 
            m_tp(tp),
            m_integral(0.0)
        {}

        virtual float getValueUncached(float time)
        {
            auto m_last_time = getLastTime();
            auto result = m_integral / m_tp;

            m_integral += (m_k * m_input.getValue(time) - result) * (time - m_last_time);

            return result;
        }

    private:
        UnitBase& m_input;
        float m_k, m_tp, m_integral;
    };
}

