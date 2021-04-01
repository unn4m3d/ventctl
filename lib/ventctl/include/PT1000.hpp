#pragma once
#include <cstddef>
#include <cmath>

namespace ventctl
{
    inline float pt1000_resistance(float voltage)
    {
        return voltage*(-250) + 1500;
    }

    constexpr float pt1000_correction[] = {1.51892983e-15, -2.85842067e-12, -5.34227299e-09,
                              1.80282972e-05, -1.61875985e-02, 4.84112370e+00};
    
    
    inline float poly(const float* p, size_t s, float x)
    {
        float npow = 1, result = 0;
        for(int i = s - 1; i >= 0; i--)
        {
            result += npow * p[i];
            npow *= x;
        }
        return result;
    }
    
    inline float pt1000_temp(float resistance)
    {
        constexpr float A = 3.9083e-3, B = -5.775e-7, R0 = 1000;
        float result = ((-R0 * A + sqrtf(R0*R0 * A * A - 4* R0 * B * (R0 - resistance)))/(2*R0*B));
        
        if(resistance < R0) result += poly(pt1000_correction, 6, resistance); 
    
        return result;
    }
}