// Eq.h -- high-pass filter + 4-band parametric EQ (low shelf, two peaking
// bands, high shelf), each independently bypassable.
#pragma once
#include "Biquad.h"

namespace voxchain
{

class Eq
{
public:
    void setSampleRate (double sr)
    {
        hp.setSampleRate (sr); low.setSampleRate (sr);
        mid1.setSampleRate (sr); mid2.setSampleRate (sr); high.setSampleRate (sr);
    }

    void setHighpass (bool on, float freq)          { hpOn = on;   hp.setHighpass (freq, 0.707); }
    void setLowShelf  (bool on, float freq, float g) { lowOn = on;  low.setLowShelf (freq, g); }
    void setMid1      (bool on, float freq, float g, float q) { mid1On = on; mid1.setPeak (freq, q, g); }
    void setMid2      (bool on, float freq, float g, float q) { mid2On = on; mid2.setPeak (freq, q, g); }
    void setHighShelf (bool on, float freq, float g) { highOn = on; high.setHighShelf (freq, g); }

    void reset() { hp.reset(); low.reset(); mid1.reset(); mid2.reset(); high.reset(); }

    inline float process (float x) noexcept
    {
        if (hpOn)   x = hp.process (x);
        if (lowOn)  x = low.process (x);
        if (mid1On) x = mid1.process (x);
        if (mid2On) x = mid2.process (x);
        if (highOn) x = high.process (x);
        return x;
    }

    // combined magnitude response at `freq`, for drawing the EQ curve (dB)
    double responseDb (double freq) const
    {
        double m = 1.0;
        if (hpOn)   m *= hp.magnitudeAt (freq);
        if (lowOn)  m *= low.magnitudeAt (freq);
        if (mid1On) m *= mid1.magnitudeAt (freq);
        if (mid2On) m *= mid2.magnitudeAt (freq);
        if (highOn) m *= high.magnitudeAt (freq);
        return 20.0 * std::log10 (std::max (m, 1.0e-6));
    }

private:
    Biquad hp, low, mid1, mid2, high;
    bool hpOn = true, lowOn = true, mid1On = true, mid2On = true, highOn = true;
};

} // namespace voxchain
