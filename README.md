# img2sample

Turn an image into an audio sample, right inside your DAW. Drop an image, add an
optional text prompt (plus BPM / key / seed), and img2sample generates a short
sample you can audition and **drag straight onto a track**.

Available as **VST3**, **Audio Unit**, and **Standalone** on macOS (Apple Silicon).

## How It Works

img2sample is a JUCE C++ plugin with a WebView (HTML/JS) UI. Audio is generated
by Google's **Lyria** models via the Gemini API — there is **no local server and
no Python backend**. You supply your own Gemini API key.

1. **Open** the plugin in your DAW (or run the standalone app).
2. **Paste** a Gemini API key (stored locally — get one at [aistudio.google.com/apikey](https://aistudio.google.com/apikey)).
3. **Drop** an image (PNG/JPG) and optionally type a prompt; set BPM, key, seed.
4. **Generate** — the plugin calls the Lyria API and returns a short sample.
5. **Audition** it in the built-in player, then **drag the "drag to DAW" button**
   onto a track, or **save .wav**.

Two generation modes: **clip** (`lyria-3-clip-preview`) and **full track**
(`lyria-3-pro-preview`).

## Requirements

- macOS 12 (Monterey) or later, **Apple Silicon**
- A Google **Gemini API key** (free tier available)
- Internet connection (calls the Gemini API)

## Install (pre-built)

The build is ad-hoc signed (not notarized), so macOS quarantines it on download.

```bash
# from the unzipped folder
xattr -dr com.apple.quarantine img2sample.vst3 img2sample.component img2sample.app

cp -R img2sample.vst3      ~/Library/Audio/Plug-Ins/VST3/
cp -R img2sample.component ~/Library/Audio/Plug-Ins/Components/
# standalone:
open img2sample.app
```

Then rescan plugins in your DAW. AU may require a fresh DAW launch.

## Build from source

### Prerequisites

- CMake 3.22+
- Xcode command line tools (C++17, clang)

### Build

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE 8 is fetched automatically via CMake FetchContent. Plugin binaries are copied
to the system plugin directories after build. The web UI
(`plugin/Resources/index.html`, `bridge.js`) is bundled into each artefact's
`Resources/` folder as a post-build step.

There's also a convenience script:

```bash
bash scripts/build_plugin.sh
```

## Web build

`web/index.html` is a standalone, browser-only version of the same UI. It calls
the Gemini API directly via `fetch`, plays/downloads in-browser, and does **not**
use the JUCE bridge or native drag-to-DAW. Open it in a browser to use it.

## Architecture

See [ARCHITECTURE.md](ARCHITECTURE.md). In short:

```
plugin/
  Source/
    PluginProcessor.{h,cpp}   # JUCE audio processor — holds + plays generated audio
    PluginEditor.{h,cpp}      # Hosts the WebView, binds bridge functions, native drag overlay
    Network/
      WebBridge.{h,cpp}       # JS<->C++ bridge: generate, playback, save, drag, audio decode
      LyriaClient.{h,cpp}     # Gemini/Lyria HTTP client
    UI/
      NativeDragView.{h,mm}   # macOS NSView overlay that starts the drag-to-DAW session
  Resources/
    index.html, bridge.js     # WebView UI (bundled into the plugin)
web/
  index.html                  # Standalone browser build
scripts/
  build_plugin.sh             # CMake build helper
  package.sh                  # Bundle the built artefacts into a zip
```

## API key & privacy

- The key is stored locally via JUCE `ApplicationProperties` (plugin) or
  `localStorage` (web). It is sent only to Google's Gemini endpoint.
- Images and prompts you generate from are sent to the Gemini API.

## License

Licensed under the [GNU General Public License v3.0](LICENSE).
JUCE is used under its GPLv3 license.

## Known limitations

- macOS / Apple Silicon only; builds are ad-hoc signed (Gatekeeper bypass needed).
- Requires your own Gemini API key and network access.
- Drag-to-DAW is macOS-only (native NSView overlay).
