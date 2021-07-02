#pragma once
#include <Unit.hpp>

namespace ventctl
{
    class IntFilter : public UnitBase
    {
    public:
        IntFilter(UnitBase& input, float k) : 
        m_input(input), 
        m_k(k),
        m_last(0)
        {}

        virtual float getValueUncached(float time)
        {
            m_last = m_input.getValue(time) * m_k + m_last * (1 - m_k);
            return m_last;
        }
    private:
        UnitBase& m_input;
        float m_k, m_last;
    };
} // namespace ventctl
