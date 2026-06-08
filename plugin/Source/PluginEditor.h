// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Network/WebBridge.h"
#include "UI/NativeDragView.h"

namespace img2sample {

class Img2SampleEditor : public juce::AudioProcessorEditor,
                          public juce::DragAndDropContainer
{
public:
    explicit Img2SampleEditor (Img2SampleProcessor&);
    ~Img2SampleEditor() override;

    void resized() override;
    void paint (juce::Graphics&) override;

    // Called from WebBridge when JS reports drag button position
    void updateDragButtonRect (int x, int y, int w, int h);

private:
    Img2SampleProcessor& processorRef;

    // WebView bridge
    WebBridge webBridge;

    // WebBrowserComponent — created with options
    juce::WebBrowserComponent webView;

    // Aspect ratio constrainer
    juce::ComponentBoundsConstrainer constrainer;

    // Native macOS drag view positioned over the HTML drag button
    NativeDragView nativeDragView;

    // Resource provider for serving HTML/CSS/JS
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    // Create WebBrowserComponent options with all bridge functions bound
    juce::WebBrowserComponent::Options createWebViewOptions();

    // MIME type helper
    static const char* getMimeForExtension (const juce::String& extension);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Img2SampleEditor)
};

} // namespace img2sample
