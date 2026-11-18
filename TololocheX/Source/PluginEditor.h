#pragma once
#include "PluginProcessor.h"

// Look & feel plano y moderno. El diseño visual final se trabaja aquí.
struct ModernLook : public juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float thick = r * 0.11f, arcR = r - thick;
        const float ang = start + pos * (end - start);

        juce::Path track, val;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
        val.addCentredArc   (c.x, c.y, arcR, arcR, 0.0f, start, ang, true);
        juce::PathStrokeType st (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        g.setColour (juce::Colour (0xff262a31));
        g.strokePath (track, st);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (val, st);

        const float ir = arcR - thick * 1.6f;
        g.setColour (juce::Colour (0xff15171b));
        g.fillEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);

        juce::Path p;
        p.addRoundedRectangle (-thick * 0.4f, -ir * 0.85f, thick * 0.8f, ir * 0.45f, thick * 0.4f);
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.fillPath (p, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    juce::Font getLabelFont (juce::Label& l) override
    {
        return juce::Font (juce::FontOptions (juce::jmax (8.0f, (float) l.getHeight() * 0.62f)));
    }
};

class TololocheXEditor : public juce::AudioProcessorEditor
{
public:
    explicit TololocheXEditor (TololocheXProcessor&);
    ~TololocheXEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    TololocheXProcessor& proc;
    ModernLook look;
    Knob boom, madera, clip;

    void setup (Knob&, const juce::String& id, const juce::String& name, juce::Colour accent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TololocheXEditor)
};
