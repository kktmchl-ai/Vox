// DeEsser.h -- split-band de-esser. A bandpass filter isolates the sibilant
// band; a fast peak detector on that band drives extra gain reduction which is
// applied only to the band before it is summed back with the untouched rest
// of the signal ("rest" = input minus band, computed with the same filter so
// the two halves reconstruct the original signal when no reduction is applied).
#pragma once
#include "Biquad.h"
#include <cmath>
#include <algorithm>

namespace voxchain
{

class DeEsser
{
public:
    void setSampleRate (double newSr)
    {
        sr = newSr;
        band.setSampleRate (sr);
        updateFilter();
        attackCoeff  = std::exp (-1.0f / (0.001f * 0.5f  * (float) sr));   // fast: sibilance is transient
        releaseCoeff = std::exp (-1.0f / (0.001f * 40.f  * (float) sr));
    }

    void setFrequency (float hz) { freq = std::clamp (hz, 1500.f, 12000.f); updateFilter(); }
    void setThresholdDb (float v) { thresholdDb = v; }
    void setRangeDb (float v) { rangeDb = std::max (0.f, v); }
    void setListen (bool b) { listen = b; }

    void reset() { band.reset(); envDb = -80.f; grDb = 0.f; }

    inline float process (float x) noexcept
    {
        const float bandSig = band.process (x);
        const float rest    = x - bandSig;

        const float levelDb = 20.0f * std::log10 (std::max (std::abs (bandSig), 1.0e-8f));
        const float coeff = (levelDb > envDb) ? attackCoeff : releaseCoeff;
        envDb = levelDb + coeff * (envDb - levelDb);

        const float over = envDb - thresholdDb;
        float reduceDb = over > 0.f ? std::min (over, rangeDb) : 0.f;    // hard-ratio above threshold, capped
        grDb = reduceDb;
        const float gain = std::pow (10.0f, -reduceDb / 20.0f);

        if (listen) return bandSig * gain;
        return rest + bandSig * gain;
    }

    float gainReductionDb() const noexcept { return grDb; }

private:
    void updateFilter() { band.setBandpass (freq, 2.2); }

    double sr = 44100.0;
    Biquad band;
    float freq = 6500.f, thresholdDb = -24.f, rangeDb = 12.f;
    bool listen = false;
    float attackCoeff = 0.f, releaseCoeff = 0.f, envDb = -80.f, grDb = 0.f;
};

} // namespace voxchain
