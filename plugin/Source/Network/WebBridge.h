// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <JuceHeader.h>

namespace img2sample {

class Img2SampleProcessor;

class WebBridge
{
public:
    explicit WebBridge (Img2SampleProcessor& p);
    ~WebBridge() = default;

    // Bridge functions called from JS via withNativeFunction
    void generateAudio (const juce::Array<juce::var>& args,
                        juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void getApiKey (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void setApiKey (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void startPlayback (const juce::Array<juce::var>& args,
                        juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void stopPlayback (const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void seekTo (const juce::Array<juce::var>& args,
                 juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void getPlaybackPosition (const juce::Array<juce::var>& args,
                              juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void setGeneratedAudio (const juce::Array<juce::var>& args,
                            juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void saveToDisk (const juce::Array<juce::var>& args,
                     juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void dragToDAW (const juce::Array<juce::var>& args,
                    juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void updateDragBtnRect (const juce::Array<juce::var>& args,
                            juce::WebBrowserComponent::NativeFunctionCompletion completion);

    void saveFileDialog (const juce::Array<juce::var>& args,
                         juce::WebBrowserComponent::NativeFunctionCompletion completion);

    juce::File getDragFile() const { return lastTempFile; }

    void getIsPlaying (const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion completion);

    // Emit event to WebView from C++
    void emitEvent (const juce::String& type, const juce::var& data);

    // Set the webview reference (called after webview is constructed)
    void setWebView (juce::WebBrowserComponent* wv) { webView = wv; }

    // Callback when JS reports drag button position
    std::function<void (int, int, int, int)> onDragBtnRect;

private:
    Img2SampleProcessor& processor;
    juce::WebBrowserComponent* webView = nullptr;

    // Persistent storage for API key
    void saveApiKeyToStorage (const juce::String& key);
    juce::String loadApiKeyFromStorage();

    // Decode audio bytes (container OR raw L16 PCM from Lyria) into a buffer,
    // push to processor, and write a temp WAV for drag-to-DAW. Returns true on success.
    bool ingestAudio (const juce::MemoryBlock& audioBytes, const juce::String& mimeType);

    // Parse raw audio bytes as headerless L16 PCM into a float buffer.
    // sampleRate/numChannels are derived from the mime (e.g. "audio/L16;rate=24000").
    static bool decodeRawPcm (const juce::MemoryBlock& bytes, const juce::String& mimeType,
                              juce::AudioBuffer<float>& outBuffer, double& outSampleRate);

    // Temp file for drag-to-DAW
    juce::File lastTempFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebBridge)
};

} // namespace img2sample
