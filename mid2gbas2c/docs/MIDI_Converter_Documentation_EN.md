# MIDI Converter Functionality Documentation

## Table of Contents

1. [Project Overview](#project-overview)
2. [Features](#features)
3. [Command-Line Arguments](#command-line-arguments)
4. [Core Algorithms](#core-algorithms)
   1. [MIDI File Parsing](#midi-file-parsing)
   2. [Channel Allocation Strategy](#channel-allocation-strategy)
   3. [Pitch Bend and Vibrato Wheel Handling](#pitch-bend-and-vibrato-wheel-handling)
   4. [Timer Compensation Mechanism](#timer-compensation-mechanism)
5. [Volume Processing](#volume-processing)
6. [Macro Event System](#macro-event-system)
7. [Visualization Feature](#visualization-feature)
8. [Development Process](#development-process)
9. [Usage Examples](#usage-examples)
10. [Technical Details](#technical-details)
    1. [Initial Timer Optimization](#initial-timer-optimization)

## Project Overview

The MIDI Converter is a tool that converts standard MIDI files into executable code for the Gigatron audio engine. The Gigatron is an 8-bit retro computer with 4-channel audio synthesis capabilities. This converter supports complex MIDI features, including pitch bend, vibrato wheel, dynamic channel allocation, and timer compensation, to achieve optimal music playback on limited hardware resources.

## Features

### 1. MIDI File Parsing
- Supports standard MIDI file format (.mid)
- Parses multi-track MIDI files
- Automatically handles Note On/Note Off event pairs
- Supports velocity, volume controllers, expression controllers
- Supports program changes (instrument switching)

### 2. Pitch Bend and Vibrato Wheel Handling
- **Pitch Bend Quantization**: Quantizes MIDI pitch bend range (-8192 to 8191) into a small range (-50 to 50)
- **Large Pitch Bend Handling**: Pitch bends exceeding a semitone range are automatically expanded into note changes + small bends
- **Vibrato Wheel Quantization**: Quantizes modulation wheel values (0-127) into a small range (0-20)
- **Pitch Bend Disable Option**: Completely disables pitch bend and vibrato output via the `-np` parameter

### 3. Channel Waveform Force Specification
- **Waveform Force Specification**: Force specific waveforms for each channel via `-ch1wave` to `-ch4wave` parameters
- **Supported Waveform Types**: 0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto (use MIDI program conversion)
- **Flexible Configuration**: Each channel can be independently configured with different waveforms for richer timbre combinations

### 3. Channel Allocation Strategy
- **Static Allocation**: Assigns MIDI channels to Gigatron channels (1-4) in MIDI channel order
- **Dynamic Allocation**: Uses a FIFO (First-In, First-Out) strategy to manage 4 Gigatron channels
- **Smart Channel Switching**: Automatically replaces the least recently used channel when all channels are in use

### 4. Timer Compensation Mechanism
- **Precision Compensation**: Supports compensating low-precision timers to high precision (e.g., 30 → 60)
- **Channel Distribution**: Distributes channel events across different timer ticks during compensation
- **Smart Triggering**: Compensation is only triggered when the target precision is significantly higher than the current precision

### 5. Volume Processing
- **Volume Mapping**: MIDI velocity (0-127) is mapped to Gigatron volume (0-63)
- **Minimum Volume Boost**: Supports compressing non-zero volumes above a specified minimum
- **Combined Volume Calculation**: Combines base velocity, volume controller, and expression controller

### 6. Macro Event System
- **Volume Macro Sequence**: Defines volume change sequence during note duration
- **Waveform Macro Sequence**: Defines waveform change sequence during note duration
- **Pitch Bend Macro Sequence**: Defines pitch bend change sequence during note duration
- **Release Macro Sequence**: Defines volume, waveform, and pitch bend change sequence after note release
- **Macro Event Generation**: Automatically generates macro events based on configuration to implement complex sound effects

### 7. Visualization Feature
- **Simple Pixel Visualization**: Provides piano keyboard-like visualization on the Gigatron screen
- **Note Position Mapping**: Maps notes to specific positions on the screen
- **Volume Color Encoding**: Uses different colors to represent volume levels
- **Real-time Display**: Synchronizes with music playback to show active note status

## Command-Line Arguments

```
Usage: midi_converter.exe <midi_file_path> <output_gbas_file_path> [options]
```

### Argument Description

#### Required Parameters
- `midi_file_path`: Path to the input MIDI file
- `output_gbas_file_path`: Path to the output Gigatron BASIC file

#### Optional Parameters
- `-d`: Enable dynamic channel allocation (default: static allocation)
- `-nv`: Disable velocity changes during note on (volume fixed at note on)
- `-np`: Disable pitch bend and modulation (quantize to semitones only, pitch=0)
- `-time <seconds>`: Maximum duration in seconds (default: unlimited)
- `-pitch_multiple <value>`: Pitch bend multiplier (default: 1.0)
- `-accuracy <levels>`: Volume accuracy levels 1-64 (default: 64)
- `-vl <levels>`: Volume accuracy levels 1-64 (alias for -accuracy)
- `-min_volume <value>`: Minimum volume level 0-63 (default: 0)
- `-compensate <value>`: Timer compensation target (default: 60)
- `-speed <value>`: Playback speed multiplier (default: 1.0)
- `-ch1wave <wave>`: Channel 1 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)
- `-ch2wave <wave>`: Channel 2 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)
- `-ch3wave <wave>`: Channel 3 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)
- `-ch4wave <wave>`: Channel 4 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)
- `-config <file>`: Use INI configuration file for instrument settings (default: no configuration file)

## Core Algorithms

### MIDI File Parsing

The program uses the `MidiFile` library to parse MIDI files, performing the following steps:

1. **File Reading**: Reads the entire MIDI file into memory
2. **Time Analysis**: Calculates the absolute timestamp for each event
3. **Note Linking**: Automatically matches Note On and Note Off event pairs
4. **Event Classification**: Classifies events into Note On/Off, controllers, pitch bend, etc.
5. **RPN Processing**: Processes Registered Parameter Number messages, supporting pitch bend sensitivity settings

### Channel Allocation Strategy

#### Static Allocation Mode
- MIDI channels (0-15) are assigned to Gigatron channels (1-4) in order.
- The first 4 MIDI channels with note activity acquire Gigatron channels.
- Subsequent MIDI channels are ignored.

#### Dynamic Allocation Mode
- Maintains a list of available channels `{1, 2, 3, 4}`.
- Assigns an available Gigatron channel to each new MIDI channel.
- When all channels are in use, a FIFO strategy is employed:
  - The time of the last Note On event for each Gigatron channel is recorded.
  - When a new channel is needed, the least recently used channel is replaced.

#### Single Track Polyphony Processing
- In dynamic allocation mode, supports polyphonic playback for a single MIDI channel
- Uses FIFO queue to manage multiple notes from the same MIDI channel
- When all Gigatron channels are occupied, replaces the oldest note

### Pitch Bend and Vibrato Wheel Handling

#### Pitch Bend Quantization Algorithm
```cpp
// Converts MIDI pitch bend value (-8192 to 8191) to semitones
int raw_bend_value = message.getP2() * 128 + message.getP1();
int bend_value_centered = raw_bend_value - 8192;
double actual_semitone_bend = (static_cast<double>(bend_value_centered) / 8192.0) * pitch_bend_range;

// Splits large pitch bends into note offsets and fine bends
int note_offset = static_cast<int>(std::round(total_bend_semitones));
double fine_bend_semitones = total_bend_semitones - note_offset;

// Converts to Gigatron engine units
double current_pitch_bend_cents = fine_bend_semitones * 100.0;
int final_pitch_bend_gigatron_unit = static_cast<int>(current_pitch_bend_cents * pitch_bend_multiplier);
```

#### Vibrato Wheel Quantization Algorithm
```cpp
// Quantizes modulation wheel value (0-127) into a small range (0-20)
const double max_vibrato_depth_cents = 50.0;
double vibrato_depth_cents = (static_cast<double>(modulation) / 127.0) * max_vibrato_depth_cents;
current_pitch_bend_cents += vibrato_depth_cents;
```

#### Pitch Bend Sensitivity Processing
```cpp
// Process RPN messages to set pitch bend sensitivity
if (controller_number == 101) { // RPN MSB
    _rpn_msb[midi_channel] = controller_value;
} else if (controller_number == 100) { // RPN LSB
    _rpn_lsb[midi_channel] = controller_value;
} else if (controller_number == 6) { // Data Entry MSB
    _data_entry_msb[midi_channel] = controller_value;
    // If RPN 0 (Pitch Bend Range), update pitch bend sensitivity
    if (_rpn_msb[midi_channel] == 0 && _rpn_lsb[midi_channel] == 0) {
        double new_range = _data_entry_msb[midi_channel] + (_data_entry_lsb[midi_channel] != -1 ? _data_entry_lsb[midi_channel] / 100.0 : 0.0);
        _channel_pitch_bend_range[midi_channel] = std::min(72.0, std::max(0.0, new_range));
    }
}
```

### Timer Compensation Mechanism

#### Compensation Algorithm
```cpp
// Calculates compensation ratio
double compensation_ratio = static_cast<double>(timer_compensation_target) / gigatron_ticks_per_second;
bool needs_compensation = (compensation_ratio > 1.01);

// If compensation is needed and there are multiple channel events at the current tick, distribute channels
if (needs_compensation && channel_events_list.size() > 1) {
    // Calculates the number of compensation timers to insert
    int compensation_ticks = static_cast<int>(std::floor(compensation_ratio)) - 1;
    
    // Splits channel events into two halves
    size_t half_size = channel_events_list.size() / 2;
    std::vector<std::pair<int, CustomMidiEvent>> first_half(channel_events_list.begin(), channel_events_list.begin() + half_size);
    std::vector<std::pair<int, CustomMidiEvent>> second_half(channel_events_list.begin() + half_size, channel_events_list.end());
    
    // Outputs the first half of channel events
    // Inserts compensation timer calls
    // Outputs the second half of channel events
}
```

### Volume Processing

#### Volume Mapping Algorithm
```cpp
// MIDI velocity 0-127 scaled to 0-63
int scaled_velocity = std::min(63, velocity / 2);

// Minimum volume boost
if (min_volume_boost > 0 && min_volume_boost < 63 && scaled_velocity > 0) {
    scaled_velocity = min_volume_boost + static_cast<int>(std::round(
        (static_cast<double>(scaled_velocity - 1) / 62.0) * (63 - min_volume_boost)
    ));
    scaled_velocity = std::min(63, scaled_velocity);
}

// Combined volume calculation
double combined_volume_double = (base_velocity * volume_controller * expression_controller) / (127.0 * 127.0);
int vol = convert_midi_volume(static_cast<int>(std::round(combined_volume_double)), min_volume_boost);
```

#### Volume Simplification Algorithm
```cpp
// Simplify volume values to specified number of levels
int simplify_volume(int volume, int volume_levels) {
    if (volume_levels <= 1) return 0; // If only 1 level, all volumes are 0
    if (volume_levels >= 64) return volume; // If levels >= 64, no simplification
    
    // Divide 0-63 volume range into volume_levels levels
    int level_size = 64 / volume_levels;
    int level = volume / level_size;
    
    // Ensure not out of range
    if (level >= volume_levels) level = volume_levels - 1;
    
    // Return the middle value of the level to maintain relative volume relationships
    return level * level_size + level_size / 2;
}
```

### Configuration File System

#### Configuration File Parsing
The program uses the `IniParser` class to parse INI configuration files, supporting the following configuration items:

1. **Instrument Configuration**: Configure specific parameters for each MIDI instrument program number (0-127)
2. **Drum Configuration**: Configure specific parameters for each drum note (27-87)
3. **Macro Definitions**: Define volume, waveform, pitch bend, and release sequence macros
4. **Accuracy Settings**: Set independent accuracy levels for each instrument

#### Configuration File Format Example
```ini
[Instrument_80]
accuracy=30
macros=wave,vol,pitch_bend,release_vol,release_wave,release_pitch_bend
wave_on=2
vol_on=63,60,55,50,45,40,35,30
pitch_bend_on=0,0,0,0,0,0,0
release_vol_on=30,20,10,0
release_wave_on=1,1,1,1
release_pitch_bend_on=0,0,0,0

[Drum_35]
accuracy=20
macros=wave,vol
wave_on=0
vol_on=63,50,30,10,0
```

## Development Process

### Phase One: Basic Functionality Implementation
1. **MIDI Parsing**: Implement basic MIDI file reading and event parsing.
2. **Simple Conversion**: Convert Note On/Off events into Gigatron BASIC code.
3. **Channel Mapping**: Implement simple MIDI channel to Gigatron channel mapping.

### Phase Two: Advanced Feature Development
1. **Pitch Bend Support**: Add pitch bend parsing and quantization.
2. **Vibrato Support**: Add modulation wheel parsing and quantization.
3. **Dynamic Channel Allocation**: Implement dynamic channel allocation with FIFO strategy.
4. **Volume Optimization**: Add support for volume and expression controllers.

### Phase Three: Optimization and Extension
1. **Timer Compensation**: Implement low-to-high precision timer compensation.
2. **Parameterized Design**: Add command-line argument support for various configurations.
3. **Performance Optimization**: Optimize event processing and code generation logic.
4. **Error Handling**: Improve error handling and user feedback.

### Phase Four: Configuration System and Macro Features
1. **Configuration File System**: Implement INI configuration file parsing, supporting instrument and drum configuration.
2. **Macro Event System**: Implement volume, waveform, pitch bend, and release sequence macros.
3. **Volume Simplification**: Implement volume value simplification to specified number of levels.
4. **Single Track Polyphony**: Improve dynamic allocation mode to support single track polyphony processing.
5. **Release Event Processing**: Add release macro sequence processing.

### Phase Five: Refinement and Testing
1. **Pitch Bend Disable**: Add the `-np` parameter to completely disable pitch bend and vibrato.
2. **Debug Log System**: Add detailed debug logging functionality.
3. **Documentation**: Write detailed usage instructions and technical documentation.
4. **Comprehensive Testing**: Conduct thorough testing with various MIDI files.

## Usage Examples

### Basic Conversion
```bash
./midi_converter.exe input.mid output.gbas
```

### Advanced Conversion (Dynamic Allocation + Pitch Bend Quantization)
```bash
./midi_converter.exe input.mid output.gbas -d -time 30 -pitch_multiple 1.0 -min_volume 30 -compensate 60
```

### Disable Pitch Bend and Vibrato
```bash
./midi_converter.exe input.mid output.gbas -np -time 30 -pitch_multiple 1.0 -min_volume 30 -compensate 60
```

### Disable Velocity Changes
```bash
./midi_converter.exe input.mid output.gbas -nv -time 30 -pitch_multiple 1.0 -min_volume 30 -compensate 60
```

### Timer Compensation (30 precision compensated to 60)
```bash
./midi_converter.exe input.mid output.gbas -d -np -time 30 -pitch_multiple 1.0 -compensate 30 -min_volume 30
```

### Channel Waveform Force Specification
```bash
./midi_converter.exe input.mid output.gbas -ch1wave 1 -ch2wave 2 -ch3wave 3 -ch4wave 0
```

### Volume Accuracy Control
```bash
./midi_converter.exe input.mid output.gbas -vl 8 -time 30 -min_volume 20
```

### Speed Control
```bash
./midi_converter.exe input.mid output.gbas -speed 0.5 -time 30
```

### Combined Example (Dynamic Allocation + Pitch Bend Quantization + Channel Waveform Specification)
```bash
./midi_converter.exe input.mid output.gbas -d -nv -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 20 -compensate 60 -ch1wave 1 -ch2wave 0 -ch3wave 3 -ch4wave 1
```

## Technical Details

### Data Structures

#### CustomMidiEvent Struct
```cpp
struct CustomMidiEvent {
    int channel;        // Gigatron Channel (1-4)
    int note;           // MIDI Note Index (0-127)
    double velocity;     // MIDI Velocity (0-127)
    double volume;       // Current Volume (Controller 7)
    double expression;     // Current Expression (Controller 11)
    int program;        // MIDI Instrument Program Number (0-127)
    long timestamp;     // Event Timestamp, in MIDI ticks
    long duration;      // Note Duration, in MIDI ticks
    bool is_note_off;    // Is Note Off Event
    double pitch_bend;      // MIDI Pitch Bend Value (in semitones)
    bool is_velocity_change; // Is Velocity Change Event
    bool is_pitch_bend_change; // Is Pitch Bend Change Event
    bool is_volume_change; // Is Volume Change Event
    int modulation;     // MIDI Modulation Wheel Value (CC 1)
};
```

#### NoteState Struct
```cpp
struct NoteState {
    int channel;        // Gigatron Channel
    int note;           // MIDI Note Index
    int velocity;       // Current Velocity
    int volume;         // Current Volume (Controller 7)
    int expression;     // Current Expression (Controller 11)
    int program;        // Current Instrument Program Number
    double pitch_bend;  // Current Pitch Bend Value (in semitones)
    int modulation;     // Current Modulation Wheel Value
    long start_tick;    // Note Start Tick
    bool active;        // Is Note Active
};
```

### Key Algorithms

#### Event Optimization Algorithm
1. **Priority Handling**: Note Off events > Note On events > Other events
2. **Channel Merging**: Multiple events on the same channel at the same tick are merged into one.
3. **Average Calculation**: Calculates the average for parameters like volume, expression, and pitch bend.

#### Timestamp Conversion Algorithm
```cpp
// Converts MIDI ticks to Gigatron ticks
double gigatron_ticks_per_midi_tick = (static_cast<double>(tempo_us) * gigatron_ticks_per_second) / (static_cast<double>(ppqn) * 1000000.0);
long gigatron_start_tick = static_cast<long>(event.timestamp * gigatron_ticks_per_midi_tick);
```

### Initial Timer Optimization
- **Dynamic `tick_sum` Initialization**: The `tick_sum` variable in `proc music_data` is now dynamically initialized to `(first_event_tick - 1)` instead of a fixed `0`.
- **Accurate Timing**: Ensures that the first `eatSound_Timer` call accurately reflects the timing of the first MIDI event.

### Output Format

The generated Gigatron BASIC code includes the following sections:
1. **Initialization Code**: Sets variables and the audio engine.
2. **Pitch Correction Table**: Pitch correction values for each octave.
3. **Timer Handling**: The `eatSound_Timer` procedure.
4. **Note Playback**: The `beep` procedure.
5. **Main Program**: The `music_data` procedure, containing all note events.

### Channel Waveform Force Specification Algorithm
```cpp
// Use forced specified waveform, or default MIDI program conversion if not specified
int wave;
if (channel_waveforms[event.channel] != -1) {
    wave = channel_waveforms[event.channel];
} else {
    wave = convert_midi_waveform(event.program);
}
```

### Dependencies
- **MidiFile**: For MIDI file parsing.
- **Standard C++ Library**: iostream, vector, string, algorithm, cmath, map, fstream.

### Compilation Requirements
- C++11 or higher compliant compiler.
- Requires linking the MidiFile library.

### Performance Considerations
- Uses `std::map` for fast lookups.
- Events are sorted by timestamp to avoid redundant sorting.
- Batch processes events at the same tick to reduce output file size.

## Conclusion

The MIDI Converter is a powerful tool capable of converting complex MIDI files into Gigatron executable code. Through intelligent channel allocation, precise pitch bend and vibrato wheel quantization, flexible timer compensation mechanisms, and meticulous volume processing, the converter achieves high-quality music playback on limited hardware resources.

The project's design fully considers the limitations of Gigatron hardware and employs various optimization techniques to ensure that the generated code is both efficient and easy to understand. Detailed command-line argument support allows users to flexibly configure the conversion process according to their specific needs.