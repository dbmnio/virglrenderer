#!/bin/bash

# Simple build script for Phase 2 CGL test
# This builds the test using the main project's build system

set -e

echo "🔨 Building Phase 2 CGL Test..."

# Check if we're on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo "❌ Error: This test is designed for macOS only"
    exit 1
fi

# Try different possible build directory names
if [[ -d "builddir" ]]; then
    BUILD_DIR="builddir"
elif [[ -d "build" ]]; then
    BUILD_DIR="build"
else
    echo "📦 Setting up build directory..."
    meson setup builddir -Dtests=enabled
    BUILD_DIR="builddir"
fi

# Compile the main project to ensure all dependencies are built
echo "🔧 Building main project in ${BUILD_DIR}..."
meson compile -C ${BUILD_DIR}

# Find the correct library path
if [[ -f "${BUILD_DIR}/src/libvirglrenderer.a" ]]; then
    LIB_PATH="${BUILD_DIR}/src/libvirglrenderer.a"
elif [[ -f "${BUILD_DIR}/src/libvirglrenderer.so" ]]; then
    LIB_PATH="-L${BUILD_DIR}/src -lvirglrenderer"
else
    echo "❌ Error: Could not find virglrenderer library in ${BUILD_DIR}/src/"
    exit 1
fi

# Build the Phase 2 test using the project's built libraries
echo "🧪 Building Phase 2 test..."
clang -std=c99 -Wall -Wextra \
    -I. -I${BUILD_DIR} -I${BUILD_DIR}/src \
    -framework OpenGL \
    -lepoxy \
    test_phase2_cgl.c \
    ${LIB_PATH} \
    -o test_phase2_cgl

echo "✅ Build complete! Run with: ./test_phase2_cgl" 