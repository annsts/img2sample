#!/usr/bin/env bash
# img2sample - Image to Audio Sample Plugin
# Copyright (C) 2026
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build a macOS .pkg installer that copies the plugin formats into the standard
# system locations so DAWs (Ableton, Logic, etc.) pick them up on next scan:
#   img2sample.vst3      -> /Library/Audio/Plug-Ins/VST3
#   img2sample.component -> /Library/Audio/Plug-Ins/Components   (Audio Unit)
#   img2sample.app       -> /Applications                        (standalone)
#
# The .pkg is unsigned/not-notarized: right-click the .pkg and choose "Open"
# to bypass Gatekeeper on the target Mac.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
ART="$PROJECT_DIR/build/Img2Sample_artefacts/Release"
# Version: $VERSION env override (CI passes the tag) > current git tag > 0.0.0.
# Leading "v" is stripped (v0.1.0 -> 0.1.0).
VERSION="${VERSION:-$(git -C "$PROJECT_DIR" describe --tags --abbrev=0 2>/dev/null || echo 0.0.0)}"
VERSION="${VERSION#v}"
DIST="$PROJECT_DIR/dist"
ROOT="$DIST/pkg-root"           # staged file tree, installed at "/"
COMP="$DIST/pkg-components"     # intermediate component pkgs
OUT="$DIST/img2sample-${VERSION}.pkg"
IDENT="com.img2sample"

echo "================================================"
echo "  img2sample — macOS installer (.pkg) v$VERSION"
echo "================================================"

if [ ! -d "$ART" ]; then
    echo "No build found at $ART — running build first..."
    bash "$SCRIPT_DIR/build_plugin.sh"
fi

rm -rf "$ROOT" "$COMP"
mkdir -p "$ROOT" "$COMP"

VST3="$ART/VST3/img2sample.vst3"
AU="$ART/AU/img2sample.component"
APP="$ART/Standalone/img2sample.app"

# Stage each format into its install location under the pkg root.
if [ -d "$VST3" ]; then
    mkdir -p "$ROOT/Library/Audio/Plug-Ins/VST3"
    cp -R "$VST3" "$ROOT/Library/Audio/Plug-Ins/VST3/"
fi
if [ -d "$AU" ]; then
    mkdir -p "$ROOT/Library/Audio/Plug-Ins/Components"
    cp -R "$AU" "$ROOT/Library/Audio/Plug-Ins/Components/"
fi
if [ -d "$APP" ]; then
    mkdir -p "$ROOT/Applications"
    cp -R "$APP" "$ROOT/Applications/"
fi

# Re-sign deep ad-hoc so each bundle's seal is valid after the copy.
find "$ROOT" -name .DS_Store -delete
# Strip AppleDouble (._*) sidecars that cp can leave behind.
find "$ROOT" -name '._*' -delete
dot_clean "$ROOT" 2>/dev/null || true
while IFS= read -r b; do
    codesign --force --deep --sign - "$b"
done < <(find "$ROOT" \( -name '*.vst3' -o -name '*.component' -o -name '*.app' \) -maxdepth 4)

# Single component pkg installed at "/".
pkgbuild \
    --root "$ROOT" \
    --identifier "$IDENT" \
    --version "$VERSION" \
    --install-location "/" \
    "$COMP/img2sample.pkg"

RESOURCES="$DIST/resources"
mkdir -p "$RESOURCES"

cat > "$RESOURCES/welcome.html" <<'WELCOME'
<!DOCTYPE html>
<html>
<head>
<style>
  body {
    font-family: -apple-system, "Helvetica Neue", sans-serif;
    font-size: 13px;
    line-height: 1.6;
    color: #1d1d1f;
    padding: 0;
    margin: 0;
  }
  h1 {
    font-size: 20px;
    font-weight: 600;
    margin: 0 0 16px 0;
    letter-spacing: -0.2px;
  }
  p {
    margin: 0 0 14px 0;
  }
  .dim {
    color: #86868b;
    font-size: 12px;
  }
  table {
    border-collapse: collapse;
    margin: 12px 0;
    font-size: 12px;
    width: 100%;
  }
  td {
    padding: 4px 0;
    vertical-align: top;
  }
  td:first-child {
    font-family: "SF Mono", Menlo, monospace;
    font-size: 11px;
    white-space: nowrap;
    padding-right: 16px;
    color: #1d1d1f;
  }
  td:last-child {
    color: #86868b;
  }
  .req {
    font-size: 11px;
    color: #86868b;
    margin-top: 18px;
    padding-top: 12px;
    border-top: 1px solid #e5e5e5;
  }
</style>
</head>
<body>
  <h1>img2sample</h1>
  <p>Turn images into audio samples. Drop a photo, painting, or texture &mdash; its colors, mood, and energy become an audio sample you can drag straight onto a track.</p>
  <p>Shape the output with element and mood tags, set BPM and key, and generate isolated stems (vocals, bass, drums, guitar, synth pads, textures, foley).</p>
  <p class="dim">Powered by Google&rsquo;s Lyria models via the Gemini API.</p>

  <table>
    <tr><td>img2sample.vst3</td><td>/Library/Audio/Plug-Ins/VST3</td></tr>
    <tr><td>img2sample.component</td><td>/Library/Audio/Plug-Ins/Components</td></tr>
    <tr><td>img2sample.app</td><td>/Applications</td></tr>
  </table>

  <p class="dim">After installing, rescan plugins in your DAW. You&rsquo;ll need a free Gemini API key on first run &mdash; get one at <b>aistudio.google.com/apikey</b>.</p>

  <p class="req">macOS 12+ &middot; Apple Silicon</p>
</body>
</html>
WELCOME

# Distribution wrapper (gives the nice installer UI + arch hint).
cat > "$DIST/distribution.xml" <<DISTXML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>img2sample ${VERSION}</title>
    <welcome file="welcome.html" mime-type="text/html"/>
    <options require-scripts="false" hostArchitectures="arm64"/>
    <choices-outline>
        <line choice="default"/>
    </choices-outline>
    <choice id="default" title="img2sample">
        <pkg-ref id="$IDENT"/>
    </choice>
    <pkg-ref id="$IDENT" version="${VERSION}">img2sample.pkg</pkg-ref>
</installer-gui-script>
DISTXML

productbuild \
    --distribution "$DIST/distribution.xml" \
    --package-path "$COMP" \
    --resources "$RESOURCES" \
    --version "$VERSION" \
    "$OUT"

rm -rf "$ROOT" "$COMP" "$RESOURCES" "$DIST/distribution.xml"

echo ""
echo "  Installer: $OUT"
du -sh "$OUT" | awk '{print "  Size: "$1}'
echo ""
echo "  On the target Mac: right-click the .pkg -> Open (unsigned, Gatekeeper)."
echo "================================================"
