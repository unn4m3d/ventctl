#include <HiFiThermalSensor.hpp>
#include <PT1000.hpp>

ventctl::HiFiThermalSensor::HiFiThermalSensor(const char* name, uint8_t channel, SamplingTime st) :
    Peripheral<float>(name),
    m_channel(channel),
    m_samplingTime(st),
    m_voltage(0)
{}

struct adc_pin
{
    GPIO_TypeDef* gpio;
    uint8_t pin;
};

static constexpr const adc_pin pinmap[] = {
    {GPIOA, 0},
    {GPIOA, 1},
    {GPIOA, 2},
    {GPIOA, 3},
    {GPIOA, 4},
    {GPIOA, 5},
    {GPIOA, 6},
    {GPIOA, 7},
    {GPIOB, 0},
    {GPIOB, 1},
    {GPIOC, 0},
    {GPIOC, 1},
    {GPIOC, 2},
    {GPIOC, 3},
    {GPIOC, 4},
    {GPIOC, 5}
};

void ventctl::HiFiThermalSensor::initialize()
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN_Msk | RCC_APB2ENR_ADC2EN_Msk;
    
    auto gpio = pinmap[m_channel].gpio;
    auto num = pinmap[m_channel].pin;

    gpio->MODER |= 3 << (num * 2);
}

float ventctl::HiFiThermalSensor::read_value()
{
    return read_temperature();
}

void ventctl::HiFiThermalSensor::update()
{
    ADC1->CR2 |= ADC_CR2_ADON_Msk;
    ADC2->CR2 |= ADC_CR2_ADON_Msk;
    // Vrefint is ADC1_IN17
    ADC123_COMMON->CCR |= ADC_CCR_TSVREFE_Msk;
    
    auto chan = m_channel;
    auto* adc1_smpr = &ADC1->SMPR1;
    auto* adc2_smpr = &ADC2->SMPR2;
    if(chan > 9)
    {
        chan -= 10;
        adc2_smpr = &ADC2->SMPR1;
    }

    *adc1_smpr &= ~(7 << 21);
    *adc1_smpr |= (uint32_t)m_samplingTime << 21;
    *adc2_smpr &= ~(7 << (chan * 3));
    *adc2_smpr |= (uint32_t)m_samplingTime << (chan * 3);

    ADC1->SQR1 &= ~(0xF) << 20;
    ADC1->SQR1 |= 1 << 20;
    ADC1->SQR3 = 17;
    ADC2->SQR1 &= ~(0xF) << 20;
    ADC2->SQR1 |= 1 << 20;
    ADC2->SQR1 = m_channel;

    ADC1->CR2 |= ADC_CR2_SWSTART_Msk;
    ADC2->CR2 |= ADC_CR2_SWSTART_Msk;

    while(!((ADC1->SR & ADC_SR_EOC_Msk) && (ADC2->SR & ADC_SR_EOC_Msk)));

    uint16_t vrefint = ADC1->DR;
    uint16_t value = ADC2->DR;

    ADC1->SR &= ~(ADC_SR_STRT_Msk | ADC_SR_EOC_Msk);
    ADC2->SR &= ~(ADC_SR_STRT_Msk | ADC_SR_EOC_Msk);

    float v_cpu = 1.2 * 4096 / vrefint;
    m_voltage = value * v_cpu / 4096;
}

float ventctl::HiFiThermalSensor::read_voltage()
{
    return m_voltage;
}

float ventctl::HiFiThermalSensor::read_resistance()
{
    return read_resistance(read_voltage());
}

float ventctl::HiFiThermalSensor::read_resistance(float voltage)
{
    return ventctl::pt1000_resistance(voltage);
}

float ventctl::HiFiThermalSensor::read_temperature()
{
    return read_temperature(read_resistance());
}

float ventctl::HiFiThermalSensor::read_temperature(float resistance)
{
    return ventctl::pt1000_temp(resistance);
}

void ventctl::HiFiThermalSensor::print(ventctl::file_t file, bool s)
{
    Peripheral<float>::print(file, s);
    float voltage = read_voltage();
    float res = read_resistance(voltage);
    float temp = read_temperature(res);
    if(s)
        fprintf(file, "=%1.1f", temp);
    else
        fprintf(file, "= %1.4f C (%1.4f V, %1.2f Ohm)", temp, voltage, res);
}
