# Makefile for midi_converter

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -mconsole -I.
LDFLAGS = -mconsole

# Directories
MIDIFILE_DIR = ../../../ROMvX0-main/Exe/midifile-master
MIDIFILE_INCLUDE = $(MIDIFILE_DIR)/include
MIDIFILE_LIB = $(MIDIFILE_DIR)/lib

# Source and target
TARGET = midi_converter.exe
ANALYZER_TARGET = midi_analyzer.exe
SOURCES = midi_converter.cpp ini_parser.cpp
ANALYZER_SOURCES = midi_analyzer.cpp ini_parser.cpp
OBJECTS = $(SOURCES:.cpp=.o)
ANALYZER_OBJECTS = $(ANALYZER_SOURCES:.cpp=.o)

# Pre-built library
MIDIFILE_LIBRARY = $(MIDIFILE_DIR)/lib/libmidifile.a

# Default target
all: $(TARGET) $(ANALYZER_TARGET)

# Build the main executable using the pre-built library
$(TARGET): $(OBJECTS) $(MIDIFILE_LIBRARY)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@ $(MIDIFILE_LIBRARY)

# Build the analyzer executable
$(ANALYZER_TARGET): $(ANALYZER_OBJECTS) $(MIDIFILE_LIBRARY)
	$(CXX) $(LDFLAGS) $(ANALYZER_OBJECTS) -o $@ $(MIDIFILE_LIBRARY)

# Rule to compile .cpp files into .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(MIDIFILE_INCLUDE) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(ANALYZER_TARGET) $(OBJECTS) $(ANALYZER_OBJECTS)

# Test the program with the example command
test: $(TARGET)
	./$(TARGET) ff1_open.mid ff1_test.c -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 40 -compensate 60 -ch1wave 1 -ch2wave 1 -ch3wave 1 -ch4wave 1

# Test with the new parameter format example
test-new: $(TARGET)
	./$(TARGET) Aria.mid Aria.c -d -nv -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 20 -compensate 60 -ch1wave 1 -ch2wave 0 -ch3wave 3 -ch4wave 1

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
	@echo "  ./midi_converter.exe Aria.mid Aria.c -d -nv -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 20 -compensate 60 -ch1wave 1 -ch2wave 0 -ch3wave 3 -ch4wave 1"

.PHONY: all clean test test-new help