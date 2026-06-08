// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "WebBridge.h"
#include "../PluginProcessor.h"
#include "LyriaClient.h"

namespace img2sample {

WebBridge::WebBridge (Img2SampleProcessor& p) : processor (p) {}

void WebBridge::generateAudio (const juce::Array<juce::var>& args,
                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    // args[0] = imageBase64, args[1] = imageMimeType, args[2] = prompt, args[3] = clipMode (bool)
    // args[4] = optional JSON string with generation config
    if (args.size() < 4)
    {
        completion (juce::var ("error: missing arguments"));
        return;
    }

    auto imageBase64 = args[0].toString();
    auto imageMimeType = args[1].toString();
    auto prompt = args[2].toString();
    bool clipMode = (bool) args[3];

    // Parse optional generation config
    GenerationConfig genCfg;
    if (args.size() > 4 && args[4].isString())
    {
        auto cfgJson = juce::JSON::parse (args[4].toString());
        if (! cfgJson.isVoid())
        {
            auto temp = cfgJson.getProperty ("temperature", {});
            if (! temp.isVoid()) genCfg.temperature = (float) (double) temp;

            auto topK = cfgJson.getProperty ("topK", {});
            if (! topK.isVoid()) genCfg.topK = (int) topK;

            auto seed = cfgJson.getProperty ("seed", {});
            if (! seed.isVoid()) genCfg.seed = (int) seed;

        }
    }

    auto apiKey = loadApiKeyFromStorage();
    if (apiKey.isEmpty())
    {
        completion (juce::var ("error: no API key"));
        return;
    }

    auto client = std::make_shared<LyriaClient>();
    client->setApiKey (apiKey);

    auto sharedCompletion = std::make_shared<juce::WebBrowserComponent::NativeFunctionCompletion> (
        std::move (completion));

    client->generateAsync (imageBase64, imageMimeType, prompt, clipMode,
        [this, sharedCompletion, client] (LyriaClient::GenerationResult result) {
            if (result.success)
            {
                // Encode audio data as base64 to pass back to JS
                auto audioBase64 = juce::Base64::toBase64 (result.audioData.getData(),
                                                            result.audioData.getSize());

                auto* responseObj = new juce::DynamicObject();
                responseObj->setProperty ("success", true);
                responseObj->setProperty ("audioData", audioBase64);
                responseObj->setProperty ("mimeType", result.audioMimeType);

                (*sharedCompletion) (juce::var (responseObj));

                // Decode (container or raw L16 PCM) → processor + temp WAV for drag-to-DAW
                ingestAudio (result.audioData, result.audioMimeType);
            }
            else
            {
                auto* responseObj = new juce::DynamicObject();
                responseObj->setProperty ("success", false);
                responseObj->setProperty ("error", result.errorMessage);
                (*sharedCompletion) (juce::var (responseObj));
            }
        }, genCfg);
}

void WebBridge::getApiKey (const juce::Array<juce::var>&,
                            juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    completion (juce::var (loadApiKeyFromStorage()));
}

void WebBridge::setApiKey (const juce::Array<juce::var>& args,
                            juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (args.size() > 0)
        saveApiKeyToStorage (args[0].toString());
    completion (juce::var ("ok"));
}

void WebBridge::startPlayback (const juce::Array<juce::var>&,
                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    processor.startPlayback();
    completion (juce::var ("ok"));
}

void WebBridge::stopPlayback (const juce::Array<juce::var>&,
                               juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    processor.stopPlayback();
    completion (juce::var ("ok"));
}

void WebBridge::seekTo (const juce::Array<juce::var>& args,
                         juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (args.size() > 0)
        processor.seekTo ((float) args[0]);
    completion (juce::var ("ok"));
}

void WebBridge::getPlaybackPosition (const juce::Array<juce::var>&,
                                      juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    completion (juce::var (processor.getPlaybackPosition()));
}

void WebBridge::getIsPlaying (const juce::Array<juce::var>&,
                               juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    completion (juce::var (processor.isPlaying()));
}

void WebBridge::setGeneratedAudio (const juce::Array<juce::var>& args,
                                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    // args[0] = base64 audio data, args[1] = mimeType
    if (args.size() < 2)
    {
        completion (juce::var ("error: missing arguments"));
        return;
    }

    auto audioBase64 = args[0].toString();
    auto mimeType = args[1].toString();

    juce::MemoryOutputStream decoded;
    if (! juce::Base64::convertFromBase64 (decoded, audioBase64))
    {
        completion (juce::var ("error: failed to decode base64"));
        return;
    }

    if (ingestAudio (decoded.getMemoryBlock(), mimeType))
        completion (juce::var ("ok"));
    else
        completion (juce::var ("error: failed to decode audio"));
}

void WebBridge::saveToDisk (const juce::Array<juce::var>&,
                             juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (lastTempFile.existsAsFile())
        completion (juce::var (lastTempFile.getFullPathName()));
    else
        completion (juce::var ("error: no audio file"));
}

void WebBridge::dragToDAW (const juce::Array<juce::var>&,
                            juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    completion (juce::var (lastTempFile.existsAsFile() ? "ok" : "error: no audio file"));
}

void WebBridge::updateDragBtnRect (const juce::Array<juce::var>& args,
                                    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (args.size() >= 4 && onDragBtnRect)
    {
        onDragBtnRect ((int) args[0], (int) args[1], (int) args[2], (int) args[3]);
    }
    completion (juce::var ("ok"));
}

void WebBridge::saveFileDialog (const juce::Array<juce::var>&,
                                juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    if (! lastTempFile.existsAsFile())
    {
        completion (juce::var ("error: no audio file"));
        return;
    }

    auto sharedCompletion = std::make_shared<juce::WebBrowserComponent::NativeFunctionCompletion> (
        std::move (completion));

    auto chooser = std::make_shared<juce::FileChooser> (
        "Save Audio Sample",
        juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
            .getChildFile ("img2sample_" + juce::String (juce::Time::currentTimeMillis()) + ".wav"),
        "*.wav");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                          | juce::FileBrowserComponent::canSelectFiles,
        [this, sharedCompletion, chooser] (const juce::FileChooser&) {
            auto result = chooser->getResult();
            if (result != juce::File())
            {
                lastTempFile.copyFileTo (result);
                (*sharedCompletion) (juce::var (result.getFullPathName()));
            }
            else
            {
                (*sharedCompletion) (juce::var ("cancelled"));
            }
        });
}

