#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_gui_extra/juce_gui_extra.h>

static void savePng (juce::Component& c, const char* path, int w, int h)
{
    c.setSize (w, h);
    juce::Image img (juce::Image::ARGB, w, h, true);
    juce::Graphics g (img);
    c.paintEntireComponent (g, false);
    juce::PNGImageFormat png;
    juce::File out (path);
    out.deleteFile();
    juce::FileOutputStream fos (out);
    png.writeImageToStream (img, fos);
    std::printf ("wrote %s (%dx%d)\n", path, w, h);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    VoxChainProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);

    // feed some audio through so meters/curve have something to show
    juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi;
    for (int blk = 0; blk < 100; ++blk)
    {
        for (int c = 0; c < 2; ++c)
        {
            auto* w = buf.getWritePointer (c);
            for (int i = 0; i < 512; ++i)
            {
                double t = (blk * 512 + i) / 48000.0;
                w[i] = (float) (0.6 * std::sin (2 * juce::MathConstants<double>::pi * 200.0 * t)
                               + 0.3 * std::sin (2 * juce::MathConstants<double>::pi * 6500.0 * t));
            }
        }
        p.processBlock (buf, midi);
    }

    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    savePng (*ed, "/home/claude/voxchain/tests/render_editor.png", 760, 1180);
    return 0;
}
