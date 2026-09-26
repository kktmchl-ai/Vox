#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg        { 0xff0a1120 };
    const juce::Colour kBgSoft    { 0xff16233b };
    const juce::Colour kInk       { 0xff060a13 };
    const juce::Colour kPanel     { 0xff121b30 };
    const juce::Colour kPanelEdge { 0xff2b3a56 };
    const juce::Colour kTrack     { 0xff222e46 };
    const juce::Colour kText      { 0xffeaf1fb };
    const juce::Colour kTextDim   { 0xff7d8ba3 };
    const juce::Colour kBlue      { 0xff2f8fff };
    const juce::Colour kBlueDeep  { 0xff1652c4 };
    const juce::Colour kBlueGlow  { 0xff8fcaff };
    const juce::Colour kWarn      { 0xffffa23f };
    const juce::Colour kRed       { 0xffff5f6d };

    void drawGlowDot (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour colour, float glowAlpha = 0.55f)
    {
        juce::ColourGradient halo (colour.withAlpha (glowAlpha), c.x, c.y,
                                   colour.withAlpha (0.f), c.x + r * 2.6f, c.y, true);
        g.setGradientFill (halo);
        g.fillEllipse (c.x - r * 2.6f, c.y - r * 2.6f, r * 5.2f, r * 5.2f);
        g.setColour (colour);
        g.fillEllipse (c.x - r, c.y - r, r * 2.f, r * 2.f);
        const float hi = r * 0.42f;
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.fillEllipse (c.x - hi, c.y - hi, hi * 2.f, hi * 2.f);
    }
}

//==============================================================================
VoxChainLookAndFeel::VoxChainLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, kBg);
    setColour (juce::Slider::textBoxTextColourId, kBlueGlow);
    setColour (juce::Slider::textBoxOutlineColourId, kPanelEdge);
    setColour (juce::Slider::textBoxBackgroundColourId, kPanel);
    setColour (juce::Label::textColourId, kText);
    setColour (juce::ToggleButton::textColourId, kText);
}

void VoxChainLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                            float pos, float rotaryStartAngle, float rotaryEndAngle,
                                            juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.f;
    const auto  centre = bounds.getCentre();
    const float angle  = rotaryStartAngle + pos * (rotaryEndAngle - rotaryStartAngle);
    const float thickness = radius * 0.22f;
    const float arcR = radius - thickness * 0.5f - 2.f;

    g.setColour (kInk.withAlpha (0.55f));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.f, radius * 2.f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (kTrack);
    g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, rotaryStartAngle, angle, true);
    juce::ColourGradient grad (kBlueGlow, centre.x, centre.y - arcR, kBlueDeep, centre.x, centre.y + arcR, false);
    g.setGradientFill (grad);
    g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (kPanelEdge);
    g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.f, radius * 2.f, 1.1f);

    const float tipR = radius - thickness * 0.3f;
    const juce::Point<float> tip (centre.x + tipR * std::cos (angle - juce::MathConstants<float>::halfPi),
                                  centre.y + tipR * std::sin (angle - juce::MathConstants<float>::halfPi));
    drawGlowDot (g, tip, radius * 0.15f, kBlueGlow, 0.7f);
}

void VoxChainLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& btn, bool, bool)
{
    auto b = btn.getLocalBounds().toFloat();
    const float d = juce::jmin (14.f, b.getHeight() - 4.f);
    const juce::Point<float> dot (b.getX() + d * 0.5f + 1.f, b.getCentreY());

    if (btn.getToggleState())
        drawGlowDot (g, dot, d * 0.5f, kBlueGlow, 0.6f);
    else
    {
        g.setColour (kPanelEdge);
        g.drawEllipse (dot.x - d * 0.5f, dot.y - d * 0.5f, d, d, 1.4f);
    }

    g.setColour (btn.getToggleState() ? kText : kTextDim);
    g.setFont (juce::FontOptions (12.f, juce::Font::bold));
    g.drawText (btn.getButtonText(), b.withTrimmedLeft (d + 10.f), juce::Justification::centredLeft);
}

