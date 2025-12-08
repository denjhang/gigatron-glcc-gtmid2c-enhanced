#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath> // For std::round
#include <map>
#include <fstream> // Include fstream for file operations
#include <iomanip> // For std::setfill and std::setw

#define byte unsigned char // 定义 byte 类型

#include "MidiFile.h"
#include "MidiEvent.h"
#include "MidiMessage.h"
#include "ini_parser.h"

// 转换 MIDI 音符索引到 Gigatron 引擎支持的范围 (12-105)
int convert_midi_note(int midi_note) {
    return std::max(12, std::min(105, midi_note));
}

// 转换 MIDI 速度到 Gigatron 引擎的音量 (0-63)
int apply_volume_boost(int gigatron_volume, int min_volume_boost) {
    if (gigatron_volume == 0) {
        return 0; // 音量为0时不受抬升参数影响
    }

    // 确保音量不低于最低抬升值，且不超过最大值
    gigatron_volume = std::max(min_volume_boost, gigatron_volume);
    gigatron_volume = std::min(63, gigatron_volume);
    
    return gigatron_volume;
}

// 音量精简函数：将音量值精简为指定的等级数量
int simplify_volume(int volume, int volume_levels) {
    if (volume_levels <= 1) return 0; // 如果只有1个等级，所有音量都为0
    if (volume_levels >= 64) return volume; // 如果等级数大于等于64，不进行精简
    
    // 将0-63的音量范围划分为volume_levels个等级
    // 每个等级的大小为 64/volume_levels
    int level_size = 64 / volume_levels;
    int level = volume / level_size;
    
    // 确保不超出范围
    if (level >= volume_levels) level = volume_levels - 1;
    
    // 返回该等级的中间值，以保持音量的相对关系
    return level * level_size + level_size / 2;
}

// 转换 MIDI 程序号到 Gigatron 引擎的波形
int convert_midi_waveform(int program, IniParser* config_parser = nullptr) {
    // 如果有配置文件，尝试从配置文件获取波形设置
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找wave宏
        for (const auto& macro : config.macros) {
            if (macro.type == "wave" && !macro.on_elements.empty()) {
                // 返回第一个波形值
                if (macro.on_elements[0].type == "value") {
                    return macro.on_elements[0].value;
                }
            }
        }
    }
    
    // 默认转换逻辑
    if (program == 80) { // MIDI 程序 80 -> 方波
        return 2;
    } else if (program == 127) { // MIDI 程序 127 -> 噪音
        return 0;
    } else { // 其他乐器 -> 三角波
        return 1;
    }
}

// 获取乐器配置的精度
int get_instrument_accuracy(int program, IniParser* config_parser = nullptr) {
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        return config.accuracy;
    }
    
    // 返回默认精度
    if (config_parser) {
        return config_parser->getDefaultAccuracy();
    }
    return 30; // 默认精度
}

// 获取乐器配置的音量偏移
int get_instrument_note_offset(int program, IniParser* config_parser = nullptr) {
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找note宏
        for (const auto& macro : config.macros) {
            if (macro.type == "note" && !macro.on_elements.empty()) {
                // 返回第一个音高偏移值
                if (macro.on_elements[0].type == "value") {
                    return macro.on_elements[0].value;
                }
            }
        }
    }
    
    // 默认不偏移
    return 0;
}

// 获取乐器配置的音量序列
std::vector<int> get_instrument_volume_sequence(int program, IniParser* config_parser = nullptr) {
    std::vector<int> result;
    
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找vol宏
        for (const auto& macro : config.macros) {
            if (macro.type == "vol") {
                // 收集所有音量值
                for (const auto& element : macro.on_elements) {
                    if (element.type == "value") {
                        result.push_back(element.value);
                    }
                }
                break;
            }
        }
    }
    
    // 如果没有找到配置，返回默认音量序列
    if (result.empty()) {
        result = {63, 60, 55, 50, 45, 40, 35, 30};
    }
    
    return result;
}

// 获取乐器配置的弯音序列
std::vector<int> get_instrument_pitch_bend_sequence(int program, IniParser* config_parser = nullptr) {
    std::vector<int> result;
    
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找pitch_bend宏
        for (const auto& macro : config.macros) {
            if (macro.type == "pitch_bend") {
                // 收集所有弯音值
                for (const auto& element : macro.on_elements) {
                    if (element.type == "value") {
                        result.push_back(element.value);
                    }
                }
                break;
            }
        }
    }
    
    // 如果没有找到配置，返回默认弯音序列
    if (result.empty()) {
        result = {0, 0, 0, 0, 0, 0, 0, 0};
    }
    
    return result;
}

// 获取乐器配置的波形序列
std::vector<int> get_instrument_waveform_sequence(int program, IniParser* config_parser = nullptr) {
    std::vector<int> result;
    
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找wave宏
        for (const auto& macro : config.macros) {
            if (macro.type == "wave") {
                // 收集所有波形值
                for (const auto& element : macro.on_elements) {
                    if (element.type == "value") {
                        result.push_back(element.value);
                    }
                }
                break;
            }
        }
    }
    
    // 如果没有找到配置，返回默认波形序列
    if (result.empty()) {
        result = {1, 1, 1, 1, 1, 1, 1, 1};
    }
    
    return result;
}

// 获取乐器配置的释放音量序列
std::vector<int> get_instrument_release_volume_sequence(int program, IniParser* config_parser = nullptr) {
    std::vector<int> result;
    
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找release_vol宏
        for (const auto& macro : config.macros) {
            if (macro.type == "release_vol") {
                // 收集所有释放音量值
                for (const auto& element : macro.release_elements) {
                    if (element.type == "value") {
                        result.push_back(element.value);
                    }
                }
                break;
            }
        }
    }
    
    // 如果没有找到配置，返回默认释放音量序列
    if (result.empty()) {
        result = {30, 20, 10, 0};
    } else {
        // 确保释放音量序列的最后一个值是0，以确保音符最终关闭
        if (result.back() != 0) {
            result.push_back(0);
        }
    }
    
    return result;
}

