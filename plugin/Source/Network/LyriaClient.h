// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <JuceHeader.h>

namespace img2sample {

/** Optional generation config passed from the UI. */
struct GenerationConfig
{
    float temperature = -1.0f;   // < 0 means unset
    int topK = -1;               // < 0 means unset
    int seed = -1;               // < 0 means unset
};

class LyriaClient
{
public:
    struct GenerationResult
    {
        bool success = false;
        juce::String errorMessage;
        juce::MemoryBlock audioData;
        juce::String audioMimeType;
    };

    LyriaClient();
    ~LyriaClient() = default;

    void setApiKey (const juce::String& key);
    juce::String getApiKey() const;
    bool hasApiKey() const;

    void generateAsync (const juce::String& imageBase64,
                        const juce::String& imageMimeType,
                        const juce::String& prompt,
                        bool clipMode,
                        std::function<void (GenerationResult)> callback,
                        GenerationConfig config = {});

    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);

private:
    juce::String apiKey;
    juce::CriticalSection lock;

    GenerationResult generateBlocking (const juce::String& imageBase64,
                                        const juce::String& imageMimeType,
                                        const juce::String& prompt,
                                        bool clipMode,
                                        GenerationConfig config);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LyriaClient)
};

} // namespace img2sample
