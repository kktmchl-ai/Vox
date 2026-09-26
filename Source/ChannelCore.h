// ChannelCore.h -- the whole vocal channel strip in one class:
//   Input Gain -> EQ (HPF + 4 bands) -> Compressor -> De-esser -> Saturator
//   -> Output Gain -> Lookahead Limiter -> Output
// Each processing stage can be independently bypassed. Works on one channel;
// the plugin runs one instance per audio channel (mono or stereo).
#pragma once
#include "Eq.h"
#include "Compressor.h"
#include "DeEsser.h"
#include "Saturator.h"
#include "Limiter.h"

namespace voxchain
{

class ChannelCore
{
public:
    void prepare (double sr)
    {
        eq.setSampleRate (sr);
        comp.setSampleRate (sr);
        deess.setSampleRate (sr);
        sat.setSampleRate (sr);
        limiter.prepare (sr, 3.0);
        reset();
    }

    void reset() { eq.reset(); comp.reset(); deess.reset(); sat.reset(); limiter.reset(); }

    int latencySamples() const noexcept { return limiterOn ? limiter.latencySamples() : 0; }

    inline float process (float x) noexcept
    {
        x *= inputGain;

        if (eqOn)    x = eq.process (x);
        if (compOn)  x = comp.process (x);
        if (deessOn) x = deess.process (x);
        if (satOn)   x = sat.process (x);

        x *= outputGain;

        // Bypassing the limiter removes its lookahead delay too (latencySamples()
        // reflects this), which is a deliberate, user-triggered change -- not a
        // per-sample condition -- so the brief re-sync a host does on a latency
        // change is an acceptable trade for "off really means off".
        if (limiterOn) x = limiter.process (x);

        return x;
    }

    // parameters -------------------------------------------------------------
    void setInputGainDb  (float db) { inputGain  = std::pow (10.f, db / 20.f); }
    void setOutputGainDb (float db) { outputGain = std::pow (10.f, db / 20.f); }

    void setEqOn (bool b)    { eqOn = b; }
    void setCompOn (bool b)  { compOn = b; }
    void setDeessOn (bool b) { deessOn = b; }
    void setSatOn (bool b)   { satOn = b; }
    void setLimiterOn (bool b) { limiterOn = b; }

    Eq&         eqRef()    { return eq; }
    const Eq&   eqRef() const { return eq; }
    Compressor& compRef()  { return comp; }
    DeEsser&    deessRef() { return deess; }
    Saturator&  satRef()   { return sat; }
    Limiter&    limRef()   { return limiter; }

    float compGrDb()  const { return compOn  ? comp.gainReductionDb()  : 0.f; }
    float deessGrDb() const { return deessOn ? deess.gainReductionDb() : 0.f; }
    float limGrDb()   const { return limiterOn ? limiter.gainReductionDb() : 0.f; }

private:
    Eq eq; Compressor comp; DeEsser deess; Saturator sat; Limiter limiter;
    float inputGain = 1.f, outputGain = 1.f;
    bool eqOn = true, compOn = true, deessOn = true, satOn = true, limiterOn = true;
};

} // namespace voxchain