// 获取乐器配置的释放波形序列
std::vector<int> get_instrument_release_waveform_sequence(int program, IniParser* config_parser = nullptr) {
    std::vector<int> result;
    
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找release_wave宏
        for (const auto& macro : config.macros) {
            if (macro.type == "release_wave") {
                // 收集所有释放波形值
                for (const auto& element : macro.release_elements) {
                    if (element.type == "value") {
                        result.push_back(element.value);
                    }
                }
                break;
            }
        }
    }
    
    // 如果没有找到配置，返回默认释放波形序列
    if (result.empty()) {
        result = get_instrument_waveform_sequence(program, config_parser);
    }
    
    return result;
}

// 获取乐器配置的释放弯音序列
std::vector<int> get_instrument_release_pitch_bend_sequence(int program, IniParser* config_parser = nullptr) {
    std::vector<int> result;
    
    if (config_parser && config_parser->hasInstrument(program)) {
        const InstrumentConfig& config = config_parser->getInstrumentConfig(program);
        // 查找release_pitch_bend宏
        for (const auto& macro : config.macros) {
            if (macro.type == "release_pitch_bend") {
                // 收集所有释放弯音值
                for (const auto& element : macro.release_elements) {
                    if (element.type == "value") {
                        result.push_back(element.value);
                    }
                }
                break;
            }
        }
    }
    
    // 如果没有找到配置，返回默认释放弯音序列
    if (result.empty()) {
        result = {0, 0, 0, 0};
    }
    
    return result;
}

// 获取鼓声配置的精度
int get_drum_accuracy(int drum_id, IniParser* config_parser = nullptr) {
    if (config_parser && config_parser->hasDrum(drum_id)) {
        const InstrumentConfig& config = config_parser->getDrumConfig(drum_id);
        return config.accuracy;
    }
    
    // 返回默认精度
    if (config_parser) {
        return config_parser->getDefaultAccuracy();
    }
    return 30; // 默认精度
}

// 获取鼓声配置的波形
int get_drum_waveform(int drum_id, IniParser* config_parser = nullptr) {
    if (config_parser && config_parser->hasDrum(drum_id)) {
        const InstrumentConfig& config = config_parser->getDrumConfig(drum_id);
        // 查找wave宏
        for (const auto& macro : config.macros) {
            if (macro.type == "wave" && !macro.on_elements.empty()) {
                // 返回第一个波形值
                if (macro.on_elements[0].type == "value") {
                    return macro.on_elements[0].value;
                }
            }
        }
    }
    
    // 鼓声默认使用噪音波形
    return 0;
}

// 自定义 MidiEvent 结构体，用于存储解析后的事件信息
struct CustomMidiEvent {
    int channel;        // Gigatron 通道 (1-4)
    int original_midi_channel; // 原始 MIDI 通道 (0-15)
    int note;           // MIDI 音符索引 (0-127)
    double velocity;       // MIDI 速度 (0-127)
    double volume;         // 当前音量（控制器7）
    double expression;     // 当前表情（控制器11）
    int program;        // MIDI 乐器程序号 (0-127)
    long timestamp;     // 事件时间戳，以 MIDI ticks 为单位
    long duration;      // 音符持续时间，以 MIDI ticks 为单位
    bool is_note_off;    // 是否为 Note Off 事件
    double pitch_bend;      // MIDI 弯音轮值 (以半音为单位)
    bool is_velocity_change; // 是否为力度变化事件
    bool is_pitch_bend_change; // 是否为弯音变化事件
    bool is_volume_change; // 是否为音量变化事件
    bool is_macro_event; // 是否为宏事件
    bool is_release_event; // 是否为释放事件
    int modulation;     // MIDI 调制轮值 (CC 1)
    int wave_value;     // 宏事件中的波形值
    std::string macro_type; // 宏类型 (e.g., "vol", "wave", "pitch_bend")
    int macro_value;    // 宏值
    long macro_duration; // 宏步骤的持续时间，以 Gigatron ticks 为单位
    bool is_on_macro; // 是否为音符“on”阶段的宏事件
    bool is_release_macro; // 是否为音符“release”阶段的宏事件
    long note_on_timestamp; // 对应 Note On 事件的时间戳
    long note_off_timestamp; // 对应 Note Off 事件的时间戳
    int note_on_velocity; // 对应 Note On 事件的力度
    int note_on_program; // 对应 Note On 事件的乐器程序号
};

// 音符状态跟踪结构体
struct NoteState {
    int channel;        // Gigatron 通道
    int note;           // MIDI 音符索引
    int velocity;       // 当前力度
    int volume;         // 当前音量（控制器7）
    int expression;     // 当前表情（控制器11）
    int program;        // 当前乐器程序号
    double pitch_bend;  // 当前弯音值 (以半音为单位)
    int modulation;     // 当前调制轮值
    long start_tick;    // 音符开始的 tick
    bool active;        // 音符是否处于活动状态
};

// 单音轨复音FIFO队列项
struct PolyphonyQueueItem {
    int note;           // MIDI 音符索引
    int gigatron_channel; // 分配的Gigatron通道
    long start_tick;    // 音符开始的tick
};

// MIDI 文件解析器接口
class MidiFileParser {
public:
    virtual std::vector<CustomMidiEvent> parse(const std::string& filename, double max_duration_seconds, bool dynamic_allocation = false, bool no_velocity_change = false, int gigatron_ticks_per_second = 60) = 0;
    virtual long get_ppqn() = 0; // 每四分音符的脉冲数
    virtual long get_tempo() = 0; // 每四分音符的微秒数
    virtual ~MidiFileParser() = default;
};

