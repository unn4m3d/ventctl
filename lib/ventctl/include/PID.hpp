#include <Unit.hpp>

namespace ventctl
{
    template<typename TC = float, TKB = float>
    class PIDController : public UnitBase
    {
    public:
        using TValue = float;
        
        PIDController(UnitBase& input, TC kp, TC ki, TC kd, TValue l, TValue h, TKB kb) :
            m_input(input),
            m_k_p(kp),
            m_k_i(ki),
            m_k_d(kd),
            m_k_b(kb),
            m_low(l),
            m_high(h),
            m_integral(0),
            m_saturate(kb > 0)
            {}

        void setCoefficients(TC p, TC i, TC d)
        {
            m_k_p = p;
            m_k_d = d;
            m_k_i = i;
        }
        
        TValue getValueUncached(float time)
        {
            auto error = m_input.getValue(time);
            auto m_last_time = getLastTime();
            auto raw_output = m_k_p * error;
            if(m_k_i != 0)
            {
                m_integral += error * m_k_i * (currentTime - m_last_time);
                raw_output += m_integral;
            }

            if(m_k_d != 0)
            {
                raw_output += (error - m_error) / (currentTime - m_last_time);
            }

            if(m_saturate)
            {
                TValue overshoot(0);
                if(raw_output > m_high)
                { 
                    overshoot = raw_output - m_high;
                    raw_output = m_high; 
                }
                    
                if(raw_output < m_low)
                {
                    overshoot = raw_output - m_low;
                    raw_output = m_low;
                }

                if(m_k_b != 0)
                {
                    m_integral -= overshoot * m_k_b * (currentTime - m_last_time);
                }
                
            }

            setLastTime(currentTime);
            m_error = error;

            return raw_output;
        }

    private:
        UnitBase& m_input;
        TC m_k_p, m_k_i, m_k_d;
        TKB m_k_b;
        TValue m_low, m_high, m_integral;
        bool m_saturate;
    };
}