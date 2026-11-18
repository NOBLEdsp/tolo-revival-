#pragma once
#include <JuceHeader.h>

// ---------- Utilidades DSP pequeñas ----------
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void set (double fs, double f, double q, bool highpass)
    {
        const double w0 = juce::MathConstants<double>::twoPi * f / fs;
        const double c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
        const double a0 = 1.0 + al;
        if (highpass) { b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0; }
        else          { b0 = (1 - c) / 2 / a0; b1 =  (1 - c) / a0; b2 = b0; }
        a1 = -2.0 * c / a0;
        a2 = (1.0 - al) / a0;
    }
    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }
};

// Pasabanda de 4º orden (2 HP + 2 LP Butterworth)
struct BandPass
{
    Biquad hp[2], lp[2];
    void prepare (double fs, double lo, double hi)
    {
        for (auto& h : hp) h.set (fs, lo, 0.70710678, true);
        for (auto& l : lp) l.set (fs, hi, 0.70710678, false);
    }
    inline float process (float x)
    {
        double y = x;
        y = hp[0].process (y); y = hp[1].process (y);
        y = lp[0].process (y); y = lp[1].process (y);
        return (float) y;
    }
    void reset() { for (auto& h : hp) h.reset(); for (auto& l : lp) l.reset(); }
};

// Seguidor de envolvente de picos
struct Env
{
    float v = 0, aAtt = 0.1f, aRel = 0.001f;
    void prepare (double fs, float attMs, float relMs)
    {
        aAtt = 1.0f - std::exp (-1.0f / (attMs * 0.001f * (float) fs));
        aRel = 1.0f - std::exp (-1.0f / (relMs * 0.001f * (float) fs));
    }
    inline float process (float x)
    {
        x = std::abs (x);
        v += (x - v) * (x > v ? aAtt : aRel);
        return v;
    }
    void reset() { v = 0; }
};

// ---------- Procesador ----------
class TololocheXProcessor : public juce::AudioProcessor
{
public:
    TololocheXProcessor();
    ~TololocheXProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Tololoche X"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // BOOM (45-100 Hz): sub-octava + armónicos sintetizados
    BandPass boomBP;
    Env boomEnv;
    Biquad subLP[2], subHP;
    bool schmittHigh = false, flip = false;

    // MADERA (250-2k): 3 sub-bandas con armónicos tipo marimba
    static constexpr int numBands = 3;
    struct Band { BandPass bp; Env env; };
    Band bands[2][numBands];
    Biquad maderaLP[2][2];

    // CLIP final con sobremuestreo
    juce::dsp::Oversampling<float> oversampling { 2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    float gSmooth = 0.01f;

    juce::SmoothedValue<float> boomS, maderaS;
    std::atomic<float>* boomParam = nullptr;
    std::atomic<float>* maderaParam = nullptr;
    std::atomic<float>* clipParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TololocheXProcessor)
};