class MidiFileParserImpl : public MidiFileParser {
public:
    // 辅助函数：根据乐器配置和音符状态生成宏事件
    std::pair<std::vector<CustomMidiEvent>, long> generateMacroEvents(
        const NoteState& note_state,
        const InstrumentConfig& instrument_config,
        long note_on_tick,
        long note_off_tick,
        double gigatron_ticks_per_midi_tick,
        int original_midi_channel
    ) {
        std::vector<CustomMidiEvent> macro_events;
        long max_macro_timestamp = note_off_tick; // Initialize with note_off_tick
        int gigatron_channel = note_state.channel;
        int note = note_state.note;
        int program = note_state.program;
        int base_velocity = note_state.velocity;

        // 获取精度
        int accuracy = instrument_config.accuracy;
        if (accuracy <= 0) accuracy = _config_parser->getDefaultAccuracy(); // 使用默认精度

        // 获取宏序列
        std::vector<int> vol_on_sequence;
        std::vector<int> vol_release_sequence;

        for (const auto& macro : instrument_config.macros) {
            if (macro.type == "vol") {
                for (const auto& element : macro.on_elements) {
                    if (element.type == "value") vol_on_sequence.push_back(element.value);
                }
                for (const auto& element : macro.release_elements) {
                    if (element.type == "value") vol_release_sequence.push_back(element.value);
                }
            }
        }

        // 如果没有配置，使用默认值
        if (vol_on_sequence.empty()) vol_on_sequence = {63};
        if (vol_release_sequence.empty()) {
            vol_release_sequence = {0};
        } else {
            // 确保释放音量序列的最后一个值是0，以确保音符最终关闭
            if (vol_release_sequence.back() != 0) {
                vol_release_sequence.push_back(0);
            }
        }

        // 计算每个宏步骤在 MIDI ticks 中的持续时间
        long macro_step_midi_duration = static_cast<long>(std::round(accuracy / gigatron_ticks_per_midi_tick));
        if (macro_step_midi_duration <= 0) macro_step_midi_duration = 1; // 至少1个 MIDI tick

        // --- 生成 ON 阶段的宏事件 ---
        for (size_t i = 0; i < vol_on_sequence.size(); ++i) {
            long macro_start_midi_tick = note_on_tick + static_cast<long>(i * macro_step_midi_duration);
            if (macro_start_midi_tick >= note_off_tick) break; // 超过 Note Off 时间

            // Volume macro event
            if (i < vol_on_sequence.size()) {
                CustomMidiEvent macro_event;
                macro_event.channel = gigatron_channel;
                macro_event.original_midi_channel = original_midi_channel;
                macro_event.note = note;
                macro_event.velocity = base_velocity;
                macro_event.program = program;
                macro_event.timestamp = macro_start_midi_tick;
                macro_event.duration = 0;
                macro_event.is_note_off = false;
                macro_event.is_velocity_change = false;
                macro_event.is_pitch_bend_change = false;
                macro_event.is_volume_change = false;
                macro_event.is_macro_event = true;
                macro_event.is_release_event = false;
                macro_event.modulation = note_state.modulation;
                macro_event.wave_value = -1; // Not setting wave here
                macro_event.macro_type = "vol";
                macro_event.macro_value = vol_on_sequence[i];
                macro_event.macro_duration = accuracy; // 宏步骤的持续时间，以 Gigatron ticks 为单位
                macro_event.is_on_macro = true;
                macro_event.is_release_macro = false;
                macro_event.note_on_timestamp = note_on_tick;
                macro_event.note_off_timestamp = note_off_tick;
                macro_event.note_on_velocity = base_velocity;
                macro_event.note_on_program = program;
                macro_events.push_back(macro_event);
                max_macro_timestamp = std::max(max_macro_timestamp, macro_event.timestamp + macro_step_midi_duration);
            }
        }

        // --- 生成 RELEASE 阶段的宏事件 ---
        // 释放阶段从 Note Off 时间开始
        long release_start_midi_tick = note_off_tick;
        // 检查音符持续时间是否足够长以执行释放宏
        long note_duration_midi_ticks = note_off_tick - note_on_tick;
        long min_duration_for_release_macros_gigatron_ticks = 2 * accuracy; // "两个空格"
        long min_duration_for_release_macros_midi_ticks = static_cast<long>(std::round(min_duration_for_release_macros_gigatron_ticks / gigatron_ticks_per_midi_tick));

        if (note_duration_midi_ticks > min_duration_for_release_macros_midi_ticks) {
            for (size_t i = 0; i < vol_release_sequence.size(); ++i) {
            long macro_start_midi_tick = release_start_midi_tick + static_cast<long>(i * macro_step_midi_duration);

            // Volume macro event
            if (i < vol_release_sequence.size()) {
                CustomMidiEvent macro_event;
                macro_event.channel = gigatron_channel;
                macro_event.original_midi_channel = original_midi_channel;
                macro_event.note = note;
                macro_event.velocity = 0; // 释放阶段力度为0
                macro_event.program = program;
                macro_event.timestamp = macro_start_midi_tick;
                macro_event.duration = 0;
                macro_event.is_note_off = false;
                macro_event.is_velocity_change = false;
                macro_event.is_pitch_bend_change = false;
                macro_event.is_volume_change = false;
                macro_event.is_macro_event = true;
                macro_event.is_release_event = true;
                macro_event.modulation = note_state.modulation;
                macro_event.wave_value = -1;
                macro_event.macro_type = "vol";
                macro_event.macro_value = vol_release_sequence[i];
                macro_event.macro_duration = accuracy;
                macro_event.is_on_macro = false;
                macro_event.is_release_macro = true;
                macro_event.note_on_timestamp = note_on_tick;
                macro_event.note_off_timestamp = note_off_tick;
                macro_event.note_on_velocity = base_velocity;
                macro_event.note_on_program = program;
                macro_events.push_back(macro_event);
                max_macro_timestamp = std::max(max_macro_timestamp, macro_event.timestamp + macro_step_midi_duration);
            }
        }
        }

        return {macro_events, max_macro_timestamp};
    }

