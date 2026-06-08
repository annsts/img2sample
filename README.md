# img2sample

A DAW plugin that translates images into sound. Drop a photo, painting, or texture — its colors, mood, and energy become an audio sample you can drag straight onto a track.

The interface is built around element and mood tags — tap "drums" and the prompt expands to "raw drum pattern, no melodic instruments, no vocals, percussion only." This makes it easy to generate isolated stems (vocals, bass, guitar, synth pads, textures, foley) rather than full mixes. All tags are editable.

https://github.com/user-attachments/assets/96572499-cf83-4259-a61c-3d02857124fe

*Browser preview — the same UI runs inside the DAW plugin as a WebView.*

Available as **VST3**, **Audio Unit**, and **Standalone** on macOS (Apple Silicon). Powered by Google's [Lyria](https://deepmind.google/models/lyria/) models via the Gemini API.

## Why

Musicians already think visually — mood boards, album art, color palettes. img2sample makes that connection literal: feed the model a reference image and it becomes part of the prompt. Instead of describing a sound in words, you show it a picture and shape the output with tags. It's a faster, more intuitive way to start a track — especially for producers who think in texture and atmosphere before they think in notes.

## How It Works

1. **Open** the plugin in your DAW (or run standalone).
2. **Paste** a Gemini API key (get one at [aistudio.google.com/apikey](https://aistudio.google.com/apikey)).
3. **Drop** an image and optionally type a prompt; pick element/mood tags; set BPM, key, seed.
4. **Generate** — returns an audio sample.
5. **Drag** onto a track or **save .wav**.

Two generation modes: **clip** and **full track**.

## Requirements

- macOS 12+ (Apple Silicon)
- [Gemini API key](https://aistudio.google.com/apikey) (free tier available)
- Internet connection

## Install

Ad-hoc signed (not notarized) — remove quarantine after download:

```bash
xattr -dr com.apple.quarantine img2sample.vst3 img2sample.component img2sample.app

cp -R img2sample.vst3      ~/Library/Audio/Plug-Ins/VST3/
cp -R img2sample.component ~/Library/Audio/Plug-Ins/Components/
open img2sample.app  # standalone
```

Rescan plugins in your DAW. AU may need a fresh DAW launch.

## Build from Source

Requires CMake 3.22+ and Xcode command line tools (C++17).

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE 8 is fetched via CMake FetchContent. Plugin binaries are copied to system plugin directories after build. Or use `bash scripts/build_plugin.sh`.

## Architecture

See [ARCHITECTURE.md](ARCHITECTURE.md).

```
plugin/
  Source/
    PluginProcessor.{h,cpp}   # Audio processor — holds + plays generated audio
    PluginEditor.{h,cpp}      # WebView host, bridge bindings, native drag overlay
    Network/
      WebBridge.{h,cpp}       # JS ↔ C++ bridge
      LyriaClient.{h,cpp}     # Gemini/Lyria HTTP client
    UI/
      NativeDragView.{h,mm}   # macOS drag-to-DAW overlay
  Resources/
    index.html, bridge.js     # WebView UI
web/
  index.html                  # Standalone browser version
```

## Privacy

Your API key is stored locally. Images and prompts are sent to the Gemini API only.

## License

[GPLv3](LICENSE). JUCE is used under GPLv3.
