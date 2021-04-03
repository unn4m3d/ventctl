#include <Peripheral.hpp>
#include <PT1000.hpp>
#include <unity.h>

etl::vector<ventctl::PeripheralBase*, VC_PERIPH_CAP> ventctl::PeripheralBase::m_peripherals(0);

class DummyPeripheral : public ventctl::Peripheral<float>
{
public:
    DummyPeripheral(const char* name) :
        Peripheral(name),
        m_value(0.0)
        {}

    bool accept_value(float& v) override {m_value = v; }
    float read_value() override { return m_value; }

private:
    float m_value;
};

DummyPeripheral dummy("A dummy");

void test_periph_accepts_type()
{
    TEST_ASSERT_TRUE(dummy.accepts_type<float>());
    TEST_ASSERT_TRUE(dummy.accepts_type_id(util::type_id_v<float>));
    TEST_ASSERT_FALSE(dummy.accepts_type_id(util::type_id_v<int>));
    TEST_ASSERT_FALSE(dummy.accepts_type<char>());
}

void test_periph_set_value()
{
    float v = 13.37;
    auto dummy_ptr = static_cast<ventctl::PeripheralBase*>(&dummy);
    auto result = dummy_ptr->set_value<float>(&v);
    TEST_ASSERT_TRUE(result);
    result = dummy_ptr->get_value<float>(&v);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(v, 13.37);
}

void test_pt1000_conversion_pos()
{
    TEST_ASSERT_FLOAT_WITHIN(0.1, 0.0, ventctl::pt1000_temp(1000.0));
    TEST_ASSERT_FLOAT_WITHIN(0.1, 22.0, ventctl::pt1000_temp(1085.7));
    TEST_ASSERT_FLOAT_WITHIN(0.1, 100.0, ventctl::pt1000_temp(1385.1));
}

void test_pt1000_conversion_neg()
{
    TEST_ASSERT_FLOAT_WITHIN(0.5, -41.0, ventctl::pt1000_temp(838.7));
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_periph_accepts_type);
    RUN_TEST(test_periph_set_value);
    RUN_TEST(test_pt1000_conversion_pos);
    RUN_TEST(test_pt1000_conversion_neg);
    UNITY_END();
}