    std::vector<CustomMidiEvent> parse(const std::string& filename, double max_duration_seconds, bool dynamic_allocation = false, bool no_velocity_change = false, int gigatron_ticks_per_second_param = 60) override {
        std::vector<CustomMidiEvent> events;
        smf::MidiFile midifile;
        midifile.read(filename);

        if (!midifile.status()) {
            std::cerr << "Error: Could not read MIDI file " << filename << std::endl;
            return events;
        }

        midifile.doTimeAnalysis();
        midifile.linkNotePairs();

        _ppqn = midifile.getTicksPerQuarterNote();
        _tempo = 500000; // 默认 tempo 120 BPM (500000 microseconds per quarter note)

        long max_midi_tick = -1;
        if (max_duration_seconds > 0) {
            // 计算 30 秒对应的 MIDI tick
            // 1秒 = 1000000 微秒
            // MIDI tick = (秒数 * PPQN * 1000000) / tempo_us
            // 这里我们先假设一个默认的 tempo，后面会更新
            max_midi_tick = static_cast<long>((max_duration_seconds * _ppqn * 1000000.0) / _tempo);
        }

        // 局部变量 channel_programs 和 channel_pitch_bends 已被替换为成员变量 _channel_programs 和 _channel_pitch_bends
        // _channel_pitch_bends 已在构造函数中初始化
        // const double max_bend_semitones = 2.0; // 弯音轮最大范围 +/- 2 个半音 (现在由 _channel_pitch_bend_range 控制)
        
        // 如果使用动态分配，重置可用通道列表
        if (dynamic_allocation) {
            _available_gigatron_channels = {1, 2, 3, 4};
            _midi_channel_to_gigatron_channel_map.clear();
            _gigatron_channel_last_note_on_tick.clear();
            _midi_channel_polyphony_queue.clear();
            _gigatron_channel_usage.clear();
            // 初始化Gigatron通道使用情况
            for (int ch = 1; ch <= 4; ch++) {
                _gigatron_channel_usage[ch] = std::vector<int>();
            }
        }

        // 清空音符状态跟踪
        _active_notes.clear();

        for (int track = 0; track < midifile.getNumTracks(); ++track) {
            for (int i = 0; i < midifile[track].size(); ++i) {
                smf::MidiEvent& event = midifile[track][i];
                smf::MidiMessage& message = event; // MidiEvent 继承自 MidiMessage
                _debug_log << "Event Tick: " << event.tick << ", Command Byte: " << std::hex << (int)message.getCommandByte() << std::dec << ", Command Nibble: " << (int)message.getCommandNibble();
                _debug_log << ", Message Bytes: ";
                for (size_t k = 0; k < message.size(); ++k) {
                    _debug_log << std::hex << (int)message[k] << " " << std::dec;
                }
                _debug_log << std::endl;

                if (message.isTempo()) {
                    _tempo = message.getTempoMicroseconds();
                    // 更新 max_midi_tick因为 tempo 可能改变
                    if (max_duration_seconds > 0) {
                        max_midi_tick = static_cast<long>((max_duration_seconds * _ppqn * 1000000.0) / _tempo);
                    }
                }

                // 如果设置了最大时长，并且当前事件的时间戳超过了最大 MIDI tick，则停止处理
                if (max_midi_tick != -1 && event.tick > max_midi_tick) {
                    break; // 跳出当前轨道的循环
                }

                if (message.isNoteOn()) {
                    int midi_channel = message.getChannel();
                    int gigatron_channel = -1; // Initialize to an invalid value
                    // int note = message.getKeyNumber(); // 移动到下面

                    if (dynamic_allocation) {
                        // 动态分配模式
                        // 查找一个可用的Gigatron通道
                        // 优先选择当前未被任何MIDI通道占用的Gigatron通道
                        // 如果所有通道都被占用，则选择最早可用的通道（即宏执行结束最早的通道）
                        
                        // 动态分配模式
                        // 查找一个可用的Gigatron通道
                        // 优先按顺序选择当前未被任何MIDI通道占用的Gigatron通道，且其宏执行已结束
                        // 如果所有通道都被占用，则选择最早可用的通道（即宏执行结束最早的通道）

                        long earliest_available_tick = -1; // 声明一次
                        int candidate_gigatron_channel = -1; // 声明一次
                        bool found_free_channel = false;

                        // 1. 尝试找到一个当前“真正空闲”的Gigatron通道 (按顺序1-4)
                        for (int ch = 1; ch <= 4; ++ch) {
                            bool is_mapped_to_any_midi_channel = false;
                            for (auto const& [m_chan, g_chan] : _midi_channel_to_gigatron_channel_map) {
                                if (g_chan == ch) {
                                    is_mapped_to_any_midi_channel = true;
                                    break;
                                }
                            }
                            // 如果通道未被映射，且其宏执行已结束 (或从未被使用过)
                            if (!is_mapped_to_any_midi_channel && _gigatron_channel_available_at_tick[ch] <= event.tick) {
                                candidate_gigatron_channel = ch;
                                found_free_channel = true;
                                break; // 找到第一个空闲通道，按顺序优先
                            }
                        }

                        if (found_free_channel) {
                            // 找到了一个真正空闲的通道
                            gigatron_channel = candidate_gigatron_channel;
                            _midi_channel_to_gigatron_channel_map[midi_channel] = gigatron_channel;
                            _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                            _debug_log << "[DYNAMIC] Assigned new Gigatron Channel " << gigatron_channel << " to MIDI Channel " << midi_channel << " (free and sequential)" << std::endl;
                        } else {
                            // 没有找到真正空闲的通道，所有Gigatron通道都被占用或其宏事件尚未结束
                            // 执行FIFO策略：找到最早可用的Gigatron通道进行替换
                            for (int ch = 1; ch <= 4; ++ch) {
                                if (candidate_gigatron_channel == -1 || _gigatron_channel_available_at_tick[ch] < earliest_available_tick) {
                                    earliest_available_tick = _gigatron_channel_available_at_tick[ch];
                                    candidate_gigatron_channel = ch;
                                }
                            }

                            // 如果当前事件的开始时间早于最早可用时间，则忽略此Note On事件
                            // 否则，进行通道替换
                            if (event.tick < earliest_available_tick) {
                                _debug_log << "[DYNAMIC] Skipping Note On for MIDI Channel " << midi_channel << " (no available Gigatron channel before event tick " << event.tick << ")" << std::endl;
                                continue; // 跳过此事件
                            }

                            gigatron_channel = candidate_gigatron_channel;
                            // 找到被替换的MIDI通道
                            int midi_channel_to_evict = -1;
                            for (auto const& [m_chan, g_chan] : _midi_channel_to_gigatron_channel_map) {
                                if (g_chan == gigatron_channel) {
                                    midi_channel_to_evict = m_chan;
                                    break;
                                }
                            }
                            if (midi_channel_to_evict != -1) {
                                _debug_log << "[DYNAMIC] Evicting MIDI Channel " << midi_channel_to_evict << " from Gigatron Channel " << gigatron_channel << " (earliest available at tick: " << earliest_available_tick << ")" << std::endl;
                                _midi_channel_to_gigatron_channel_map.erase(midi_channel_to_evict);
                                _gigatron_channel_last_note_on_tick.erase(gigatron_channel);
                            }
                            _midi_channel_to_gigatron_channel_map[midi_channel] = gigatron_channel;
                            _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                            _debug_log << "[DYNAMIC] Assigned Gigatron Channel " << gigatron_channel << " to new MIDI Channel " << midi_channel << " (reassigned via FIFO)" << std::endl;
                        }
                    } else {
                        // 静态分配模式
                        if (_midi_channel_to_gigatron_channel_map.count(midi_channel)) {
                            gigatron_channel = _midi_channel_to_gigatron_channel_map[midi_channel];
                            _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                        } else {
                            if (_midi_channel_to_gigatron_channel_map.size() < 4) {
                                gigatron_channel = _midi_channel_to_gigatron_channel_map.size() + 1;
                                _midi_channel_to_gigatron_channel_map[midi_channel] = gigatron_channel;
                                _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                                _debug_log << "[STATIC] Mapped MIDI Channel " << midi_channel << " to Gigatron Channel " << gigatron_channel << std::endl;
                            } else {
                                _debug_log << "[STATIC] Skipping MIDI Channel " << midi_channel << " (already have 4 channels mapped in static mode)" << std::endl;
                                continue;
                            }
                        }
                    }

                    // 如果没有分配到有效的Gigatron通道，则跳过此事件
                    if (gigatron_channel == -1) {
                        continue;
                    }
                    
                    int note = message.getKeyNumber(); // 移动到这里

                    // 创建音符状态跟踪
                    std::string note_key = std::to_string(gigatron_channel) + "_" + std::to_string(note);
                    NoteState note_state;
                    note_state.channel = gigatron_channel;
                    note_state.note = note;
                    note_state.velocity = message.getVelocity();
                    note_state.volume = _channel_volumes[midi_channel];
                    note_state.expression = _channel_expressions[midi_channel];
                    note_state.program = _channel_programs[midi_channel];
                    note_state.pitch_bend = _channel_pitch_bends[midi_channel]; // 以半音为单位
                    note_state.modulation = _channel_modulations[midi_channel]; // 设置当前调制轮值
                    note_state.start_tick = event.tick;
                    note_state.active = true;
                    _active_notes[note_key] = note_state;

                    CustomMidiEvent new_event;
                    new_event.channel = gigatron_channel;
                    new_event.original_midi_channel = midi_channel; // 添加原始 MIDI 通道
                    new_event.note = note;
                    new_event.velocity = message.getVelocity();
                    new_event.program = _channel_programs[midi_channel]; // Use current program for this MIDI channel
                    new_event.timestamp = event.tick;
                    new_event.duration = event.getTickDuration();
                    new_event.is_note_off = false; // Note On 事件
                    new_event.pitch_bend = _channel_pitch_bends[midi_channel]; // Use current pitch bend for this MIDI channel
                    new_event.is_velocity_change = false;
                    new_event.is_pitch_bend_change = false;
                    new_event.volume = _channel_volumes[midi_channel]; // 设置当前音量
                    new_event.expression = _channel_expressions[midi_channel]; // 设置当前表情
                    new_event.is_volume_change = false; // Note On 事件本身不是音量变化事件
                    new_event.is_macro_event = false; // 默认不是宏事件
                    new_event.is_release_event = false; // 默认不是释放事件
                    new_event.modulation = _channel_modulations[midi_channel]; // 设置当前调制轮值
                    new_event.wave_value = -1; // 初始化波形值
                    events.push_back(new_event);

                    // 如果乐器有配置，生成宏事件
                    if (_config_parser && _config_parser->hasInstrument(new_event.program)) {
                        const InstrumentConfig& instrument_config = _config_parser->getInstrumentConfig(new_event.program);
                        std::pair<std::vector<CustomMidiEvent>, long> macro_result = generateMacroEvents(
                            note_state,
                            instrument_config,
                            event.tick,
                            event.getLinkedEvent()->tick, // Note Off 的时间戳
                            (static_cast<double>(_tempo) * gigatron_ticks_per_second_param) / (static_cast<double>(_ppqn) * 1000000.0), // 计算 gigatron_ticks_per_midi_tick
                            midi_channel
                        );
                        // 将生成的宏事件添加到主事件列表中
                        events.insert(events.end(), macro_result.first.begin(), macro_result.first.end());
                        // 更新 Gigatron 通道可用时间
                        _gigatron_channel_available_at_tick[gigatron_channel] = std::max(_gigatron_channel_available_at_tick[gigatron_channel], macro_result.second);
                    }

                } else if (message.isNoteOff()) {
                    int midi_channel = message.getChannel();
                    int note = message.getKeyNumber();
                    
                    if (dynamic_allocation && _midi_channel_polyphony_queue.count(midi_channel)) {
                        // 在动态分配模式下，从复音队列中查找并移除对应的音符
                        auto& queue = _midi_channel_polyphony_queue[midi_channel];
                        int gigatron_channel = -1;
                        
                        for (auto it = queue.begin(); it != queue.end(); ++it) {
                            if (it->note == note) {
                                gigatron_channel = it->gigatron_channel;
                                queue.erase(it);
                                _debug_log << "[POLYPHONY] Removed note " << note << " from Gigatron Channel " << gigatron_channel << " on MIDI Channel " << midi_channel << std::endl;
                                break;
                            }
                        }
                        
                        if (gigatron_channel != -1) {
                            // 移除音符状态跟踪
                            std::string note_key = std::to_string(gigatron_channel) + "_" + std::to_string(note);
                            if (_active_notes.count(note_key)) {
                                _active_notes.erase(note_key);
                            }
                            
                            CustomMidiEvent new_event;
                            new_event.channel = gigatron_channel;
                            new_event.original_midi_channel = midi_channel; // 添加原始 MIDI 通道
                            new_event.note = note;
                            new_event.velocity = 0; // Note Off 事件的音量为 0
                            new_event.program = _channel_programs[midi_channel]; // Use current program for this MIDI channel
                            new_event.timestamp = event.tick;
                            new_event.duration = 0; // Note Off 事件没有持续时间
                            new_event.is_note_off = true; // Note Off 事件
                            new_event.pitch_bend = _channel_pitch_bends[midi_channel]; // Use current pitch bend for this MIDI channel
                            new_event.is_velocity_change = false;
                            new_event.is_pitch_bend_change = false;
                            new_event.is_macro_event = false; // 默认不是宏事件
                            new_event.is_release_event = false; // 默认不是释放事件
                            new_event.modulation = _channel_modulations[midi_channel]; // 设置当前调制轮值
                            new_event.wave_value = -1; // 初始化波形值
                            events.push_back(new_event);
                        }
                    } else if (_midi_channel_to_gigatron_channel_map.count(midi_channel)) {
                        // 非动态分配模式或复音队列不存在，使用原始逻辑
                        int gigatron_channel = _midi_channel_to_gigatron_channel_map[midi_channel];
                        
                        // 移除音符状态跟踪
                        std::string note_key = std::to_string(gigatron_channel) + "_" + std::to_string(note);
                        if (_active_notes.count(note_key)) {
                            _active_notes.erase(note_key);
                        }
                        
                        CustomMidiEvent new_event;
                        new_event.channel = gigatron_channel;
                        new_event.original_midi_channel = midi_channel; // 添加原始 MIDI 通道
                        new_event.note = note;
                        new_event.velocity = 0; // Note Off 事件的音量为 0
                        new_event.program = _channel_programs[midi_channel]; // Use current program for this MIDI channel
                        new_event.timestamp = event.tick;
                        new_event.duration = 0; // Note Off 事件没有持续时间
                        new_event.is_note_off = true; // Note Off 事件
                        new_event.pitch_bend = _channel_pitch_bends[midi_channel]; // Use current pitch bend for this MIDI channel
                        new_event.is_velocity_change = false;
                        new_event.is_pitch_bend_change = false;
                        new_event.is_macro_event = false; // 默认不是宏事件
                        new_event.is_release_event = false; // 默认不是释放事件
                        new_event.modulation = _channel_modulations[midi_channel]; // 设置当前调制轮值
                        new_event.wave_value = -1; // 初始化波形值
                        events.push_back(new_event);
                    }
                } else if (message.getCommandNibble() == 0xE0 && message.getChannel() < 16) {
                    // MIDI 弯音轮范围是 -8192 到 8191，但通常只使用 -8192 到 8191
                    int raw_bend_value = message.getP2() * 128 + message.getP1();
                    int bend_value_centered = raw_bend_value - 8192; // 将中心值 8192 映射到 0
                    // 根据弯音轮灵敏度计算实际的半音变化
                    double actual_semitone_bend = (static_cast<double>(bend_value_centered) / 8192.0) * _channel_pitch_bend_range[message.getChannel()];
                    _debug_log << "Pitch Bend Change on channel " << message.getChannel() << ": raw=" << raw_bend_value << ", centered=" << bend_value_centered << ", actual_semitone_bend=" << actual_semitone_bend << std::endl;
                    
                    double old_bend = _channel_pitch_bends[message.getChannel()];
                    _channel_pitch_bends[message.getChannel()] = actual_semitone_bend; // 存储以半音为单位的弯音值
                    
                    // 如果弯音值发生变化，为所有活动音符生成弯音变化事件
                    if (std::abs(old_bend - actual_semitone_bend) > 0.001 && _midi_channel_to_gigatron_channel_map.count(message.getChannel())) {
                        int gigatron_channel = _midi_channel_to_gigatron_channel_map[message.getChannel()];
                        
                        // 为该通道的所有活动音符生成弯音变化事件
                        for (auto& note_pair : _active_notes) {
                            std::string note_key = note_pair.first;
                            NoteState& note_state = note_pair.second;
                            if (note_state.channel == gigatron_channel && note_state.active) {
                                CustomMidiEvent bend_event;
                                bend_event.channel = gigatron_channel;
                                bend_event.original_midi_channel = message.getChannel(); // 添加原始 MIDI 通道
                                bend_event.note = note_state.note;
                                bend_event.velocity = note_state.velocity;
                                bend_event.program = note_state.program;
                                bend_event.timestamp = event.tick;
                                bend_event.duration = 0;
                                bend_event.is_note_off = false;
                                bend_event.pitch_bend = actual_semitone_bend; // 以半音为单位
                                bend_event.is_velocity_change = false;
                                bend_event.is_pitch_bend_change = true;
                                bend_event.is_macro_event = false; // 默认不是宏事件
                                bend_event.is_release_event = false; // 默认不是释放事件
                                bend_event.modulation = note_state.modulation; // 保持调制轮值
                                bend_event.wave_value = -1; // 初始化波形值
                                events.push_back(bend_event);
                                
                                // 更新音符状态中的弯音值
                                note_state.pitch_bend = actual_semitone_bend;
                            }
                        }
                    }
                } else if (message.isPatchChange()) {
                    _channel_programs[message.getChannel()] = message.getP1();
                } else if (message.isController()) {
                    // 处理控制器事件
                    int controller_number = message.getP1();
                    int controller_value = message.getP2();
                    int midi_channel = message.getChannel();
                    
                    _debug_log << "Controller Change on MIDI Channel " << midi_channel
                               << ": Controller " << controller_number << " = " << controller_value << std::endl;
                    
                    // 更新通道的控制器值
                    if (controller_number == 7) {
                        // 音量控制器（Volume）
                        _channel_volumes[midi_channel] = controller_value;
                        _debug_log << "Updated Volume for MIDI Channel " << midi_channel
                                   << " to " << controller_value << std::endl;
                    } else if (controller_number == 11) {
                        // 表情控制器（Expression）
                        _channel_expressions[midi_channel] = controller_value;
                        _debug_log << "Updated Expression for MIDI Channel " << midi_channel
                                   << " to " << controller_value << std::endl;
                    } else if (controller_number == 1) {
                        // 调制轮（Modulation Wheel）
                        _channel_modulations[midi_channel] = controller_value;
                        _debug_log << "Updated Modulation for MIDI Channel " << midi_channel
                                   << " to " << controller_value << std::endl;
                    } else if (controller_number == 101) { // RPN MSB
                        _rpn_msb[midi_channel] = controller_value;
                    } else if (controller_number == 100) { // RPN LSB
                        _rpn_lsb[midi_channel] = controller_value;
                    } else if (controller_number == 6) { // Data Entry MSB
                        _data_entry_msb[midi_channel] = controller_value;
                        // 如果是 RPN 0 (Pitch Bend Range)，则更新弯音轮灵敏度
                        if (_rpn_msb[midi_channel] == 0 && _rpn_lsb[midi_channel] == 0) {
                            double new_range = _data_entry_msb[midi_channel] + (_data_entry_lsb[midi_channel] != -1 ? _data_entry_lsb[midi_channel] / 100.0 : 0.0);
                            _channel_pitch_bend_range[midi_channel] = std::min(72.0, std::max(0.0, new_range));
                            _debug_log << "Updated Pitch Bend Range for MIDI Channel " << midi_channel
                                       << " to " << _channel_pitch_bend_range[midi_channel] << " semitones." << std::endl;
                        }
                    } else if (controller_number == 38) { // Data Entry LSB
                        _data_entry_lsb[midi_channel] = controller_value;
                        // 如果是 RPN 0 (Pitch Bend Range)，则更新弯音轮灵敏度
                        if (_rpn_msb[midi_channel] == 0 && _rpn_lsb[midi_channel] == 0) {
                            double new_range = (_data_entry_msb[midi_channel] != -1 ? _data_entry_msb[midi_channel] : 0.0) + _data_entry_lsb[midi_channel] / 100.0;
                            _channel_pitch_bend_range[midi_channel] = std::min(72.0, std::max(0.0, new_range));
                            _debug_log << "Updated Pitch Bend Range for MIDI Channel " << midi_channel
                                       << " to " << _channel_pitch_bend_range[midi_channel] << " semitones." << std::endl;
                        }
                    }
                    
                    // 如果该MIDI通道已映射到Gigatron通道，为所有活动音符生成音量/表情/调制变化事件
                    // 但如果设置了no_velocity_change，则不生成音量变化事件
                    if (_midi_channel_to_gigatron_channel_map.count(midi_channel) && !no_velocity_change) {
                        int gigatron_channel = _midi_channel_to_gigatron_channel_map[midi_channel];
                        
                        // 为该通道的所有活动音符生成事件
                        for (auto& note_pair : _active_notes) {
                            std::string note_key = note_pair.first;
                            NoteState& note_state = note_pair.second;
                            if (note_state.channel == gigatron_channel && note_state.active) {
                                CustomMidiEvent controller_change_event;
                                controller_change_event.channel = gigatron_channel;
                                controller_change_event.original_midi_channel = midi_channel; // 添加原始 MIDI 通道
                                controller_change_event.note = note_state.note;
                                controller_change_event.velocity = note_state.velocity;
                                controller_change_event.volume = _channel_volumes[midi_channel];
                                controller_change_event.expression = _channel_expressions[midi_channel];
                                controller_change_event.program = note_state.program;
                                controller_change_event.timestamp = event.tick;
                                controller_change_event.duration = 0;
                                controller_change_event.is_note_off = false;
                                controller_change_event.pitch_bend = note_state.pitch_bend;
                                controller_change_event.is_velocity_change = false;
                                controller_change_event.is_pitch_bend_change = false;
                                controller_change_event.is_macro_event = false; // 默认不是宏事件
                                controller_change_event.is_release_event = false; // 默认不是释放事件
                                controller_change_event.modulation = _channel_modulations[midi_channel]; // 设置当前调制轮值
                                controller_change_event.wave_value = -1; // 初始化波形值
                                
                                // 标记为音量变化事件，但实际上也包含了表情和调制轮的变化
                                controller_change_event.is_volume_change = true;
                                events.push_back(controller_change_event);
                                
                                // 更新音符状态中的音量、表情和调制值
                                note_state.volume = _channel_volumes[midi_channel];
                                note_state.expression = _channel_expressions[midi_channel];
                                note_state.modulation = _channel_modulations[midi_channel];
                            }
                        }
                    }
                }
            }
        }
        return events;
    }

