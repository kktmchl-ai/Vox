// Limiter.h -- lookahead peak limiter. A short delay line lets the gain
// smoother see peaks *before* they reach the output, so it can turn the gain
// down in time (true brick-wall behaviour) rather than reacting after the fact.
#pragma once
#include <cmath>
#include <algorithm>
#include <vector>

namespace voxchain
{

class Limiter
{
public:
    void prepare (double newSr, double lookaheadMs = 3.0)
    {
        sr = newSr;
        lookaheadSamples = std::max (1, (int) std::round (0.001 * lookaheadMs * sr));
        delay.assign ((size_t) lookaheadSamples, 0.f);
        peakWin.assign ((size_t) lookaheadSamples, 0.f);
        writePos = 0;
        releaseCoeff = std::exp (-1.0f / (0.001f * releaseMs * (float) sr));
        reset();
    }

    void setCeilingDb (float v) { ceiling = std::pow (10.0f, v / 20.0f); }
    void setReleaseMs (float v) { releaseMs = std::max (5.f, v); releaseCoeff = std::exp (-1.0f / (0.001f * releaseMs * (float) sr)); }

    void reset()
    {
        std::fill (delay.begin(), delay.end(), 0.f);
        std::fill (peakWin.begin(), peakWin.end(), 0.f);
        gain = 1.f; grDb = 0.f;
    }

    int latencySamples() const noexcept { return lookaheadSamples; }

    inline float process (float x) noexcept
    {
        const float ax = std::abs (x);
        const float delayed = delay[(size_t) writePos];
        delay[(size_t) writePos]   = x;
        peakWin[(size_t) writePos] = ax;

        // required gain so that the loudest sample currently in the lookahead window
        // will not exceed the ceiling
        float maxPeak = 0.f;
        for (float v : peakWin) maxPeak = std::max (maxPeak, v);
        const float targetGain = maxPeak > 1.0e-9f ? std::min (1.0f, ceiling / maxPeak) : 1.0f;

        // instant on the way down (must never overshoot), smoothed on the way back up
        gain = (targetGain < gain) ? targetGain : (targetGain + releaseCoeff * (gain - targetGain));

        writePos = (writePos + 1) % lookaheadSamples;
        grDb = -20.0f * std::log10 (std::max (gain, 1.0e-6f));
        return delayed * gain;
    }

    float gainReductionDb() const noexcept { return grDb; }

private:
    double sr = 44100.0;
    int lookaheadSamples = 1, writePos = 0;
    std::vector<float> delay, peakWin;
    float ceiling = 0.98f, releaseMs = 60.f, releaseCoeff = 0.f;
    float gain = 1.f, grDb = 0.f;
};

} // namespace voxchain
