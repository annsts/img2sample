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
VERSION="1.0.0"
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

# Distribution wrapper (gives the nice installer UI + arch hint).
cat > "$DIST/distribution.xml" <<DISTXML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>img2sample ${VERSION}</title>
    <welcome mime-type="text/plain"><![CDATA[
img2sample — Image to Audio Sample Plugin

Installs:
  - img2sample.vst3      -> /Library/Audio/Plug-Ins/VST3
  - img2sample.component -> /Library/Audio/Plug-Ins/Components (Audio Unit)
  - img2sample.app       -> /Applications

After installing, rescan plugins in your DAW. You'll need a Gemini API key
on first run (https://aistudio.google.com/apikey).

Apple Silicon, macOS 12+.
    ]]></welcome>
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
    --version "$VERSION" \
    "$OUT"

rm -rf "$ROOT" "$COMP" "$DIST/distribution.xml"

echo ""
echo "  Installer: $OUT"
du -sh "$OUT" | awk '{print "  Size: "$1}'
echo ""
echo "  On the target Mac: right-click the .pkg -> Open (unsigned, Gatekeeper)."
echo "================================================"