    long get_ppqn() override { return _ppqn; }
    long get_tempo() override { return _tempo; }

private:
    long _ppqn = 0;
    long _tempo = 500000; // 默认 tempo 120 BPM
    std::ofstream _debug_log; // 添加 debug_log 成员变量
    std::map<int, int> _midi_channel_to_gigatron_channel_map; // MIDI通道到Gigatron通道的映射
    std::map<int, long> _gigatron_channel_last_note_on_tick; // 存储Gigatron通道上次Note On的tick
    std::map<int, int> _channel_programs; // 存储每个通道当前的乐器程序号
    std::map<int, int> _channel_volumes; // 存储每个通道当前的音量（控制器7）
    std::map<int, int> _channel_expressions; // 存储每个通道当前的表情（控制器11）
    std::map<int, int> _channel_modulations; // 存储每个通道当前的调制轮值 (CC 1)
    std::map<int, double> _channel_pitch_bends; // 存储每个通道当前的弯音轮值 (以半音为单位)
    std::map<int, double> _channel_pitch_bend_range; // 存储每个通道当前的弯音轮灵敏度 (半音)
    int _rpn_msb[16]; // 存储每个通道的 RPN MSB
    int _rpn_lsb[16]; // 存储每个通道的 RPN LSB
    int _data_entry_msb[16]; // 存储每个通道的 Data Entry MSB
    int _data_entry_lsb[16]; // 存储每个通道的 Data Entry LSB
    std::vector<int> _available_gigatron_channels = {1, 2, 3, 4}; // 存储可用的Gigatron通道 (1-4)
    std::map<std::string, NoteState> _active_notes; // 存储当前活动的音符状态
    std::map<int, std::vector<PolyphonyQueueItem>> _midi_channel_polyphony_queue; // 每个MIDI通道的复音FIFO队列
    std::map<int, std::vector<int>> _gigatron_channel_usage; // 每个Gigatron通道被哪些MIDI通道使用
    std::map<int, long> _gigatron_channel_available_at_tick; // 存储Gigatron通道何时可用
    IniParser* _config_parser = nullptr; // 配置解析器指针

public:
    MidiFileParserImpl(IniParser* config_parser_ptr = nullptr) : _config_parser(config_parser_ptr) {
        _debug_log.open("debug_midi_converter.log", std::ios_base::trunc); // 在构造函数中打开文件，每次运行清空
        if (!_debug_log.is_open()) {
            std::cerr << "Error: Could not open debug log file." << std::endl;
        } else {
            _debug_log << "Debug log opened." << std::endl;
        }
        // 初始化 _config_parser
        _config_parser = config_parser_ptr;
        // 初始化Gigatron通道映射
        // _available_gigatron_channels 已经在声明时初始化
        // _channel_pitch_bends 初始化所有通道的弯音轮值为 0.0
        // _channel_volumes 初始化所有通道的音量为 127（最大）
        // _channel_expressions 初始化所有通道的表情为 127（最大）
        for (int i = 0; i < 16; ++i) {
            _channel_pitch_bends[i] = 0.0; // 默认弯音值为 0 半音
            _channel_volumes[i] = 127; // 默认音量为 127
            _channel_expressions[i] = 127; // 默认表情为 127
            _channel_modulations[i] = 0; // 默认调制轮值为 0
            _channel_pitch_bend_range[i] = 2.0; // 默认弯音轮灵敏度为 2 半音
            _rpn_msb[i] = -1; // 初始化 RPN MSB
            _rpn_lsb[i] = -1; // 初始化 RPN LSB
            _data_entry_msb[i] = -1; // 初始化 Data Entry MSB
            _data_entry_lsb[i] = -1; // 初始化 Data Entry LSB
        }
        for (int i = 1; i <= 4; ++i) {
            _gigatron_channel_available_at_tick[i] = 0; // 初始化为0
        }
    }

