# Makefile for midi_converter

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -mconsole
LDFLAGS = -mconsole

# Directories
MIDIFILE_DIR = midifile-master
MIDIFILE_INCLUDE = $(MIDIFILE_DIR)/include
MIDIFILE_LIB = $(MIDIFILE_DIR)/lib

# Source and target
TARGET = midi_converter.exe
SOURCES = midi_converter.cpp ini_parser.cpp

# Pre-built library
MIDIFILE_LIBRARY = $(MIDIFILE_LIB)/libmidifile.a

# Default target
all: $(TARGET)

# Build the main executable using the pre-built library
$(TARGET): $(SOURCES) $(MIDIFILE_LIBRARY)
	$(CXX) $(CXXFLAGS) -I$(MIDIFILE_INCLUDE) $(SOURCES) -o $@ $(MIDIFILE_LIBRARY)

# Clean build artifacts
clean:
	rm -f $(TARGET)

# Test the program with the example command
test: $(TARGET)
	./$(TARGET) ff1_open.mid ff1_test.gbas -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 40 -compensate 60 -ch1wave 1 -ch2wave 1 -ch3wave 1 -ch4wave 1

# Test with the new parameter format example
test-new: $(TARGET)
	./$(TARGET) ff1_open.mid ff1.gbas -d -nv -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 20 -compensate 60 -ch1wave 1 -ch2wave 0 -ch3wave 3 -ch4wave 1

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the midi_converter executable"
	@echo "  clean    - Remove build artifacts"
	@echo "  test     - Build and test with example command"
	@echo "  test-new - Build and test with new parameter format"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Example usage:"
	@echo "  make test"
	@echo "  make test-new"
	@echo "  ./midi_converter.exe ff1_open.mid ff1.gbas -d -nv -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 20 -compensate 60 -ch1wave 1 -ch2wave 0 -ch3wave 3 -ch4wave 1"

.PHONY: all clean test test-new help