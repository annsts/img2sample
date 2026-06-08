// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <JuceHeader.h>

namespace img2sample {

class Img2SampleProcessor : public juce::AudioProcessor
{
public:
    Img2SampleProcessor();
    ~Img2SampleProcessor() override;

    // --- AudioProcessor interface ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- img2sample-specific ---

    // Current generated sample for playback
    void setGeneratedAudio (const juce::AudioBuffer<float>& buffer, double sr);
    void seekTo (float normalisedPos);
    void startPlayback();
    void stopPlayback();
    bool isPlaying() const { return playing; }
    float getPlaybackPosition() const;

    // Analyser for audio-reactive visuals
    const float* getFrequencyData() const { return frequencyData; }
    int getFrequencyDataSize() const { return frequencyDataSize; }

private:
    // Playback state — guarded by bufferLock for thread-safe access
    // between the audio thread (processBlock) and message thread (setGeneratedAudio)
    mutable juce::SpinLock bufferLock;
    juce::AudioBuffer<float> generatedBuffer;
    double generatedSampleRate = 44100.0;
    std::atomic<bool> playing { false };
    std::atomic<int> playbackPosition { 0 };

    // FFT for audio-reactive visuals
    static constexpr int fftOrder = 8;  // 256 samples
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int frequencyDataSize = fftSize / 2;
    juce::dsp::FFT fft { fftOrder };
    float fftData[fftSize * 2] = {};
    float frequencyData[frequencyDataSize] = {};
    int fftFillIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Img2SampleProcessor)
};

} // namespace img2sample