    ~MidiFileParserImpl() {
        if (_debug_log.is_open()) {
            _debug_log << "Debug log closed." << std::endl;
            _debug_log.close(); // 在析构函数中关闭文件
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <midi_file_path>" << std::endl;
        return 1;
    }

    std::string midi_filepath = argv[1];
    double max_duration_seconds = 10.0; // 解析前10秒

    IniParser config_parser; // 创建一个 IniParser 实例
    // 如果需要加载配置文件，可以在这里调用 config_parser.parse("midi_config.ini");

    MidiFileParserImpl parser(&config_parser); // 传递 IniParser 实例的指针
    std::vector<CustomMidiEvent> midi_events = parser.parse(midi_filepath, max_duration_seconds, true); // 启用动态分配

    long ppqn = parser.get_ppqn();
    long tempo_us = parser.get_tempo(); // 微秒/四分音符

    // 计算每个 MIDI tick 对应的秒数
    double seconds_per_midi_tick = static_cast<double>(tempo_us) / (static_cast<double>(ppqn) * 1000000.0);

    std::cout << "Parsed MIDI Events for " << midi_filepath << " (first " << max_duration_seconds << " seconds):" << std::endl;
    std::cout << "PPQN: " << ppqn << ", Tempo (us/quarter note): " << tempo_us << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;

    for (const auto& event : midi_events) {
        double event_time_seconds = event.timestamp * seconds_per_midi_tick;
        std::cout << "Time: " << std::fixed << std::setprecision(3) << event_time_seconds << "s (Tick: " << event.timestamp << ") | ";
        std::cout << "GigaCh: " << event.channel << " (MIDICh: " << event.original_midi_channel << ") | ";
        
        if (event.is_macro_event) {
            std::cout << "MACRO: Type=" << event.macro_type << ", Value=" << event.macro_value << ", Duration=" << event.macro_duration << " GigaTicks | ";
            std::cout << "Note: " << event.note << ", Vel: " << event.velocity << ", Prog: " << event.program << std::endl;
        } else if (event.is_note_off) {
            std::cout << "NOTE_OFF: Note=" << event.note << ", Prog=" << event.program << std::endl;
        } else if (event.is_pitch_bend_change) {
            std::cout << "PITCH_BEND_CHANGE: Note=" << event.note << ", Bend=" << std::fixed << std::setprecision(2) << event.pitch_bend << " semitones" << std::endl;
        } else if (event.is_volume_change) {
            std::cout << "VOLUME_CHANGE: Note=" << event.note << ", Vol=" << event.volume << ", Exp=" << event.expression << ", Mod=" << event.modulation << std::endl;
        } else { // Note On event
            std::cout << "NOTE_ON: Note=" << event.note << ", Vel=" << event.velocity << ", Prog=" << event.program << ", Dur=" << event.duration << " ticks | ";
            std::cout << "Vol=" << event.volume << ", Exp=" << event.expression << ", Bend=" << std::fixed << std::setprecision(2) << event.pitch_bend << " semitones" << std::endl;
        }
    }

    return 0;
}