//==============================================================================
void Meter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (kInk);
    g.fillRoundedRectangle (r, 3.f);

    const float t = juce::jlimit (0.f, 1.f, (value - rangeLo) / juce::jmax (0.001f, rangeHi - rangeLo));
    const bool vertical = getHeight() > getWidth();

    juce::Rectangle<float> fill;
    if (vertical) fill = r.removeFromBottom (r.getHeight() * t);
    else          fill = r.removeFromLeft (r.getWidth() * t);

    juce::ColourGradient grad = vertical
        ? juce::ColourGradient (kBlueGlow, 0.f, getHeight() * (1.f - t), kBlueDeep, 0.f, (float) getHeight(), false)
        : juce::ColourGradient (kBlueDeep, 0.f, 0.f, kBlueGlow, getWidth() * t, 0.f, false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (fill, 3.f);

    g.setColour (kPanelEdge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.f, 1.f);
}

//==============================================================================
void EqCurveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (kInk.withAlpha (0.5f));
    g.fillRoundedRectangle (r, 6.f);

    const float x0 = r.getX() + 4.f, x1 = r.getRight() - 4.f, w = x1 - x0;
    const float y0 = r.getY() + 4.f, y1 = r.getBottom() - 4.f, h = y1 - y0;
    const double dbRange = 15.0;

    // 0 dB centre line + grid
    g.setColour (kPanelEdge.withAlpha (0.6f));
    g.drawHorizontalLine ((int) (y0 + h * 0.5f), x0, x1);
    for (float fx : { 100.f, 1000.f, 10000.f })
    {
        const float lx = x0 + w * (std::log10 (fx / 20.f) / std::log10 (20000.f / 20.f));
        g.drawVerticalLine ((int) lx, y0, y1);
    }

    juce::Path curve;
    const int N = 120;
    for (int i = 0; i <= N; ++i)
    {
        const double freq = 20.0 * std::pow (20000.0 / 20.0, (double) i / N);
        const double db = juce::jlimit (-dbRange, dbRange, proc.eqResponseDb (freq));
        const float px = x0 + w * (float) i / N;
        const float py = y0 + h * 0.5f - (float) (db / dbRange) * (h * 0.5f);
        if (i == 0) curve.startNewSubPath (px, py); else curve.lineTo (px, py);
    }
    g.setColour (kBlueGlow);
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path fillPath (curve);
    fillPath.lineTo (x1, y0 + h * 0.5f);
    fillPath.lineTo (x0, y0 + h * 0.5f);
    fillPath.closeSubPath();
    g.setColour (kBlue.withAlpha (0.12f));
    g.fillPath (fillPath);
}

//==============================================================================
Knob& VoxChainEditor::addKnob (std::vector<std::unique_ptr<Knob>>& vec, const juce::String& id,
                               const juce::String& title, const juce::String& suffix, int decimals)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 20);
    k->slider.setTextValueSuffix (suffix);
    k->slider.setNumDecimalPlacesToDisplay (decimals);
    auto* param = proc.apvts.getParameter (id);
    k->slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    addAndMakeVisible (k->slider);

    k->label.setText (title, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::FontOptions (11.f, juce::Font::bold));
    k->label.setColour (juce::Label::textColourId, kTextDim);
    addAndMakeVisible (k->label);

    k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    vec.push_back (std::move (k));
    return *vec.back();
}

juce::ToggleButton& VoxChainEditor::addToggle (juce::ToggleButton& btn, const juce::String& id, const juce::String& text,
                                               std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attach)
{
    btn.setButtonText (text);
    addAndMakeVisible (btn);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, btn);
    return btn;
}

void VoxChainEditor::layoutRow (juce::Rectangle<int> row, std::vector<std::unique_ptr<Knob>>& vec)
{
    if (vec.empty()) return;
    const int w = row.getWidth() / (int) vec.size();
    for (auto& k : vec)
    {
        auto cell = row.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell.reduced (6, 0));
    }
}

juce::Rectangle<int> VoxChainEditor::beginCard (juce::Rectangle<int>& area, int totalHeight,
                                                const juce::String& title, juce::ToggleButton* enableToggle)
{
    auto cardArea = area.removeFromTop (totalHeight);
    area.removeFromTop (12);                                   // gap to next card

    // background card is drawn in the editor's paint() using the same rect,
    // so store nothing here -- just lay out the header and return the content area.
    auto content = cardArea.reduced (14, 10);
    auto header = content.removeFromTop (22);
    if (enableToggle != nullptr)
        enableToggle->setBounds (header.removeFromLeft (140));
    content.removeFromTop (4);
    (void) title;
    return content;
}

