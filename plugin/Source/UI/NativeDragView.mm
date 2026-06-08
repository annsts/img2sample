// img2sample - Image to Audio Sample Plugin
// Copyright (C) 2026
// SPDX-License-Identifier: GPL-3.0-or-later

#if JUCE_MAC || defined(__APPLE__)

#import <Cocoa/Cocoa.h>

// Include JUCE after Cocoa to avoid Point ambiguity
#include "NativeDragView.h"

// NOTE: this target is built WITHOUT ARC (no -fobjc-arc), so object references
// must be retained manually. _mouseDownEvent is a retained property — storing the
// NSEvent raw (unretained) let it dealloc before mouseDragged: ran → use-after-free.
@interface Img2SampleDragView : NSView <NSDraggingSource>
@property (nonatomic, retain) NSEvent* mouseDownEvent;
@property (nonatomic, retain) NSURL* fileURL;
@end

@implementation Img2SampleDragView

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
    return NSDragOperationCopy;
}

// Claim mouse events even when the plugin window is not the key window.
- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
    return YES;
}

// Ensure this overlay wins hit-testing over the sibling WKWebView for points
// inside its bounds — otherwise the webview swallows mouseDown and we never drag.
- (NSView*)hitTest:(NSPoint)point
{
    return [super hitTest:point];
}

- (void)dealloc
{
    self.mouseDownEvent = nil;
    self.fileURL = nil;
    [super dealloc];
}

- (void)mouseDown:(NSEvent*)event
{
    self.mouseDownEvent = event;  // retained via property — survives until mouseDragged:
}

- (void)mouseDragged:(NSEvent*)event
{
    if (!self.fileURL)
        return;

    NSPoint down = self.mouseDownEvent ? [self.mouseDownEvent locationInWindow] : NSZeroPoint;
    NSPoint curr = [event locationInWindow];
    CGFloat dist = hypot(curr.x - down.x, curr.y - down.y);
    if (dist < 4.0)
        return;

    NSPasteboardItem* pbItem = [[NSPasteboardItem alloc] init];
    [pbItem setString:[self.fileURL absoluteString]
              forType:NSPasteboardTypeFileURL];

    NSDraggingItem* dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pbItem];

    NSImage* icon = [[NSWorkspace sharedWorkspace] iconForFile:[self.fileURL path]];
    if (!icon)
        icon = [NSImage imageNamed:NSImageNameMultipleDocuments];
    [icon setSize:NSMakeSize(32, 32)];

    [dragItem setDraggingFrame:self.bounds contents:icon];

    [self beginDraggingSessionWithItems:@[dragItem]
                                  event:event
                                 source:self];
}

@end

namespace img2sample {

struct NativeDragView::Pimpl
{
    Img2SampleDragView* view = nil;
    NSView* parentView = nil;

    void create()
    {
        if (view == nil)
        {
            view = [[Img2SampleDragView alloc] initWithFrame:NSZeroRect];
            view.wantsLayer = YES;
            view.layer.backgroundColor = [NSColor clearColor].CGColor;
        }
    }

    void destroy()
    {
        if (view != nil)
        {
            [view removeFromSuperview];
            view = nil;
            parentView = nil;
        }
    }
};

NativeDragView::NativeDragView() : pimpl (std::make_unique<Pimpl>())
{
    pimpl->create();
}

NativeDragView::~NativeDragView()
{
    pimpl->destroy();
}

void NativeDragView::attachTo (juce::Component& parent)
{
    auto* peer = parent.getPeer();
    if (peer == nullptr)
        return;

    auto* nativeHandle = (NSView*) peer->getNativeHandle();
    if (nativeHandle == nil)
        return;

    pimpl->parentView = nativeHandle;

    if (pimpl->view.superview != nativeHandle)
    {
        [pimpl->view removeFromSuperview];
        [nativeHandle addSubview:pimpl->view positioned:NSWindowAbove relativeTo:nil];
    }
}

void NativeDragView::detach()
{
    if (pimpl->view != nil)
        [pimpl->view removeFromSuperview];
    pimpl->parentView = nil;
}

void NativeDragView::setBounds (int x, int y, int w, int h)
{
    if (pimpl->view == nil || pimpl->parentView == nil)
        return;

    // JS reports coords top-left origin (CSS px). If the parent NSView is flipped
    // (JUCE's editor view is), coords map directly. If non-flipped, flip the y.
    CGFloat parentH = pimpl->parentView.bounds.size.height;
    CGFloat originY = pimpl->parentView.isFlipped ? (CGFloat) y
                                                  : parentH - (CGFloat) y - (CGFloat) h;
    NSRect frame = NSMakeRect ((CGFloat) x, originY, (CGFloat) w, (CGFloat) h);
    [pimpl->view setFrame:frame];
}

void NativeDragView::setVisible (bool visible)
{
    if (pimpl->view != nil)
        [pimpl->view setHidden:!visible];
}

void NativeDragView::setFile (const juce::File& file)
{
    if (pimpl->view != nil)
    {
        if (file.existsAsFile())
            pimpl->view.fileURL = [NSURL fileURLWithPath:
                [NSString stringWithUTF8String:file.getFullPathName().toRawUTF8()]];
        else
            pimpl->view.fileURL = nil;
    }
}

} // namespace img2sample

#else // Non-Mac

#include "NativeDragView.h"

namespace img2sample {

struct NativeDragView::Pimpl {};

NativeDragView::NativeDragView() : pimpl (std::make_unique<Pimpl>()) {}
NativeDragView::~NativeDragView() = default;
void NativeDragView::attachTo (juce::Component&) {}
void NativeDragView::detach() {}
void NativeDragView::setBounds (int, int, int, int) {}
void NativeDragView::setVisible (bool) {}
void NativeDragView::setFile (const juce::File&) {}

} // namespace img2sample

#endif
