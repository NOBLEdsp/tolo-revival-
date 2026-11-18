#include "PluginEditor.h"

using namespace juce;

TololocheXEditor::TololocheXEditor (TololocheXProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    setup (boom,   "boom",   "BOOM",   Colour (0xffff5a36));
    setup (madera, "madera", "MADERA", Colour (0xffe8a34a));
    setup (clip,   "clip",   "CLIP",   Colour (0xff8fb4ff));

    setResizable (true, true);
    setResizeLimits (360, 212, 1400, 824);
    getConstrainer()->setFixedAspectRatio (1.7);
    setSize (600, 353);
}

TololocheXEditor::~TololocheXEditor() { setLookAndFeel (nullptr); }

void TololocheXEditor::setup (Knob& k, const String& id, const String& name, Colour accent)
{
    k.name = name;
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 20);
    k.slider.setColour (Slider::rotarySliderFillColourId, accent);
    k.slider.setColour (Slider::textBoxTextColourId, Colours::white.withAlpha (0.8f));
    k.slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    k.slider.setRotaryParameters (degreesToRadians (225.0f), degreesToRadians (495.0f), true);
    k.slider.setDoubleClickReturnValue (true, proc.apvts.getParameter (id)->getDefaultValue());
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void TololocheXEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff101216));
    const float h = (float) getHeight();

    g.setColour (Colours::white.withAlpha (0.55f));
    g.setFont (Font (FontOptions (h * 0.05f)));
    g.drawText ("TOLOLOCHE X", getLocalBounds().removeFromTop ((int) (h * 0.14f)).reduced ((int) (h * 0.05f), 0),
                Justification::centredLeft);

    const int colW = getWidth() / 3;
    int i = 0;
    for (auto* k : { &boom, &madera, &clip })
    {
        auto r = Rectangle<int> (i++ * colW, (int) (h * 0.17f), colW, (int) (h * 0.08f));
        g.setColour (k->slider.findColour (Slider::rotarySliderFillColourId));
        g.setFont (Font (FontOptions (h * 0.055f, Font::bold)));
        g.drawText (k->name, r, Justification::centred);
    }
}

void TololocheXEditor::resized()
{
    const int h = getHeight(), colW = getWidth() / 3;
    const int textH = jmax (12, (int) (h * 0.07f));
    int i = 0;
    for (auto* k : { &boom, &madera, &clip })
    {
        k->slider.setTextBoxStyle (Slider::TextBoxBelow, false, colW / 2, textH);
        k->slider.setBounds (i++ * colW, (int) (h * 0.26f), colW, (int) (h * 0.68f));
    }
}