//==============================================================================
VoxChainEditor::VoxChainEditor (VoxChainProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (760, 1180);

    addToggle (eqOnBtn, "eqOn", "EQ", eqOnAtt);
    addToggle (compOnBtn, "compOn", "COMPRESSOR", compOnAtt);
    addToggle (deessOnBtn, "deessOn", "DE-ESSER", deessOnAtt);
    addToggle (deessListenBtn, "deessListen", "LISTEN", deessListenAtt);
    addToggle (satOnBtn, "satOn", "SATURATION", satOnAtt);
    addToggle (limOnBtn, "limOn", "LIMITER", limOnAtt);

    addAndMakeVisible (eqCurve);

    addKnob (masterKnobs, "inGain",  "INPUT",  " dB", 1);
    addKnob (masterKnobs, "outGain", "OUTPUT", " dB", 1);
    addKnob (masterKnobs, "ceiling", "CEILING", " dB", 1);
    addKnob (masterKnobs, "limRelease", "LIM RELEASE", " ms", 0);

    addKnob (eqKnobsA, "hpFreq",   "HPF",       " Hz", 0);
    addKnob (eqKnobsA, "lowFreq",  "LOW FREQ",  " Hz", 0);
    addKnob (eqKnobsA, "lowGain",  "LOW GAIN",  " dB", 1);
    addKnob (eqKnobsA, "mid1Freq", "MID1 FREQ", " Hz", 0);
    addKnob (eqKnobsA, "mid1Gain", "MID1 GAIN", " dB", 1);
    addKnob (eqKnobsA, "mid1Q",    "MID1 Q",    "",    2);

    addKnob (eqKnobsB, "mid2Freq", "MID2 FREQ", " Hz", 0);
    addKnob (eqKnobsB, "mid2Gain", "MID2 GAIN", " dB", 1);
    addKnob (eqKnobsB, "mid2Q",    "MID2 Q",    "",    2);
    addKnob (eqKnobsB, "highFreq", "HIGH FREQ", " Hz", 0);
    addKnob (eqKnobsB, "highGain", "HIGH GAIN", " dB", 1);

    addKnob (compKnobs, "thr",    "THRESHOLD", " dB", 1);
    addKnob (compKnobs, "ratio",  "RATIO",     ":1",  1);
    addKnob (compKnobs, "knee",   "KNEE",      " dB", 1);
    addKnob (compKnobs, "atk",    "ATTACK",    " ms", 2);
    addKnob (compKnobs, "rel",    "RELEASE",   " ms", 0);
    addKnob (compKnobs, "makeup", "MAKEUP",    " dB", 1);

    addKnob (deessKnobs, "deessFreq",  "FREQUENCY", " Hz", 0);
    addKnob (deessKnobs, "deessThr",   "THRESHOLD", " dB", 1);
    addKnob (deessKnobs, "deessRange", "RANGE",     " dB", 1);

    addKnob (satKnobs, "drive",  "DRIVE",  " %", 0);
    addKnob (satKnobs, "warmth", "WARMTH", " %", 0);
    addKnob (satKnobs, "satMix", "MIX",    " %", 0);

    for (auto* m : { &inMeterL, &inMeterR, &outMeterL, &outMeterR })
    {
        m->setRange (-48.f, 3.f);
        addAndMakeVisible (m);
    }
    compGrMeter.setRange (0.f, 24.f);
    deessGrMeter.setRange (0.f, 24.f);
    limGrMeter.setRange (0.f, 24.f);
    addAndMakeVisible (compGrMeter);
    addAndMakeVisible (deessGrMeter);
    addAndMakeVisible (limGrMeter);

    for (auto* l : { &inMeterLbl, &outMeterLbl, &compGrLbl, &deessGrLbl, &limGrLbl })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::FontOptions (10.f, juce::Font::bold));
        l->setColour (juce::Label::textColourId, kTextDim);
        addAndMakeVisible (l);
    }
    inMeterLbl.setText ("IN", juce::dontSendNotification);
    outMeterLbl.setText ("OUT", juce::dontSendNotification);
    compGrLbl.setText ("GR", juce::dontSendNotification);
    deessGrLbl.setText ("GR", juce::dontSendNotification);
    limGrLbl.setText ("GR", juce::dontSendNotification);

    buildBackground();
    resized();                 // knobs are created above, after the implicit resized() from setSize()
    startTimerHz (30);
}

