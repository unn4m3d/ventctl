#include <PID.hpp>
#include <unity.h>
#include <functional>
#include <cstdio>
#include <iostream>
#include <Aperiodic.hpp>

ventctl::PIDController<float, float>
    pid_ol(1.5, 1.5, 0.5),
    pid_cl(1.5, 0.0, 0.0);


// TODO: Test saturation 


class PIDTest
{
public:
    PIDTest(ventctl::PIDController<float, float>& ctl, float step) :
        m_ctl(ctl),
        m_time(0.0),
        m_step(step),
        m_result(0.0)
        {}

    float test_until(float time, const std::function<float(float, float)>& callback)
    {
        if(time <= m_time) 
        {
            return NAN;
        }
        for(; m_time <= time; m_time += m_step)
        {
            
            m_result = m_ctl.nextValue(callback(m_time, m_result), m_time);
        }

        return m_result;
    }
private:
    ventctl::PIDController<float, float>& m_ctl;
    float m_time, m_step, m_result;
};

PIDTest pto(pid_ol, 0.001), ptc(pid_cl, 0.0001);

void test_pid_open_loop()
{
    auto input = [](float time, float output){ return 1.0; };
    auto result_1 = pto.test_until(1.0, input );
    TEST_ASSERT_FLOAT_WITHIN(0.1, 3.015, result_1);
    auto result_6 = pto.test_until(6.0, input );
    TEST_ASSERT_FLOAT_WITHIN(0.1, 10.5, result_6);
}

void test_pid_closed_loop()
{
    const auto task = 3.0;
    auto input = [&task](float time, float output)
    { 
        auto out = task - output; 
        if(out == INFINITY)
        {
            return 10.0;
        }
        else if(out == -INFINITY)
        {
            return -10.0;
        }
        else
        {
            return out;
        }
    };
    auto result_1 = ptc.test_until(1.0, input );
    TEST_ASSERT_FLOAT_WITHIN(0.1, 2.2, result_1);
    auto result_9 = ptc.test_until(4.0, input );
    TEST_ASSERT_FLOAT_WITHIN(0.01, 3.0, result_9);
}

void test_aperiodic_step_response()
{
    ventctl::Aperiodic w(1, 1);

    float result[5] = {0};

    for(int i = 1; i < 500; i++)
    {
        auto r = w.nextValue(1.0, i * 0.01);
        if(i % 100 == 0) result[i/100] = r;
        std::cout << "Value: " << r << std::endl;
    }

    TEST_ASSERT_FLOAT_WITHIN(0.1, 0, result[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01, 0.634, result[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1, result[4]);
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_pid_open_loop);
    RUN_TEST(test_pid_closed_loop);
    RUN_TEST(test_aperiodic_step_response);
    UNITY_END();
}