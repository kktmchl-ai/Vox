#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
class VoxChainLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    VoxChainLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

//==============================================================================
class Meter final : public juce::Component
{
public:
    void setRange (float lo, float hi) { rangeLo = lo; rangeHi = hi; }
    void setValue (float db) { value = db; repaint(); }
    void paint (juce::Graphics&) override;

private:
    float value = -100.f, rangeLo = -60.f, rangeHi = 0.f;
};

//==============================================================================
class EqCurveView final : public juce::Component
{
public:
    explicit EqCurveView (VoxChainProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;

private:
    VoxChainProcessor& proc;
};

//==============================================================================
struct Knob
{
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
};

class VoxChainEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit VoxChainEditor (VoxChainProcessor&);
    ~VoxChainEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildBackground();

    Knob& addKnob (std::vector<std::unique_ptr<Knob>>& vec, const juce::String& id,
                   const juce::String& title, const juce::String& suffix, int decimals);
    juce::ToggleButton& addToggle (juce::ToggleButton& btn, const juce::String& id, const juce::String& text,
                                   std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach);

    // draws a card background, a header (title + optional enable toggle) and
    // returns the remaining content area inside the card.
    juce::Rectangle<int> beginCard (juce::Rectangle<int>& area, int totalHeight,
                                    const juce::String& title, juce::ToggleButton* enableToggle);
    static void layoutRow (juce::Rectangle<int> row, std::vector<std::unique_ptr<Knob>>& vec);

    VoxChainProcessor& proc;
    VoxChainLookAndFeel laf;
    juce::Image backgroundImage;
    bool  ledActive = false;
    float ledPhase  = 0.f;

    std::vector<std::unique_ptr<Knob>> masterKnobs, eqKnobsA, eqKnobsB, compKnobs, deessKnobs, satKnobs;

    juce::ToggleButton eqOnBtn, compOnBtn, deessOnBtn, deessListenBtn, satOnBtn, limOnBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        eqOnAtt, compOnAtt, deessOnAtt, deessListenAtt, satOnAtt, limOnAtt;

    EqCurveView eqCurve { proc };
    Meter inMeterL, inMeterR, outMeterL, outMeterR, compGrMeter, deessGrMeter, limGrMeter;
    juce::Label inMeterLbl, outMeterLbl, compGrLbl, deessGrLbl, limGrLbl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxChainEditor)
};
