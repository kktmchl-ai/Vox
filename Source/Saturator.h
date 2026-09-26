// Saturator.h -- tanh waveshaper. `drive` controls how hard the signal is
// pushed into the curve (more odd harmonics); `warmth` adds a small even-order
// term (asymmetric clipping, like a tube) for a "warm" character; a one-pole
// DC blocker removes the resulting bias; `mix` blends dry/wet (parallel drive).
#pragma once
#include <cmath>
#include <algorithm>

namespace voxchain
{

class Saturator
{
public:
    void setSampleRate (double newSr) { sr = newSr; }
    void setDrive (float v)  { drive = std::clamp (v, 0.f, 1.f); }
    void setWarmth (float v) { warmth = std::clamp (v, 0.f, 1.f); }
    void setMix (float v)    { mix = std::clamp (v, 0.f, 1.f); }

    void reset() { dcX1 = dcY1 = 0.f; }

    inline float process (float x) noexcept
    {
        const float driveAmt = 1.0f + drive * 14.0f;                 // 1x .. 15x pre-gain into the curve
        const float norm = std::tanh (driveAmt);                     // keeps ~unity gain at low levels
        float shaped = std::tanh (driveAmt * x) / std::max (norm, 1.0e-6f);

        // small even-harmonic bias term for "warmth" (tube-style asymmetry)
        shaped += warmth * 0.25f * (shaped * shaped) * (x >= 0.f ? 1.f : -1.f);

        // DC blocker (one-pole highpass, ~5 Hz corner)
        const float y = shaped - dcX1 + 0.995f * dcY1;
        dcX1 = shaped; dcY1 = y;

        return x + mix * (y - x);
    }

private:
    double sr = 44100.0;
    float drive = 0.3f, warmth = 0.2f, mix = 0.5f;
    float dcX1 = 0.f, dcY1 = 0.f;
};

} // namespace voxchain