void WebBridge::emitEvent (const juce::String& type, const juce::var& data)
{
    if (webView != nullptr)
    {
        juce::MessageManager::callAsync ([this, type, data] {
            if (webView != nullptr)
                webView->emitEventIfBrowserIsVisible (juce::Identifier (type), data);
        });
    }
}

void WebBridge::saveApiKeyToStorage (const juce::String& key)
{
    juce::PropertiesFile::Options opts;
    opts.applicationName = "img2sample";
    opts.folderName = "img2sample";
    opts.filenameSuffix = ".settings";
    juce::ApplicationProperties props;
    props.setStorageParameters (opts);
    if (auto* p = props.getUserSettings())
    {
        p->setValue ("apiKey", key);
        p->saveIfNeeded();
    }
}

juce::String WebBridge::loadApiKeyFromStorage()
{
    juce::PropertiesFile::Options opts;
    opts.applicationName = "img2sample";
    opts.folderName = "img2sample";
    opts.filenameSuffix = ".settings";
    juce::ApplicationProperties props;
    props.setStorageParameters (opts);
    if (auto* p = props.getUserSettings())
        return p->getValue ("apiKey", "");
    return {};
}

bool WebBridge::decodeRawPcm (const juce::MemoryBlock& bytes, const juce::String& mimeType,
                              juce::AudioBuffer<float>& outBuffer, double& outSampleRate)
{
    // Lyria/Gemini default output is headerless 16-bit little-endian PCM,
    // e.g. mime "audio/L16;codec=pcm;rate=24000". JUCE's createReaderFor can't
    // read it (no RIFF header), so parse it manually.
    if (bytes.getSize() < 2)
        return false;

    // Sample rate: parse "rate=NNNN" from the mime; default 24000.
    int sampleRate = 24000;
    auto rateIdx = mimeType.indexOfIgnoreCase ("rate=");
    if (rateIdx >= 0)
    {
        auto parsed = mimeType.substring (rateIdx + 5).getIntValue();
        if (parsed > 0)
            sampleRate = parsed;
    }
    outSampleRate = (double) sampleRate;

    // Channels: L16 from Lyria is mono unless "channels=N" present.
    int numChannels = 1;
    auto chIdx = mimeType.indexOfIgnoreCase ("channels=");
    if (chIdx >= 0)
    {
        auto parsed = mimeType.substring (chIdx + 9).getIntValue();
        if (parsed > 0)
            numChannels = parsed;
    }

    auto* raw = static_cast<const int16_t*> (bytes.getData());
    auto totalSamples = (int) (bytes.getSize() / sizeof (int16_t));
    auto numFrames = totalSamples / numChannels;
    if (numFrames <= 0)
        return false;

    outBuffer.setSize (numChannels, numFrames);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* dst = outBuffer.getWritePointer (ch);
        for (int i = 0; i < numFrames; ++i)
            dst[i] = (float) raw[i * numChannels + ch] / 32768.0f;
    }
    return true;
}

bool WebBridge::ingestAudio (const juce::MemoryBlock& audioBytes, const juce::String& mimeType)
{
    juce::AudioBuffer<float> buffer;
    double sampleRate = 0.0;

    // First try a real container (wav/mp3/etc). Fall back to raw L16 PCM.
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto* stream = new juce::MemoryInputStream (audioBytes, false);
    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager.createReaderFor (std::unique_ptr<juce::InputStream> (stream)));

    if (reader != nullptr)
    {
        buffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&buffer, 0, (int) reader->lengthInSamples, 0, true, true);
        sampleRate = reader->sampleRate;
    }
    else if (! decodeRawPcm (audioBytes, mimeType, buffer, sampleRate))
    {
        return false;
    }

    processor.setGeneratedAudio (buffer, sampleRate);

    // Write a proper WAV temp file for drag-to-DAW.
    auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("img2sample");
    tempDir.createDirectory();
    auto tempFile = tempDir.getChildFile (
        "sample_" + juce::String (juce::Time::currentTimeMillis()) + ".wav");

    if (auto fileStream = tempFile.createOutputStream())
    {
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wavFormat.createWriterFor (fileStream.release(),
                                       sampleRate,
                                       (unsigned int) buffer.getNumChannels(),
                                       16, {}, 0));
        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
    }

    lastTempFile = tempFile;
    return true;
}

} // namespace img2sample
