#!/bin/bash

# Installation script for Vosk Voice Transcription Server (C)
# Optimized for Raspberry Pi 5
# Run with: bash install_rpi5.sh

set -e

echo "==========================================="
echo "  Vosk Voice Server - Raspberry Pi 5"
echo "  Installation Script"
echo "==========================================="
echo

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running on Raspberry Pi
if [ ! -f /proc/device-tree/model ]; then
    echo -e "${YELLOW}Warning: This script is optimized for Raspberry Pi 5${NC}"
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Update package list
echo -e "${GREEN}[1/7] Updating package list...${NC}"
sudo apt update

# Install build tools
echo -e "${GREEN}[2/7] Installing build tools...${NC}"
sudo apt install -y build-essential cmake git pkg-config

# Install libwebsockets
echo -e "${GREEN}[3/7] Installing libwebsockets...${NC}"
if ! pkg-config --exists libwebsockets; then
    echo "Building libwebsockets from source..."
    cd /tmp
    git clone https://github.com/warmcat/libwebsockets.git
    cd libwebsockets
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DLWS_WITH_SSL=ON \
             -DLWS_WITH_LIBUV=OFF \
             -DLWS_WITH_HTTP2=OFF
    make -j$(nproc)
    sudo make install
    sudo ldconfig
    cd ../../
    rm -rf libwebsockets
else
    echo "libwebsockets already installed"
fi

# Install OpenSSL development files
echo -e "${GREEN}[4/7] Installing OpenSSL development files...${NC}"
sudo apt install -y libssl-dev

# Install Vosk API
echo -e "${GREEN}[5/7] Installing Vosk API...${NC}"
if [ ! -f /usr/local/lib/libvosk.so ] && [ ! -f /usr/lib/libvosk.so ]; then
    echo "Downloading Vosk API for ARM64..."
    cd /tmp

    # Download Vosk API for ARM64 (aarch64)
    wget https://github.com/alphacep/vosk-api/releases/download/v0.3.45/vosk-linux-aarch64-0.3.45.zip
    unzip vosk-linux-aarch64-0.3.45.zip

    # Install library
    cd vosk-linux-aarch64-0.3.45
    sudo cp libvosk.so /usr/local/lib/
    sudo cp -r vosk_api.h /usr/local/include/
    sudo ldconfig

    cd ..
    rm -rf vosk-linux-aarch64-0.3.45*
else
    echo "Vosk API already installed"
fi

# Download Vosk model
echo -e "${GREEN}[6/7] Downloading Vosk model...${NC}"
cd "$(dirname "$0")/.."

if [ ! -d "model" ]; then
    echo "Downloading English model (40MB)..."
    wget https://alphacephei.com/vosk/models/vosk-model-small-en-us-0.15.zip
    unzip vosk-model-small-en-us-0.15.zip
    mv vosk-model-small-en-us-0.15 model
    rm vosk-model-small-en-us-0.15.zip
    echo "Model downloaded and extracted"
else
    echo "Model directory already exists"
fi

# Generate SSL certificates
echo -e "${GREEN}[7/7] Generating SSL certificates...${NC}"
if [ ! -f "cert.pem" ] || [ ! -f "key.pem" ]; then
    echo "Generating self-signed SSL certificate..."
    openssl req -x509 -newkey rsa:4096 -nodes -out cert.pem -keyout key.pem -days 365 \
        -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"
    echo "SSL certificates generated"
else
    echo "SSL certificates already exist"
fi

# Build the application
echo
echo -e "${GREEN}Building the application...${NC}"
cd c_implementation
make rpi5

echo
echo -e "${GREEN}==========================================="
echo "  Installation Complete!"
echo "==========================================="
echo
echo "To run the server:"
echo "  cd c_implementation"
echo "  ./build/vosk_server"
echo
echo "Or install system-wide:"
echo "  cd c_implementation"
echo "  sudo make install"
echo "  vosk_server"
echo
echo "Access the server at:"
echo "  https://$(hostname -I | awk '{print $1}'):5000"
echo
echo -e "${YELLOW}Note: You'll see a security warning due to the self-signed certificate.${NC}"
echo -e "${YELLOW}Click 'Advanced' and 'Proceed' to continue (safe for local use).${NC}"
echo
