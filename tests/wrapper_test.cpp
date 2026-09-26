#include "../Source/PluginProcessor.h"
#include <cstdio>

static const double SR = 48000.0;
static int failures = 0;
static void check (const char* name, bool ok, const juce::String& info = {})
{
    std::printf ("%s  %s  %s\n", ok ? "PASS" : "FAIL", name, info.toRawUTF8());
    if (! ok) ++failures;
}

static void setParam (VoxChainProcessor& p, const char* id, float realValue)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
}

static std::vector<float> tone (double f0, double seconds, float amp = 0.3f)
{
    std::vector<float> x ((size_t) (seconds * SR));
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = amp * (float) std::sin (2 * juce::MathConstants<double>::pi * f0 * (double) i / SR);
    return x;
}

static std::vector<float> render (VoxChainProcessor& p, const std::vector<float>& in, int channels = 2)
{
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (channels, 512);  juce::MidiBuffer midi;
    for (size_t pos = 0; pos < in.size(); pos += 512)
    {
        const int n = (int) std::min<size_t> (512, in.size() - pos);
        for (int c = 0; c < channels; ++c) std::copy (in.begin() + (long) pos, in.begin() + (long) pos + n, buf.getWritePointer (c));
        buf.setSize (channels, n, true, false, true);
        p.processBlock (buf, midi);
        std::copy (buf.getReadPointer (0), buf.getReadPointer (0) + n, out.begin() + (long) pos);
    }
    return out;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    VoxChainProcessor p;
    p.setPlayConfigDetails (2, 2, SR, 512);
    p.prepareToPlay (SR, 512);
    std::printf ("reported latency: %d samples\n", p.getLatencySamples());
    check ("latency reported to host", p.getLatencySamples() > 0);

    // 1. everything bypassed -> identity (aside from the limiter's lookahead delay)
    for (const char* id : { "eqOn", "compOn", "deessOn", "satOn", "limOn" }) setParam (p, id, 0.f);
    p.reset(); p.prepareToPlay (SR, 512);
    auto in = tone (300, 0.5);
    auto out = render (p, in);
    float maxErr = 0.f; for (size_t i = 0; i < in.size(); ++i) maxErr = std::max (maxErr, std::abs (out[i] - in[i]));
    check ("full bypass is near bit-exact", maxErr < 1.0e-5f, juce::String::formatted ("(max err %.2e)", maxErr));
    check ("latency is 0 with everything bypassed", p.getLatencySamples() == 0);

    // 2. re-enable everything, verify no NaN/Inf and output is peak-limited
    for (const char* id : { "eqOn", "compOn", "deessOn", "satOn", "limOn" }) setParam (p, id, 1.f);
    setParam (p, "ceiling", -1.0f);
    p.reset(); p.prepareToPlay (SR, 512);
    in = tone (300, 1.0, 1.5f);   // loud, ~+3.5dBFS
    out = render (p, in);
    check ("latency > 0 with limiter on", p.getLatencySamples() > 0);
    bool finite = true; float peak = 0.f;
    for (float v : out) { if (! std::isfinite (v)) finite = false; peak = std::max (peak, std::abs (v)); }
    check ("output finite", finite);
    check ("output respects ceiling (~-1dB)", 20.0f * std::log10 (peak) < -0.8f, juce::String::formatted ("(peak %.2f dBFS)", 20.0f*std::log10(peak)));

    // 3. compressor threshold parameter actually changes gain reduction telemetry
    for (const char* id : { "eqOn", "deessOn", "satOn", "limOn" }) setParam (p, id, 0.f);
    setParam (p, "compOn", 1.f); setParam (p, "thr", -6.f); setParam (p, "ratio", 4.f); setParam (p, "atk", 0.1f); setParam (p, "rel", 5.f);
    p.reset(); p.prepareToPlay (SR, 512);
    in = tone (300, 0.3, 0.9f);
    render (p, in);
    check ("compressor gain reduction telemetry > 0 dB when over threshold", p.compGrDb() > 1.0f, juce::String::formatted ("(%.2f dB)", p.compGrDb()));

    // 4. state save/restore round-trips a parameter
    setParam (p, "thr", -23.f);
    juce::MemoryBlock mb; p.getStateInformation (mb);
    setParam (p, "thr", -3.f);
    p.setStateInformation (mb.getData(), (int) mb.getSize());
    check ("state round-trip", std::abs (p.apvts.getRawParameterValue ("thr")->load() - (-23.f)) < 0.2f);

    // 5. mono layout accepted
    VoxChainProcessor m; m.setPlayConfigDetails (1, 1, SR, 512);
    juce::AudioProcessor::BusesLayout lay;
    lay.inputBuses.add (juce::AudioChannelSet::mono()); lay.outputBuses.add (juce::AudioChannelSet::mono());
    check ("mono/mono layout accepted", m.isBusesLayoutSupported (lay));

    // 6. editor constructs
    { std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
      check ("editor constructs", ed != nullptr && ed->getWidth() > 0); }

    std::printf ("\n%s\n", failures == 0 ? "ALL PASS" : "SOME FAILED");
    return failures;
}
