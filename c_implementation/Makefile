# Makefile for Vosk Voice Transcription Server (C)
# Optimized for Raspberry Pi 5

CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude -D_GNU_SOURCE
LDFLAGS = -lwebsockets -lvosk -lssl -lcrypto -lpthread -lm

# Raspberry Pi 5 specific optimizations
RPI5_FLAGS = -march=armv8.2-a -mtune=cortex-a76 -mfpu=neon-fp-armv8

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = $(BUILD_DIR)/vosk_server

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build target
$(TARGET): $(BUILD_DIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Raspberry Pi 5 optimized build
rpi5: CFLAGS += $(RPI5_FLAGS)
rpi5: clean all

# Install target (copy to /usr/local/bin)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/vosk_server
	sudo chmod +x /usr/local/bin/vosk_server
	@echo "Installed to /usr/local/bin/vosk_server"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# Run the server
run: $(TARGET)
	cd .. && ./c_implementation/$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the server (default)"
	@echo "  rpi5      - Build with Raspberry Pi 5 optimizations"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Build and run the server"
	@echo "  debug     - Build with debug symbols"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  help      - Show this help message"

.PHONY: all clean run debug install rpi5 help