VoxChainEditor::~VoxChainEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxChainEditor::timerCallback()
{
    inMeterL.setValue (proc.inputMeterDb (0));
    inMeterR.setValue (proc.inputMeterDb (1));
    outMeterL.setValue (proc.outputMeterDb (0));
    outMeterR.setValue (proc.outputMeterDb (1));
    compGrMeter.setValue (24.f - proc.compGrDb());
    deessGrMeter.setValue (24.f - proc.deessGrDb());
    limGrMeter.setValue (24.f - proc.limGrDb());

    ledActive = proc.compGrDb() > 0.3f || proc.limGrDb() > 0.3f || proc.deessGrDb() > 0.3f;
    ledPhase += 0.1f;
    if (ledPhase > juce::MathConstants<float>::twoPi) ledPhase -= juce::MathConstants<float>::twoPi;
    eqCurve.repaint();
    repaint();
}

void VoxChainEditor::buildBackground()
{
    const int w = getWidth(), h = getHeight();
    backgroundImage = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (backgroundImage);

    juce::ColourGradient bgGrad (kBg, 0.f, 0.f, kBgSoft, 0.f, (float) h, false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    juce::Path header;
    header.startNewSubPath (0.f, 0.f);
    header.lineTo ((float) w, 0.f);
    header.lineTo ((float) w, 52.f);
    header.lineTo (0.f, 66.f);
    header.closeSubPath();
    g.setColour (kInk.withAlpha (0.55f));
    g.fillPath (header);

    const juce::Point<float> origin ((float) w - 10.f, 6.f);
    for (float yy = -20.f; yy < 200.f; yy += 9.f)
        for (float xx = (float) w - 240.f; xx < (float) w + 20.f; xx += 9.f)
        {
            const float d = origin.getDistanceFrom ({ xx, yy });
            const float t = juce::jlimit (0.f, 1.f, 1.f - d / 210.f);
            if (t <= 0.02f) continue;
            const float rr = 0.9f + 1.4f * t;
            g.setColour (kBlueGlow.withAlpha (0.10f * t));
            g.fillEllipse (xx - rr, yy - rr, rr * 2.f, rr * 2.f);
        }

    const float angles[] = { 200.f, 216.f, 232.f, 248.f, 264.f, 280.f };
    for (float ang : angles)
    {
        const float rad = juce::degreesToRadians (ang);
        const juce::Point<float> p2 (origin.x + 130.f * std::cos (rad), origin.y + 130.f * std::sin (rad));
        juce::ColourGradient sg (kBlueGlow.withAlpha (0.28f), origin.x, origin.y, kBlueGlow.withAlpha (0.f), p2.x, p2.y, false);
        g.setGradientFill (sg);
        g.drawLine (origin.x, origin.y, p2.x, p2.y, 1.2f);
    }

    // card backgrounds for every section, matching the rects resized() lays out
    auto drawCard = [&] (juce::Rectangle<int> r)
    {
        g.setColour (kPanel);
        g.fillRoundedRectangle (r.toFloat(), 10.f);
        g.setColour (kPanelEdge);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.f, 1.1f);
    };

    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (56);
    drawCard (juce::Rectangle<int> (area).removeFromTop (176));           // master/limiter
    area.removeFromTop (176 + 12);
    drawCard (juce::Rectangle<int> (area).removeFromTop (372));           // EQ
    area.removeFromTop (372 + 12);
    drawCard (juce::Rectangle<int> (area).removeFromTop (168));           // comp
    area.removeFromTop (168 + 12);
    drawCard (juce::Rectangle<int> (area).removeFromTop (168));           // deesser
    area.removeFromTop (168 + 12);
    drawCard (juce::Rectangle<int> (area).removeFromTop (150));           // sat
}

void VoxChainEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (backgroundImage, 0, 0);

    g.setColour (kText);
    g.setFont (juce::FontOptions (26.f, juce::Font::bold));
    g.drawText ("VoxChain", 18, 8, 260, 34, juce::Justification::centredLeft);

    const float pulse = 0.5f + 0.5f * std::sin (ledPhase);
    const float glowA = ledActive ? 0.9f : 0.2f + 0.15f * pulse;
    const float coreA = ledActive ? 1.0f : 0.5f + 0.25f * pulse;
    const juce::Point<float> ledPos (168.f, 25.f);
    const juce::Colour ledColour = ledActive ? kWarn : kBlueGlow;

    juce::ColourGradient halo (ledColour.withAlpha (glowA * 0.5f), ledPos.x, ledPos.y,
                               ledColour.withAlpha (0.f), ledPos.x + 15.f, ledPos.y, true);
    g.setGradientFill (halo);
    g.fillEllipse (ledPos.x - 15.f, ledPos.y - 15.f, 30.f, 30.f);
    g.setColour (ledColour.withAlpha (coreA));
    g.fillEllipse (ledPos.x - 4.f, ledPos.y - 4.f, 8.f, 8.f);
    g.setColour (juce::Colours::white.withAlpha (coreA * 0.9f));
    g.fillEllipse (ledPos.x - 1.6f, ledPos.y - 1.6f, 3.2f, 3.2f);

    g.setColour (kTextDim);
    g.setFont (juce::FontOptions (11.f));
    g.drawText (juce::String ("latency ") + juce::String (proc.getLatencySamples()) + " smp",
               getWidth() - 180, 16, 160, 14, juce::Justification::centredRight);

    auto sectionTitle = [&] (int y, const juce::String& t)
    {
        g.setColour (kTextDim);
        g.setFont (juce::FontOptions (13.f, juce::Font::bold));
        g.drawText (t, getWidth() - 168 - 150, y, 150, 18, juce::Justification::centredRight);
    };
    (void) sectionTitle;
}

void VoxChainEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (56);

    // ---- master / limiter card ----
    {
        auto content = beginCard (area, 176, "MASTER", &limOnBtn);
        auto meters = content.removeFromRight (110);
        content.removeFromRight (10);
        layoutRow (content, masterKnobs);

        auto mIn  = meters.removeFromLeft (meters.getWidth() / 3).reduced (4, 0);
        auto mOut = meters.removeFromLeft (meters.getWidth() / 2).reduced (4, 0);
        auto mGr  = meters.reduced (4, 0);
        inMeterLbl.setBounds (mIn.removeFromTop (14));
        outMeterLbl.setBounds (mOut.removeFromTop (14));
        limGrLbl.setBounds (mGr.removeFromTop (14));
        auto inPair = mIn; inMeterL.setBounds (inPair.removeFromLeft (inPair.getWidth()/2 - 1));
        inMeterR.setBounds (inPair);
        auto outPair = mOut; outMeterL.setBounds (outPair.removeFromLeft (outPair.getWidth()/2 - 1));
        outMeterR.setBounds (outPair);
        limGrMeter.setBounds (mGr);
    }

    // ---- EQ card ----
    {
        auto content = beginCard (area, 372, "EQ", &eqOnBtn);
        eqCurve.setBounds (content.removeFromTop (74));
        content.removeFromTop (8);
        layoutRow (content.removeFromTop (118), eqKnobsA);
        content.removeFromTop (6);
        layoutRow (content.removeFromTop (118), eqKnobsB);
    }

    // ---- Compressor card ----
    {
        auto content = beginCard (area, 168, "COMPRESSOR", &compOnBtn);
        auto meter = content.removeFromRight (46);
        content.removeFromRight (10);
        layoutRow (content, compKnobs);
        compGrLbl.setBounds (meter.removeFromTop (14));
        compGrMeter.setBounds (meter.reduced (10, 0));
    }

    // ---- De-esser card ----
    {
        auto content = beginCard (area, 168, "DE-ESSER", &deessOnBtn);
        auto meter = content.removeFromRight (46);
        content.removeFromRight (10);
        auto listenArea = content.removeFromRight (90);
        deessListenBtn.setBounds (listenArea.removeFromTop (22));
        content.removeFromRight (6);
        layoutRow (content, deessKnobs);
        deessGrLbl.setBounds (meter.removeFromTop (14));
        deessGrMeter.setBounds (meter.reduced (10, 0));
    }

    // ---- Saturator card ----
    {
        auto content = beginCard (area, 150, "SATURATION", &satOnBtn);
        layoutRow (content, satKnobs);
    }
}
