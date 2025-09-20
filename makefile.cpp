# Makefile for CipherChat-CPP

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
TARGET = cipherchat
SRCDIR = src
SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lws2_32
    TARGET := $(TARGET).exe
else
    LDFLAGS = -pthread
endif

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)
ifeq ($(OS),Windows_NT)
	del /F /Q *.o $(TARGET) 2>nul || true
endif

# Install (Linux/macOS)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall (Linux/macOS)
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build (default)
release: CXXFLAGS += -DNDEBUG
release: $(TARGET)

# Run server
run-server: $(TARGET)
	./$(TARGET)

# Run client
run-client: $(TARGET)
	./$(TARGET)

# Test build
test: $(TARGET)
	@echo "Building test completed successfully"
	@echo "Run './$(TARGET)' to start CipherChat"

# Help
help:
	@echo "Available targets:"
	@echo "  all         - Build CipherChat (default)"
	@echo "  clean       - Remove build files"
	@echo "  debug       - Build with debug symbols"
	@echo "  release     - Build optimized version"
	@echo "  install     - Install to system (Linux/macOS)"
	@echo "  uninstall   - Remove from system (Linux/macOS)"
	@echo "  run-server  - Build and run in server mode"
	@echo "  run-client  - Build and run in client mode"
	@echo "  test        - Test build"
	@echo "  help        - Show this help"

# Phony targets
.PHONY: all clean install uninstall debug release run-server run-client test help
