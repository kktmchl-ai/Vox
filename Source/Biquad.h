// Biquad.h -- RBJ "Audio EQ Cookbook" biquad filter. One instance = one 2nd-order
// section; the EQ combines several of these in series.
#pragma once
#include <cmath>
#include <complex>
#include <algorithm>

namespace voxchain
{

enum class FilterType { Highpass, LowShelf, Peak, HighShelf, Bandpass };

class Biquad
{
public:
    void setSampleRate (double newSr) { sr = newSr; }

    void setHighpass (double freq, double q)
    {
        const double w0 = 2.0 * M_PI * clampFreq (freq) / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * std::max (0.05, q));
        const double b0 =  (1.0 + cw) / 2.0, b1 = -(1.0 + cw), b2 = (1.0 + cw) / 2.0;
        const double a0 = 1.0 + alpha, a1 = -2.0 * cw, a2 = 1.0 - alpha;
        setCoeffs (b0, b1, b2, a0, a1, a2);
    }

    void setBandpass (double freq, double q)
    {
        const double w0 = 2.0 * M_PI * clampFreq (freq) / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * std::max (0.1, q));
        const double b0 = alpha, b1 = 0.0, b2 = -alpha;
        const double a0 = 1.0 + alpha, a1 = -2.0 * cw, a2 = 1.0 - alpha;
        setCoeffs (b0, b1, b2, a0, a1, a2);
    }

    void setPeak (double freq, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * M_PI * clampFreq (freq) / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * std::max (0.1, q));
        const double b0 = 1.0 + alpha * A, b1 = -2.0 * cw, b2 = 1.0 - alpha * A;
        const double a0 = 1.0 + alpha / A, a1 = -2.0 * cw, a2 = 1.0 - alpha / A;
        setCoeffs (b0, b1, b2, a0, a1, a2);
    }

    void setLowShelf (double freq, double gainDb, double s = 0.9)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * M_PI * clampFreq (freq) / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / std::max (0.1, s) - 1.0) + 2.0);
        const double twoRootAalpha = 2.0 * std::sqrt (A) * alpha;
        const double b0 =    A * ((A + 1.0) - (A - 1.0) * cw + twoRootAalpha);
        const double b1 =  2.0 * A * ((A - 1.0) - (A + 1.0) * cw);
        const double b2 =    A * ((A + 1.0) - (A - 1.0) * cw - twoRootAalpha);
        const double a0 =        (A + 1.0) + (A - 1.0) * cw + twoRootAalpha;
        const double a1 =   -2.0 * ((A - 1.0) + (A + 1.0) * cw);
        const double a2 =        (A + 1.0) + (A - 1.0) * cw - twoRootAalpha;
        setCoeffs (b0, b1, b2, a0, a1, a2);
    }

    void setHighShelf (double freq, double gainDb, double s = 0.9)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * M_PI * clampFreq (freq) / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / std::max (0.1, s) - 1.0) + 2.0);
        const double twoRootAalpha = 2.0 * std::sqrt (A) * alpha;
        const double b0 =    A * ((A + 1.0) + (A - 1.0) * cw + twoRootAalpha);
        const double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw);
        const double b2 =    A * ((A + 1.0) + (A - 1.0) * cw - twoRootAalpha);
        const double a0 =        (A + 1.0) - (A - 1.0) * cw + twoRootAalpha;
        const double a1 =    2.0 * ((A - 1.0) - (A + 1.0) * cw);
        const double a2 =        (A + 1.0) - (A - 1.0) * cw - twoRootAalpha;
        setCoeffs (b0, b1, b2, a0, a1, a2);
    }

    void reset() { z1 = z2 = 0.0; }

    inline float process (float x) noexcept
    {
        const double in = (double) x;
        const double out = c_b0 * in + z1;
        z1 = c_b1 * in + z2 - c_a1 * out;
        z2 = c_b2 * in - c_a2 * out;
        return (float) out;
    }

    // magnitude response at `freq` Hz, for drawing the EQ curve in the GUI.
    double magnitudeAt (double freq) const
    {
        const double w = 2.0 * M_PI * freq / sr;
        const std::complex<double> z = std::exp (std::complex<double> (0.0, -w));
        const std::complex<double> num = c_b0 + c_b1 * z + c_b2 * z * z;
        const std::complex<double> den = 1.0 + c_a1 * z + c_a2 * z * z;
        return std::abs (num / den);
    }

private:
    static double clampFreq (double f) { return std::clamp (f, 10.0, 22000.0); }

    void setCoeffs (double b0, double b1, double b2, double a0, double a1, double a2)
    {
        c_b0 = b0 / a0; c_b1 = b1 / a0; c_b2 = b2 / a0; c_a1 = a1 / a0; c_a2 = a2 / a0;
    }

    double sr = 44100.0;
    double c_b0 = 1.0, c_b1 = 0.0, c_b2 = 0.0, c_a1 = 0.0, c_a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;
};

} // namespace voxchain
