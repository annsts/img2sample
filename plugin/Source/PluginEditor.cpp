// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"

namespace img2sample {

// ── Local dev server for hot-reload during development ──
#if defined(DEV) && DEV
static constexpr bool devMode = true;
#else
static constexpr bool devMode = false;
#endif
static constexpr const char* localDevServerAddress = "http://localhost:8080";

// ── Macro to bind bridge functions ──
#define BIND_BRIDGE(name) \
    .withNativeFunction (juce::Identifier { #name }, \
        [this] (const juce::Array<juce::var>& args, \
                juce::WebBrowserComponent::NativeFunctionCompletion c) { \
            webBridge.name (args, std::move (c)); \
        })

Img2SampleEditor::Img2SampleEditor (Img2SampleProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      webBridge (p),
      webView (createWebViewOptions())
{
    setSize (560, 760);

    // Wire the bridge to the webview for emitting events
    webBridge.setWebView (&webView);

    // Wire drag button rect updates from JS
    webBridge.onDragBtnRect = [this] (int x, int y, int w, int h) {
        updateDragButtonRect (x, y, w, h);
    };

    // Add webview as child
    addAndMakeVisible (webView);

    // Attach native drag view once the peer is available
    nativeDragView.setVisible (false);

    // Load the URL
    if (devMode)
    {
        webView.goToURL (localDevServerAddress);
    }
    else
    {
        auto url = webView.getResourceProviderRoot();
        webView.goToURL (url);
    }

    // Set resizable with aspect ratio
    setResizable (true, true);
    constrainer.setFixedAspectRatio (560.0 / 760.0);
    setConstrainer (&constrainer);
}

Img2SampleEditor::~Img2SampleEditor()
{
    nativeDragView.detach();
    webBridge.setWebView (nullptr);
}

void Img2SampleEditor::updateDragButtonRect (int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
    {
        nativeDragView.setVisible (false);
        return;
    }

    // Attach to native view hierarchy if not already
    nativeDragView.attachTo (*this);

    // Update the file to drag
    nativeDragView.setFile (webBridge.getDragFile());

    nativeDragView.setBounds (x, y, w, h);
    nativeDragView.setVisible (true);
}

juce::WebBrowserComponent::Options Img2SampleEditor::createWebViewOptions()
{
    return juce::WebBrowserComponent::Options{}
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withNativeIntegrationEnabled()
        .withResourceProvider (
            [this] (const auto& url) { return getResource (url); },
            juce::URL { localDevServerAddress }.getOrigin())
        BIND_BRIDGE (generateAudio)
        BIND_BRIDGE (getApiKey)
        BIND_BRIDGE (setApiKey)
        BIND_BRIDGE (startPlayback)
        BIND_BRIDGE (stopPlayback)
        BIND_BRIDGE (seekTo)
        BIND_BRIDGE (getPlaybackPosition)
        BIND_BRIDGE (getIsPlaying)
        BIND_BRIDGE (setGeneratedAudio)
        BIND_BRIDGE (saveToDisk)
        BIND_BRIDGE (dragToDAW)
        BIND_BRIDGE (updateDragBtnRect)
        BIND_BRIDGE (saveFileDialog);
}

void Img2SampleEditor::paint (juce::Graphics&)
{
    // WebView fills the entire area — no custom painting needed
}

void Img2SampleEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource> Img2SampleEditor::getResource (const juce::String& url)
{
    if (devMode)
        return std::nullopt; // Falls through to localhost in dev mode

    // Determine which resource to serve
    auto resourceToRetrieve = (url == "/" || url.isEmpty())
        ? juce::String ("index.html")
        : url.fromFirstOccurrenceOf ("/", false, false);

    // Look for the resource file relative to the app bundle
#if JUCE_MAC
    auto resourceFilesRoot = juce::File::getSpecialLocation (
        juce::File::currentApplicationFile).getChildFile ("Contents/Resources/");
#elif JUCE_WINDOWS
    auto resourceFilesRoot = juce::File::getSpecialLocation (
        juce::File::currentApplicationFile).getParentDirectory()
            .getParentDirectory().getChildFile ("Resources/");
#else
    auto resourceFilesRoot = juce::File::getSpecialLocation (
        juce::File::currentApplicationFile).getParentDirectory()
            .getChildFile ("Resources/");
#endif

    auto resourceFile = resourceFilesRoot.getChildFile (resourceToRetrieve);

    if (auto stream = resourceFile.createInputStream())
    {
        auto extension = resourceToRetrieve.fromLastOccurrenceOf (".", false, false);
        auto mimeType = getMimeForExtension (extension);

        // Read file into byte vector
        auto sizeInBytes = (size_t) stream->getTotalLength();
        std::vector<std::byte> data (sizeInBytes);
        stream->setPosition (0);
        stream->read (data.data(), data.size());

        return juce::WebBrowserComponent::Resource { std::move (data), juce::String (mimeType) };
    }

    DBG ("Resource not found: " + resourceToRetrieve);
    return std::nullopt;
}

const char* Img2SampleEditor::getMimeForExtension (const juce::String& extension)
{
    auto ext = extension.toLowerCase();

    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css")                  return "text/css";
    if (ext == "js")                   return "text/javascript";
    if (ext == "json")                 return "application/json";
    if (ext == "png")                  return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "svg")                  return "image/svg+xml";
    if (ext == "ico")                  return "image/vnd.microsoft.icon";
    if (ext == "woff2")                return "font/woff2";
    if (ext == "woff")                 return "font/woff";
    if (ext == "ttf")                  return "font/ttf";
    if (ext == "txt")                  return "text/plain";
    if (ext == "map")                  return "application/json";
    if (ext == "wav")                  return "audio/wav";
    if (ext == "mp3")                  return "audio/mpeg";

    return "application/octet-stream";
}

} // namespace img2sample
