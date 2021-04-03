#include <Peripheral.hpp>
#include <unity.h>

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

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_periph_accepts_type);
    RUN_TEST(test_periph_set_value);
    UNITY_END();
}