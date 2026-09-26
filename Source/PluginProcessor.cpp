#include "PluginProcessor.h"
#include "PluginEditor.h"

using APF  = juce::AudioParameterFloat;
using APB  = juce::AudioParameterBool;
using Attr = juce::AudioParameterFloatAttributes;
using NRange = juce::NormalisableRange<float>;

juce::AudioProcessorValueTreeState::ParameterLayout VoxChainProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto hz  = [] (const juce::String& l) { return Attr().withLabel (l); };

    p.push_back (std::make_unique<APF> (ParameterID { "inGain", 1 },  "Input Gain",  NRange (-24.f, 24.f, 0.1f), 0.f, hz ("dB")));
    p.push_back (std::make_unique<APF> (ParameterID { "outGain", 1 }, "Output Gain", NRange (-24.f, 24.f, 0.1f), 0.f, hz ("dB")));

    // ---- EQ ----
    p.push_back (std::make_unique<APB> (ParameterID { "eqOn", 1 }, "EQ On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "hpFreq", 1 }, "High-pass", NRange (20.f, 500.f, 0.1f, 0.35f), 80.f, hz ("Hz")));

    p.push_back (std::make_unique<APB> (ParameterID { "lowOn", 1 }, "Low Shelf On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "lowFreq", 1 }, "Low Freq", NRange (40.f, 500.f, 0.1f, 0.4f), 120.f, hz ("Hz")));
    p.push_back (std::make_unique<APF> (ParameterID { "lowGain", 1 }, "Low Gain", NRange (-12.f, 12.f, 0.1f), 0.f, hz ("dB")));

    p.push_back (std::make_unique<APB> (ParameterID { "mid1On", 1 }, "Mid 1 On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "mid1Freq", 1 }, "Mid 1 Freq", NRange (150.f, 3000.f, 0.1f, 0.3f), 800.f, hz ("Hz")));
    p.push_back (std::make_unique<APF> (ParameterID { "mid1Gain", 1 }, "Mid 1 Gain", NRange (-12.f, 12.f, 0.1f), 0.f, hz ("dB")));
    p.push_back (std::make_unique<APF> (ParameterID { "mid1Q", 1 },    "Mid 1 Q",    NRange (0.3f, 5.f, 0.01f), 1.0f));

    p.push_back (std::make_unique<APB> (ParameterID { "mid2On", 1 }, "Mid 2 On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "mid2Freq", 1 }, "Mid 2 Freq", NRange (800.f, 8000.f, 0.1f, 0.3f), 3000.f, hz ("Hz")));
    p.push_back (std::make_unique<APF> (ParameterID { "mid2Gain", 1 }, "Mid 2 Gain", NRange (-12.f, 12.f, 0.1f), 0.f, hz ("dB")));
    p.push_back (std::make_unique<APF> (ParameterID { "mid2Q", 1 },    "Mid 2 Q",    NRange (0.3f, 5.f, 0.01f), 1.0f));

    p.push_back (std::make_unique<APB> (ParameterID { "highOn", 1 }, "High Shelf On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "highFreq", 1 }, "High Freq", NRange (2000.f, 16000.f, 1.f, 0.35f), 8000.f, hz ("Hz")));
    p.push_back (std::make_unique<APF> (ParameterID { "highGain", 1 }, "High Gain", NRange (-12.f, 12.f, 0.1f), 0.f, hz ("dB")));

    // ---- Compressor ----
    p.push_back (std::make_unique<APB> (ParameterID { "compOn", 1 }, "Comp On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "thr", 1 },    "Threshold", NRange (-40.f, 0.f, 0.1f), -18.f, hz ("dB")));
    p.push_back (std::make_unique<APF> (ParameterID { "ratio", 1 },  "Ratio",     NRange (1.f, 10.f, 0.01f, 0.5f), 3.f, hz (":1")));
    p.push_back (std::make_unique<APF> (ParameterID { "knee", 1 },   "Knee",      NRange (0.f, 24.f, 0.1f), 6.f, hz ("dB")));
    p.push_back (std::make_unique<APF> (ParameterID { "atk", 1 },    "Attack",    NRange (0.1f, 100.f, 0.01f, 0.3f), 12.f, hz ("ms")));
    p.push_back (std::make_unique<APF> (ParameterID { "rel", 1 },    "Release",   NRange (5.f, 1000.f, 1.f, 0.3f), 120.f, hz ("ms")));
    p.push_back (std::make_unique<APF> (ParameterID { "makeup", 1 }, "Makeup",    NRange (-6.f, 24.f, 0.1f), 0.f, hz ("dB")));

    // ---- De-esser ----
    p.push_back (std::make_unique<APB> (ParameterID { "deessOn", 1 }, "De-esser On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "deessFreq", 1 },  "De-ess Freq",  NRange (1500.f, 12000.f, 1.f, 0.4f), 6500.f, hz ("Hz")));
    p.push_back (std::make_unique<APF> (ParameterID { "deessThr", 1 },   "De-ess Thresh", NRange (-60.f, 0.f, 0.1f), -24.f, hz ("dB")));
    p.push_back (std::make_unique<APF> (ParameterID { "deessRange", 1 }, "De-ess Range",  NRange (0.f, 24.f, 0.1f), 12.f, hz ("dB")));
    p.push_back (std::make_unique<APB> (ParameterID { "deessListen", 1 }, "De-ess Listen", false));

    // ---- Saturator ----
    p.push_back (std::make_unique<APB> (ParameterID { "satOn", 1 }, "Sat On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "drive", 1 },  "Drive",   NRange (0.f, 100.f, 0.1f), 30.f, hz ("%")));
    p.push_back (std::make_unique<APF> (ParameterID { "warmth", 1 }, "Warmth",  NRange (0.f, 100.f, 0.1f), 20.f, hz ("%")));
    p.push_back (std::make_unique<APF> (ParameterID { "satMix", 1 }, "Sat Mix", NRange (0.f, 100.f, 0.1f), 50.f, hz ("%")));

    // ---- Limiter ----
    p.push_back (std::make_unique<APB> (ParameterID { "limOn", 1 }, "Limiter On", true));
    p.push_back (std::make_unique<APF> (ParameterID { "ceiling", 1 }, "Ceiling",     NRange (-12.f, 0.f, 0.1f), -0.3f, hz ("dB")));
    p.push_back (std::make_unique<APF> (ParameterID { "limRelease", 1 }, "Lim Release", NRange (5.f, 500.f, 1.f, 0.4f), 60.f, hz ("ms")));

    return { p.begin(), p.end() };
}

VoxChainProcessor::VoxChainProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool VoxChainProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& in  = l.getMainInputChannelSet();
    const auto& out = l.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void VoxChainProcessor::prepareToPlay (double sampleRate, int)
{
    preparedChannels = juce::jmax (1, juce::jmin (2, getTotalNumOutputChannels()));
    for (int c = 0; c < preparedChannels; ++c) core[c].prepare (sampleRate);
    setLatencySamples (core[0].latencySamples());
}

void VoxChainProcessor::releaseResources() {}

void VoxChainProcessor::pullParameters()
{
    const float inGain  = p ("inGain")->load();
    const float outGain = p ("outGain")->load();

    const bool eqOn = p ("eqOn")->load() > 0.5f;
    const float hpFreq = p ("hpFreq")->load();
    const bool lowOn = p ("lowOn")->load() > 0.5f;   const float lowFreq = p ("lowFreq")->load(), lowGain = p ("lowGain")->load();
    const bool m1On  = p ("mid1On")->load() > 0.5f;  const float m1Freq = p ("mid1Freq")->load(), m1Gain = p ("mid1Gain")->load(), m1Q = p ("mid1Q")->load();
    const bool m2On  = p ("mid2On")->load() > 0.5f;  const float m2Freq = p ("mid2Freq")->load(), m2Gain = p ("mid2Gain")->load(), m2Q = p ("mid2Q")->load();
    const bool hiOn  = p ("highOn")->load() > 0.5f;  const float hiFreq = p ("highFreq")->load(), hiGain = p ("highGain")->load();

    const bool compOn = p ("compOn")->load() > 0.5f;
    const float thr = p ("thr")->load(), ratio = p ("ratio")->load(), knee = p ("knee")->load();
    const float atk = p ("atk")->load(), rel = p ("rel")->load(), makeup = p ("makeup")->load();

    const bool deessOn = p ("deessOn")->load() > 0.5f;
    const float deessFreq = p ("deessFreq")->load(), deessThr = p ("deessThr")->load(), deessRange = p ("deessRange")->load();
    const bool deessListen = p ("deessListen")->load() > 0.5f;

    const bool satOn = p ("satOn")->load() > 0.5f;
    const float drive = p ("drive")->load() * 0.01f, warmth = p ("warmth")->load() * 0.01f, satMix = p ("satMix")->load() * 0.01f;

    const bool limOn = p ("limOn")->load() > 0.5f;
    const float ceiling = p ("ceiling")->load(), limRelease = p ("limRelease")->load();

    for (int c = 0; c < preparedChannels; ++c)
    {
        auto& ch = core[c];
        ch.setInputGainDb (inGain);
        ch.setOutputGainDb (outGain);

        ch.setEqOn (eqOn);
        ch.eqRef().setHighpass (true, hpFreq);
        ch.eqRef().setLowShelf (lowOn, lowFreq, lowGain);
        ch.eqRef().setMid1 (m1On, m1Freq, m1Gain, m1Q);
        ch.eqRef().setMid2 (m2On, m2Freq, m2Gain, m2Q);
        ch.eqRef().setHighShelf (hiOn, hiFreq, hiGain);

        ch.setCompOn (compOn);
        ch.compRef().setThresholdDb (thr);
        ch.compRef().setRatio (ratio);
        ch.compRef().setKneeDb (knee);
        ch.compRef().setAttackMs (atk);
        ch.compRef().setReleaseMs (rel);
        ch.compRef().setMakeupDb (makeup);

        ch.setDeessOn (deessOn);
        ch.deessRef().setFrequency (deessFreq);
        ch.deessRef().setThresholdDb (deessThr);
        ch.deessRef().setRangeDb (deessRange);
        ch.deessRef().setListen (deessListen);

        ch.setSatOn (satOn);
        ch.satRef().setDrive (drive);
        ch.satRef().setWarmth (warmth);
        ch.satRef().setMix (satMix);

        ch.setLimiterOn (limOn);
        ch.limRef().setCeilingDb (ceiling);
        ch.limRef().setReleaseMs (limRelease);
    }

    const int newLatency = core[0].latencySamples();
    if (newLatency != getLatencySamples())
        setLatencySamples (newLatency);
}

void VoxChainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin (buffer.getNumChannels(), preparedChannels);
    if (numCh < 1 || numSamples < 1) return;

    pullParameters();

    float peakGrComp = 0.f, peakGrDeess = 0.f, peakGrLim = 0.f;

    for (int c = 0; c < numCh; ++c)
    {
        auto* d = buffer.getWritePointer (c);
        float inPeak = 0.f, outPeak = 0.f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float in = d[i];
            inPeak = std::max (inPeak, std::abs (in));
            const float out = core[c].process (in);
            d[i] = out;
            outPeak = std::max (outPeak, std::abs (out));
        }
        inMeter[(size_t) c].store (20.f * std::log10 (std::max (inPeak, 1.0e-6f)));
        outMeter[(size_t) c].store (20.f * std::log10 (std::max (outPeak, 1.0e-6f)));
        peakGrComp  = std::max (peakGrComp,  core[c].compGrDb());
        peakGrDeess = std::max (peakGrDeess, core[c].deessGrDb());
        peakGrLim   = std::max (peakGrLim,   core[c].limGrDb());
    }

    compGr.store (peakGrComp);
    deessGr.store (peakGrDeess);
    limGr.store (peakGrLim);
}

juce::AudioProcessorEditor* VoxChainProcessor::createEditor() { return new VoxChainEditor (*this); }

void VoxChainProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void VoxChainProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxChainProcessor(); }
