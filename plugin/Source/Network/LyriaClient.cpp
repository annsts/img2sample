// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LyriaClient.h"

namespace img2sample {

LyriaClient::LyriaClient() {}

void LyriaClient::setApiKey (const juce::String& key)
{
    const juce::ScopedLock sl (lock);
    apiKey = key;
}

juce::String LyriaClient::getApiKey() const
{
    return apiKey;
}

bool LyriaClient::hasApiKey() const
{
    return apiKey.isNotEmpty();
}

void LyriaClient::generateAsync (const juce::String& imageBase64,
                                   const juce::String& imageMimeType,
                                   const juce::String& prompt,
                                   bool clipMode,
                                   std::function<void (GenerationResult)> callback,
                                   GenerationConfig config)
{
    auto aliveFlag = alive;
    auto cb = std::make_shared<std::function<void (GenerationResult)>> (std::move (callback));

    // Capture copies for thread safety
    auto imgB64 = imageBase64;
    auto imgMime = imageMimeType;
    auto promptCopy = prompt;

    std::thread ([this, aliveFlag, cb, imgB64, imgMime, promptCopy, clipMode, config]() {
        auto result = generateBlocking (imgB64, imgMime, promptCopy, clipMode, config);

        if (*aliveFlag)
        {
            juce::MessageManager::callAsync ([cb, result = std::move (result)]() {
                if (*cb)
                    (*cb) (std::move (result));
            });
        }
    }).detach();
}

LyriaClient::GenerationResult LyriaClient::generateBlocking (
    const juce::String& imageBase64,
    const juce::String& imageMimeType,
    const juce::String& prompt,
    bool clipMode,
    GenerationConfig config)
{
    GenerationResult result;

    juce::String currentKey;
    {
        const juce::ScopedLock sl (lock);
        currentKey = apiKey;
    }

    if (currentKey.isEmpty())
    {
        result.errorMessage = "No API key set";
        return result;
    }

    // Build prompt text
    juce::String textPrompt;
    if (prompt.isNotEmpty())
    {
        textPrompt = "Generate a short audio sample inspired by this image. Style: "
                     + prompt
                     + ". Make it a usable sample for music production — no full song structure, just a raw texture or sound element.";
    }
    else
    {
        textPrompt = "Generate a short audio sample inspired by the mood, colors, and atmosphere of this image. "
                     "Make it a raw texture or sound element usable in music production — not a full song.";
    }

    // Build JSON request body
    auto* textPart = new juce::DynamicObject();
    textPart->setProperty ("text", textPrompt);

    auto* inlineData = new juce::DynamicObject();
    inlineData->setProperty ("mimeType", imageMimeType);
    inlineData->setProperty ("data", imageBase64);

    auto* imagePart = new juce::DynamicObject();
    imagePart->setProperty ("inlineData", juce::var (inlineData));

    juce::Array<juce::var> parts;
    parts.add (juce::var (textPart));
    parts.add (juce::var (imagePart));

    auto* content = new juce::DynamicObject();
    content->setProperty ("parts", juce::var (parts));

    juce::Array<juce::var> contents;
    contents.add (juce::var (content));

    juce::Array<juce::var> modalities;
    modalities.add ("AUDIO");
    modalities.add ("TEXT");

    auto* genConfig = new juce::DynamicObject();
    genConfig->setProperty ("responseModalities", juce::var (modalities));

    if (config.temperature >= 0.0f)
        genConfig->setProperty ("temperature", (double) config.temperature);
    if (config.topK >= 0)
        genConfig->setProperty ("topK", config.topK);
    if (config.seed >= 0)
        genConfig->setProperty ("seed", config.seed);

    auto* body = new juce::DynamicObject();
    body->setProperty ("contents", juce::var (contents));
    body->setProperty ("generationConfig", juce::var (genConfig));

    auto jsonBody = juce::JSON::toString (juce::var (body));

    // Build URL
    auto model = clipMode ? juce::String ("lyria-3-clip-preview")
                          : juce::String ("lyria-3-pro-preview");

    auto urlStr = "https://generativelanguage.googleapis.com/v1beta/models/"
                  + model + ":generateContent?key=" + currentKey;

    juce::URL url (urlStr);
    url = url.withPOSTData (jsonBody);

    // Make HTTP request
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                       .withExtraHeaders ("Content-Type: application/json")
                       .withConnectionTimeoutMs (120000)
                       .withNumRedirectsToFollow (5);

    auto stream = url.createInputStream (options);

    if (stream == nullptr)
    {
        result.errorMessage = "Failed to connect to API";
        return result;
    }

    auto responseBody = stream->readEntireStreamAsString();

    // Check for HTTP errors via response content
    auto parsed = juce::JSON::parse (responseBody);

    if (parsed.isVoid())
    {
        result.errorMessage = "Invalid response from API";
        return result;
    }

    // Check for error object
    auto errorObj = parsed.getProperty ("error", {});
    if (! errorObj.isVoid())
    {
        auto errorMsg = errorObj.getProperty ("message", "Unknown API error").toString();
        result.errorMessage = errorMsg;
        return result;
    }

    // Extract audio from candidates
    auto candidates = parsed.getProperty ("candidates", {});
    if (auto* candidateArr = candidates.getArray())
    {
        for (auto& candidate : *candidateArr)
        {
            auto contentObj = candidate.getProperty ("content", {});
            auto partsArr = contentObj.getProperty ("parts", {});

            if (auto* partsArray = partsArr.getArray())
            {
                for (auto& part : *partsArray)
                {
                    auto inlineDataObj = part.getProperty ("inlineData", {});
                    if (inlineDataObj.isVoid())
                        inlineDataObj = part.getProperty ("inline_data", {});

                    if (! inlineDataObj.isVoid())
                    {
                        auto mimeType = inlineDataObj.getProperty ("mimeType", "").toString();
                        if (mimeType.isEmpty())
                            mimeType = inlineDataObj.getProperty ("mime_type", "").toString();

                        if (mimeType.startsWith ("audio/"))
                        {
                            auto data = inlineDataObj.getProperty ("data", "").toString();

                            if (data.isNotEmpty())
                            {
                                juce::MemoryOutputStream decoded;
                                if (juce::Base64::convertFromBase64 (decoded, data))
                                {
                                    result.audioData = decoded.getMemoryBlock();
                                    result.audioMimeType = mimeType;
                                    result.success = true;
                                    return result;
                                }
                                else
                                {
                                    result.errorMessage = "Failed to decode audio data";
                                    return result;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    result.errorMessage = "No audio in API response";
    return result;
}

} // namespace img2sample
