namespace ventctl
{
    template<typename TValue = float, typename TC = float>
    class PIDController
    {
    public:
        PIDController(TC kp, TC ki, TC kd) :
            m_k_p(kp),
            m_k_i(ki),
            m_k_d(kd),
            m_k_b(TC(0)),
            m_saturate(false),
            m_last_time(0),
            m_error(0),
            m_integral(0)
            {}
        
        PIDController(TC kp, TC ki, TC kd, TValue l, TValue h, TC kb = 0) :
            m_k_p(kp),
            m_k_i(ki),
            m_k_d(kd),
            m_k_b(TC(0)),
            m_last_time(0),
            m_error(0),
            m_saturate(true),
            m_high(h),
            m_low(l),
            m_integral(0)
            {}

        void setCoefficients(TC p, TC i, TC d)
        {
            m_k_p = p;
            m_k_d = d;
            m_k_i = i;
        }

        void setLastTime(float t)
        {
            m_last_time = t;
        }
        
        TValue nextValue(TValue error, float currentTime)
        {
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
        TC m_k_p, m_k_i, m_k_d, m_k_b;
        TValue m_low, m_high, m_error, m_integral;
        float m_last_time;
        bool m_saturate;
    };
}