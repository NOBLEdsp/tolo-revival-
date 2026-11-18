#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout TololocheXProcessor::createLayout()
{
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; });

    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "boom", 1 },   "Boom",
           NormalisableRange<float> (0.0f, 1.0f), 0.40f, pct));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "madera", 1 }, "Madera",
           NormalisableRange<float> (0.0f, 1.0f), 0.30f, pct));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "clip", 1 },   "Clip",
           NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    return l;
}

TololocheXProcessor::TololocheXProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  AudioChannelSet::stereo(), true)
          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    boomParam   = apvts.getRawParameterValue ("boom");
    maderaParam = apvts.getRawParameterValue ("madera");
    clipParam   = apvts.getRawParameterValue ("clip");
}

bool TololocheXProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void TololocheXProcessor::prepareToPlay (double fs, int blockSize)
{
    boomBP.prepare (fs, 45.0, 100.0);
    boomEnv.prepare (fs, 1.0f, 40.0f);
    for (auto& b : subLP) b.set (fs, 60.0, 0.70710678, false);
    subHP.set (fs, 25.0, 0.70710678, true);

    // 250-550 / 550-1100 / 1100-2000 Hz
    const double edges[numBands + 1] = { 250.0, 550.0, 1100.0, 2000.0 };
    for (int c = 0; c < 2; ++c)
    {
        for (int k = 0; k < numBands; ++k)
        {
            bands[c][k].bp.prepare (fs, edges[k], edges[k + 1]);
            bands[c][k].env.prepare (fs, 0.2f, 10.0f);
        }
        for (auto& f : maderaLP[c]) f.set (fs, 5000.0, 0.70710678, false);
    }

    boomS.reset (fs, 0.02);   boomS.setCurrentAndTargetValue (*boomParam);
    maderaS.reset (fs, 0.02); maderaS.setCurrentAndTargetValue (*maderaParam);

    oversampling.initProcessing ((size_t) blockSize);
    oversampling.reset();
    setLatencySamples ((int) oversampling.getLatencyInSamples());
    gSmooth = jmax (0.01f, *clipParam * 4.0f);

    boomBP.reset(); boomEnv.reset(); subHP.reset();
    for (auto& b : subLP) b.reset();
    for (int c = 0; c < 2; ++c)
    {
        for (auto& b : bands[c]) { b.bp.reset(); b.env.reset(); }
        for (auto& f : maderaLP[c]) f.reset();
    }
}

void TololocheXProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    auto* ch0 = buffer.getWritePointer (0);
    auto* ch1 = buffer.getWritePointer (1);

    boomS.setTargetValue (*boomParam);
    maderaS.setTargetValue (*maderaParam);

    for (int i = 0; i < n; ++i)
    {
        const float boomG = boomS.getNextValue();
        const float madG  = maderaS.getNextValue();
        float in[2] = { ch0[i], ch1[i] };

        // ============ BOOM (mono, estilo Epicenter) ============
        const float mono = 0.5f * (in[0] + in[1]);
        const float b = boomBP.process (mono);
        const float e = boomEnv.process (b);

        float sub = 0.0f, harm = 0.0f;
        if (e > 0.002f)
        {
            // Divisor de frecuencia /2 (flip-flop con histéresis) -> octava abajo
            const float thr = 0.3f * e;
            if (! schmittHigh && b > thr)       { schmittHigh = true; flip = ! flip; }
            else if (schmittHigh && b < -thr)   { schmittHigh = false; }
            sub = flip ? e : -e;

            // Armónicos (2º y 3º) con Chebyshev normalizado por la envolvente
            const float x  = jlimit (-1.0f, 1.0f, b / e);
            const float x2 = x * x;
            const float t2 = 2.0f * x2 - 1.0f;
            const float t3 = x * (4.0f * x2 - 3.0f);
            harm = e * (0.6f * t2 + 0.4f * t3);
        }
        // Suavizar el cuadrado a casi-seno y quitar subgraves peligrosos (<25 Hz)
        double s = subLP[1].process (subLP[0].process ((double) sub));
        s = subHP.process (s);

        const float boomOut = boomG * ((float) s + 0.5f * harm);

        // ============ MADERA (por canal, 3 sub-bandas) ============
        float out[2];
        for (int c = 0; c < 2; ++c)
        {
            float w = 0.0f;
            for (int k = 0; k < numBands; ++k)
            {
                auto& bd = bands[c][k];
                const float bb = bd.bp.process (in[c]);
                const float ee = bd.env.process (bb);
                if (ee > 1.0e-4f)
                {
                    const float x  = jlimit (-1.0f, 1.0f, bb / ee);
                    const float x2 = x * x;
                    const float t2 = 2.0f * x2 - 1.0f;
                    const float t3 = x * (4.0f * x2 - 3.0f);
                    const float t4 = 8.0f * x2 * x2 - 8.0f * x2 + 1.0f;
                    // 2ª y sobre todo 4ª (doble octava) = carácter de barra de marimba
                    w += ee * (0.45f * t2 + 0.20f * t3 + 0.70f * t4);
                }
            }
            const double wl = maderaLP[c][1].process (maderaLP[c][0].process ((double) w));
            out[c] = in[c] + boomOut + madG * 0.8f * (float) wl;
        }
        ch0[i] = out[0];
        ch1[i] = out[1];
    }

    // ============ CLIP: tanh normalizado a 0 dBFS, 2x oversampling ============
    dsp::AudioBlock<float> block (buffer);
    auto up = oversampling.processSamplesUp (block);
    const float gTarget = jmax (0.01f, *clipParam * 4.0f);
    for (size_t i = 0; i < up.getNumSamples(); ++i)
    {
        gSmooth += 0.002f * (gTarget - gSmooth);
        const float norm = 1.0f / std::tanh (gSmooth);
        for (size_t c = 0; c < up.getNumChannels(); ++c)
        {
            auto* p = up.getChannelPointer (c);
            p[i] = std::tanh (gSmooth * p[i]) * norm;
        }
    }
    oversampling.processSamplesDown (block);
}

AudioProcessorEditor* TololocheXProcessor::createEditor() { return new TololocheXEditor (*this); }

void TololocheXProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void TololocheXProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TololocheXProcessor(); }
