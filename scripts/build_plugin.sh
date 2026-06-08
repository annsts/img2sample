#!/usr/bin/env bash
# img2sample - Image to Audio Sample Plugin
# Copyright (C) 2026
# SPDX-License-Identifier: GPL-3.0-or-later

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "================================================"
echo "  img2sample — Build Plugin"
echo "================================================"
echo ""

# Check build tools
if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake is required but not found."
    exit 1
fi

if [[ "$(uname)" == "Darwin" ]]; then
    if ! command -v clang++ &> /dev/null; then
        echo "ERROR: clang++ is required but not found. Install Xcode command line tools."
        exit 1
    fi
else
    if ! command -v g++ &> /dev/null; then
        echo "ERROR: g++ is required but not found."
        exit 1
    fi
fi

# Install system dependencies (Linux only)
if [[ "$(uname)" == "Linux" ]]; then
    echo "[1/3] Checking Linux build dependencies..."
    if command -v apt-get &> /dev/null; then
        DEPS="libasound2-dev libfreetype-dev libxrandr-dev libxcursor-dev libxinerama-dev libgl-dev libxcomposite-dev libxext-dev pkg-config"
        MISSING=""
        for dep in $DEPS; do
            if ! dpkg -s "$dep" &>/dev/null 2>&1; then
                MISSING="$MISSING $dep"
            fi
        done
        if [ -n "$MISSING" ]; then
            echo "  Installing missing packages:$MISSING"
            sudo apt-get update -qq && sudo apt-get install -y -qq $MISSING
        else
            echo "  All dependencies present."
        fi
    elif command -v dnf &> /dev/null; then
        DEPS="alsa-lib-devel freetype-devel libXrandr-devel libXcursor-devel libXinerama-devel mesa-libGL-devel libXcomposite-devel libXext-devel pkg-config"
        sudo dnf install -y -q $DEPS
    elif command -v pacman &> /dev/null; then
        DEPS="alsa-lib freetype2 libxrandr libxcursor libxinerama mesa libxcomposite libxext pkg-config"
        sudo pacman -S --needed --noconfirm $DEPS
    else
        echo "  WARNING: Unrecognized package manager. Please install ALSA, freetype,"
        echo "  X11 (xrandr, xcursor, xinerama, xcomposite, xext), OpenGL, and pkg-config manually."
    fi
fi

# Configure
echo ""
echo "[2/3] Configuring..."
cmake -B "$PROJECT_DIR/build" -S "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -E "^--|Configuring|Generating"

# Build
echo ""
echo "[3/3] Building..."
cmake --build "$PROJECT_DIR/build" --config Release -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "================================================"
echo "  Build complete!"
echo ""

# Report output locations
if [[ "$(uname)" == "Linux" ]]; then
    VST3_PATH="$PROJECT_DIR/build/Img2Sample_artefacts/Release/VST3/img2sample.vst3"
    STANDALONE_PATH="$PROJECT_DIR/build/Img2Sample_artefacts/Release/Standalone/img2sample"
elif [[ "$(uname)" == "Darwin" ]]; then
    VST3_PATH="$PROJECT_DIR/build/Img2Sample_artefacts/Release/VST3/img2sample.vst3"
    STANDALONE_PATH="$PROJECT_DIR/build/Img2Sample_artefacts/Release/Standalone/img2sample.app"
fi

echo "  VST3:       $VST3_PATH"
echo "  Standalone:  $STANDALONE_PATH"
echo ""
echo "  To install the VST3 system-wide (Linux):"
echo "    cp -r $VST3_PATH ~/.vst3/"
echo "================================================"
