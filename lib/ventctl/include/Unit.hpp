#pragma once
#include <functional>
#include <etl/vector.h>
#include <Peripheral.hpp>

namespace ventctl
{
    class UnitBase
    {
    public:
        virtual float getValueUncached(float time) = 0;

        UnitBase() : 
            m_last(0),
            m_last_time(0){}

        virtual void setLastTime(float time)
        {
            m_last_time = time;
        }

        virtual float getValue(float time)
        {
            if(m_last_time < time)
            {
                m_last_time = time;
                m_last = getValueUncached(time);
            }  

            return m_last;
        }
    private:
        float m_last, m_last_time;

    protected:
        float getLast() { return m_last;}
    
    };

    template<typename T>
    class Unit : public UnitBase
    {
    public:
        Unit(T& value, UnitBase& input) :
            m_unit(value),
            m_input(input)
        {}

        virtual void setLastTime(float time)
        {
            UnitBase::setLastTime(time);
            m_unit.setLastTime(time);
            m_input.setLastTime(time);
        }

        virtual float getValueUncached(float time)
        {
            return m_unit.nextValue(m_input.getValue(time), time);
        }

    private:
        T& m_unit;
        UnitBase &m_input;
    };

    using UnitRef = std::reference_wrapper<UnitBase>;

    class Sum : public UnitBase
    {
    public:
        using input_vec_type = etl::vector<std::pair<UnitRef, bool>, 4>;

        Sum(input_vec_type vec) : m_inputs(vec){}

        virtual void setLastTime(float time)
        {
            UnitBase::setLastTime(time);

            for(auto& input : m_inputs)
            {
                input.first.get().setLastTime(time);
            }
        }

        virtual float getValueUncached(float time)
        {
            auto result = 0;
            for(std::pair<UnitRef, bool>& input : m_inputs)
            {
                auto invert = input.second;
                auto value = input.first.get().getValue(time);
                result += value * (invert ? -1.0 : 1.0);
            }

            return result;
        }
    private:
        input_vec_type m_inputs;
    };

    class Gain : public UnitBase
    {
        public:
        Gain(float value, UnitBase& input) :
            m_gain(value),
            m_input(input),
            m_last(0),
            m_last_time(0)
        {}

        virtual void setLastTime(float time)
        {
            m_last_time = time;
            m_input.setLastTime(time);
        }

        virtual float getValueUncached(float time)
        {
            return m_input.getValue(time) * m_gain;
            
        }
    private:
        float m_gain;
        float m_last, m_last_time;
        UnitBase &m_input;
    };

    class Source : public UnitBase
    {
    public:
        using source_type = Peripheral<float>;

        Source(source_type& src) : m_source(src) {}

        virtual void setLastTime(float)
        {}

        virtual float getValueUncached(float time)
        {
            return m_source.read_value();
        }

    private:
        source_type& m_source;

    };

    // Use as either Relay<float> or Relay<Peripheral<float>&>
    template<typename TLim>
    class Relay : public UnitBase
    {
    public:
        Relay(TLim off, TLim on, UnitBase& src) : m_off(off), m_on(on), m_source(src){}

        virtual float getValueUncached(float time)
        {
            auto src = m_source.getValue(time);
            if(m_on >= m_off)
            {
                if(src > m_on)
                    return 1.0;
                else if(src < m_off)
                    return 0.0;
            }
            else
            {
                if(src < m_on)
                    return 1.0;
                else if(src > m_off)
                    return 0.0;
            }

            return getLast();
        }

    private:
        TLim m_off, m_on;
        UnitBase& m_source;
    };


    class SinkBase
    {
    public:
        virtual void update(float) = 0;
    };

    class Sink : public SinkBase
    {
    public:
        Sink(UnitBase& source, Peripheral<float>& sink) : m_source(source), m_sink(sink){}
        
        virtual void update(float time)
        {
            auto value = m_source.getValue(time);
            m_sink.accept_value(value);
        }
    private:
        UnitBase& m_source;
        Peripheral<float>& m_sink;
    };

    template<typename T>
    using PeriphRef = std::reference_wrapper<Peripheral<T>>;


    class SteppedOutputSink : public SinkBase
    {
    public:
        SteppedOutputSink(UnitBase& source, etl::ivector<PeriphRef<bool>>& peripherals, bool ordered = false) :
            m_source(source),
            m_peripherals(peripherals),
            m_ordered(ordered)
            {}

        virtual void update(float time)
        {
            if(m_ordered)
            {
                auto max_value = (float)((1 << m_peripherals.size()) - 1);

                auto scaled = (uint8_t)std::round(max_value * m_source.getValue(time));

                for(uint8_t i = 0; i < m_peripherals.size(); ++i)
                {
                    m_peripherals[i].get() = (scaled & i) != 0;
                }
            }
            else
            {
                auto scaled = (uint8_t)std::round(m_peripherals.size() * m_source.getValue(time));

                for(uint8_t i = 0; i < m_peripherals.size(); ++i)
                    m_peripherals[i].get() = scaled <= i;
            }
        }

    private:
        UnitBase& m_source;
        etl::ivector<PeriphRef<bool>>& m_peripherals;
        bool m_ordered;
    };

}