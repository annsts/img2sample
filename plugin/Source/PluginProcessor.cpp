// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace img2sample {

Img2SampleProcessor::Img2SampleProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

Img2SampleProcessor::~Img2SampleProcessor()
{
}

void Img2SampleProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void Img2SampleProcessor::releaseResources()
{
}

bool Img2SampleProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto outSet = layouts.getMainOutputChannelSet();
    if (outSet != juce::AudioChannelSet::mono()
     && outSet != juce::AudioChannelSet::stereo())
        return false;

    auto inSet = layouts.getMainInputChannelSet();
    if (! inSet.isDisabled() && inSet != outSet)
        return false;

    return true;
}

void Img2SampleProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear output
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Play generated audio if active
    if (playing)
    {
        const juce::SpinLock::ScopedTryLockType lock (bufferLock);

        if (lock.isLocked() && generatedBuffer.getNumSamples() > 0)
        {
            int pos = playbackPosition.load();
            int samplesToPlay = buffer.getNumSamples();
            int availableSamples = generatedBuffer.getNumSamples() - pos;

            if (availableSamples <= 0)
            {
                playing = false;
                playbackPosition = 0;
                return;
            }

            int numToCopy = juce::jmin (samplesToPlay, availableSamples);
            int numChannels = juce::jmin (buffer.getNumChannels(),
                                           generatedBuffer.getNumChannels());

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.copyFrom (ch, 0, generatedBuffer, ch, pos, numToCopy);

            // If mono source, copy to both channels
            if (generatedBuffer.getNumChannels() == 1 && buffer.getNumChannels() > 1)
                buffer.copyFrom (1, 0, generatedBuffer, 0, pos, numToCopy);

            playbackPosition = pos + numToCopy;

            // Feed FFT for audio-reactive visuals
            const float* monoData = buffer.getReadPointer (0);
            for (int i = 0; i < numToCopy; ++i)
            {
                fftData[fftFillIndex++] = monoData[i];
                if (fftFillIndex >= fftSize)
                {
                    // Zero the second half (imaginary part)
                    std::fill (fftData + fftSize, fftData + fftSize * 2, 0.0f);
                    fft.performFrequencyOnlyForwardTransform (fftData);

                    // Copy magnitude to frequencyData with smoothing
                    for (int j = 0; j < frequencyDataSize; ++j)
                    {
                        float newVal = fftData[j] / (float) fftSize;
                        frequencyData[j] = frequencyData[j] * 0.7f + newVal * 0.3f;
                    }

                    fftFillIndex = 0;
                }
            }
        }
    }
}

juce::AudioProcessorEditor* Img2SampleProcessor::createEditor()
{
    return new Img2SampleEditor (*this);
}

void Img2SampleProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("Img2SampleState");
    state.setProperty ("version", 1, nullptr);

    juce::MemoryOutputStream stream (destData, true);
    state.writeToStream (stream);
}

void Img2SampleProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto state = juce::ValueTree::readFromData (data, (size_t) sizeInBytes);

    if (state.isValid() && state.hasType ("Img2SampleState"))
    {
        // Restore state
    }
}

void Img2SampleProcessor::setGeneratedAudio (const juce::AudioBuffer<float>& buffer,
                                              double sr)
{
    const juce::SpinLock::ScopedLockType lock (bufferLock);

    double hostRate = getSampleRate();
    if (hostRate <= 0.0)
        hostRate = 44100.0;

    // Resample if source rate differs from host rate
    if (sr > 0.0 && std::abs (sr - hostRate) > 1.0)
    {
        double ratio = sr / hostRate;
        int newLength = (int) ((double) buffer.getNumSamples() / ratio);
        int numChannels = buffer.getNumChannels();

        generatedBuffer.setSize (numChannels, newLength);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::LagrangeInterpolator interpolator;
            interpolator.reset();
            interpolator.process (ratio,
                                  buffer.getReadPointer (ch),
                                  generatedBuffer.getWritePointer (ch),
                                  newLength);
        }
    }
    else
    {
        generatedBuffer.makeCopyOf (buffer);
    }

    generatedSampleRate = hostRate;
    playbackPosition = 0;

    // Reset FFT state
    std::fill (frequencyData, frequencyData + frequencyDataSize, 0.0f);
    fftFillIndex = 0;
}

void Img2SampleProcessor::seekTo (float normalisedPos)
{
    const juce::SpinLock::ScopedTryLockType lock (bufferLock);
    if (lock.isLocked() && generatedBuffer.getNumSamples() > 0)
    {
        int pos = (int) (juce::jlimit (0.0f, 1.0f, normalisedPos)
                         * (float) generatedBuffer.getNumSamples());
        playbackPosition = pos;
    }
}

void Img2SampleProcessor::startPlayback()
{
    playing = true;
}

void Img2SampleProcessor::stopPlayback()
{
    playing = false;
}

float Img2SampleProcessor::getPlaybackPosition() const
{
    const juce::SpinLock::ScopedTryLockType lock (bufferLock);

    if (! lock.isLocked() || generatedBuffer.getNumSamples() == 0)
        return 0.0f;

    return (float) playbackPosition.load() / (float) generatedBuffer.getNumSamples();
}

} // namespace img2sample

// --- JUCE Plugin Entry Point ---
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new img2sample::Img2SampleProcessor();
}
