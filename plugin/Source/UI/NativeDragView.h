// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <JuceHeader.h>

namespace img2sample {

// Creates a native NSView overlay for drag-to-DAW that sits above WKWebView.
// On non-Mac platforms this is a no-op.
class NativeDragView
{
public:
    NativeDragView();
    ~NativeDragView();

    // Attach to a JUCE component's native view hierarchy
    void attachTo (juce::Component& parent);
    void detach();

    // Position the drag area (in parent component coordinates)
    void setBounds (int x, int y, int w, int h);
    void setVisible (bool visible);

    // Set the file to drag
    void setFile (const juce::File& file);

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace img2sample
