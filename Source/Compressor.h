// Compressor.h -- classic log-domain feed-forward compressor. The gain computer
// works in dB (soft-knee quadratic around the threshold); the envelope follower
// smooths the *gain reduction* itself (a common, click-free approach) using
// separate attack/release time constants.
#pragma once
#include <cmath>
#include <algorithm>

namespace voxchain
{

class Compressor
{
public:
    void setSampleRate (double newSr) { sr = newSr; updateCoeffs(); }

    void setThresholdDb (float v) { thresholdDb = v; }
    void setRatio (float v)       { ratio = std::max (1.0f, v); }
    void setKneeDb (float v)      { kneeDb = std::max (0.0f, v); }
    void setAttackMs (float v)    { attackMs = std::max (0.05f, v); updateCoeffs(); }
    void setReleaseMs (float v)   { releaseMs = std::max (1.0f, v); updateCoeffs(); }
    void setMakeupDb (float v)    { makeupDb = v; }

    void reset() { envDb = 0.0f; grDb = 0.0f; }

    // Returns the processed sample; call gainReductionDb() afterwards for metering.
    inline float process (float x) noexcept
    {
        const float levelDb = 20.0f * std::log10 (std::max (std::abs (x), 1.0e-8f));

        // envelope follower on the *input level*, so attack/release are musical
        const float coeff = (levelDb > envDb) ? attackCoeff : releaseCoeff;
        envDb = levelDb + coeff * (envDb - levelDb);

        const float over = envDb - thresholdDb;
        float reducedDb;
        if (over <= -kneeDb * 0.5f)
            reducedDb = 0.0f;
        else if (over >= kneeDb * 0.5f)
            reducedDb = over - over / ratio;
        else
        {
            const float t = over + kneeDb * 0.5f;                 // 0..kneeDb across the knee
            reducedDb = (1.0f / ratio - 1.0f) * (t * t) / (2.0f * std::max (0.001f, kneeDb));
            reducedDb = -reducedDb;                                // quadratic soft-knee (Cookbook style)
        }

        grDb = reducedDb;                                          // positive = attenuating
        const float gain = std::pow (10.0f, (-reducedDb + makeupDb) / 20.0f);
        return x * gain;
    }

    float gainReductionDb() const noexcept { return grDb; }

private:
    void updateCoeffs()
    {
        attackCoeff  = std::exp (-1.0f / (0.001f * attackMs  * (float) sr));
        releaseCoeff = std::exp (-1.0f / (0.001f * releaseMs * (float) sr));
    }

    double sr = 44100.0;
    float thresholdDb = -18.f, ratio = 3.f, kneeDb = 6.f, attackMs = 12.f, releaseMs = 120.f, makeupDb = 0.f;
    float attackCoeff = 0.f, releaseCoeff = 0.f;
    float envDb = 0.f, grDb = 0.f;
};

} // namespace voxchain
