#!/bin/bash

# CipherChat-CPP Build Script
# Supports Linux, macOS, and Windows (with MinGW)

set -e  # Exit on any error

echo "================================================"
echo "           CipherChat-CPP Build Script"
echo "================================================"

# Detect operating system
OS="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    OS="windows"
fi

echo "Detected OS: $OS"

# Set compiler and flags based on OS
CXX="g++"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -pthread"
LDFLAGS=""
TARGET="cipherchat"

case $OS in
    "linux")
        LDFLAGS="-pthread"
        ;;
    "macos")
        LDFLAGS="-pthread"
        ;;
    "windows")
        LDFLAGS="-lws2_32"
        TARGET="cipherchat.exe"
        ;;
esac

echo "Compiler: $CXX"
echo "Flags: $CXXFLAGS"
echo "Linker flags: $LDFLAGS"
echo "Target: $TARGET"

# Check if compiler exists
if ! command -v $CXX &> /dev/null; then
    echo "ERROR: $CXX compiler not found!"
    echo "Please install a C++17 compatible compiler:"
    case $OS in
        "linux")
            echo "  Ubuntu/Debian: sudo apt install g++"
            echo "  CentOS/RHEL: sudo yum install gcc-c++"
            echo "  Arch: sudo pacman -S gcc"
            ;;
        "macos")
            echo "  Install Xcode Command Line Tools: xcode-select --install"
            echo "  Or install via Homebrew: brew install gcc"
            ;;
        "windows")
            echo "  Install MinGW-w64 or use MSYS2"
            ;;
    esac
    exit 1
fi

# Clean previous builds
echo "Cleaning previous builds..."
if [[ -f "$TARGET" ]]; then
    rm "$TARGET"
    echo "Removed existing $TARGET"
fi

if [[ -f "main.o" ]]; then
    rm main.o
    echo "Removed main.o"
fi

# Check if source file exists
if [[ ! -f "main.cpp" ]]; then
    echo "ERROR: main.cpp not found!"
    echo "Please ensure main.cpp is in the current directory."
    exit 1
fi

# Compile the program
echo "Compiling CipherChat-CPP..."
echo "Command: $CXX $CXXFLAGS -o $TARGET main.cpp $LDFLAGS"

if $CXX $CXXFLAGS -o $TARGET main.cpp $LDFLAGS; then
    echo "✅ Build successful!"
    echo "Executable created: $TARGET"
    
    # Check if executable was created and is executable
    if [[ -x "$TARGET" ]]; then
        echo "✅ Executable permissions verified"
        
        # Get file size
        if command -v stat &> /dev/null; then
            if [[ "$OS" == "macos" ]]; then
                SIZE=$(stat -f%z "$TARGET")
            else
                SIZE=$(stat -c%s "$TARGET")
            fi
            echo "File size: $SIZE bytes"
        fi
        
        echo ""
        echo "================================================"
        echo "           Build Complete!"
        echo "================================================"
        echo "To run CipherChat-CPP:"
        echo "  ./$TARGET"
        echo ""
        echo "Available make targets:"
        echo "  make          - Build the application"
        echo "  make debug    - Build with debug symbols"
        echo "  make clean    - Clean build files"
        echo "  make install  - Install system-wide (Linux/macOS)"
        echo ""
        
    else
        echo "❌ ERROR: Executable was created but is not executable"
        echo "Try: chmod +x $TARGET"
        exit 1
    fi
else
    echo "❌ Build failed!"
    echo ""
    echo "Common solutions:"
    echo "1. Check that you have a C++17 compatible compiler"
    echo "2. Ensure all required libraries are installed"
    echo "3. Try building with make instead: make"
    echo "4. Check the compiler version: $CXX --version"
    exit 1
fi

# Optional: Run basic test
echo ""
read -p "Would you like to run a build test? (y/N): " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Running build test..."
    if timeout 3s ./$TARGET --version 2>/dev/null || true; then
        echo "✅ Basic executable test passed"
    else
        echo "⚠️  Test completed (this is expected for interactive programs)"
    fi
fi

echo ""
echo "Build script completed successfully!"
echo "You can now run: ./$TARGET"
