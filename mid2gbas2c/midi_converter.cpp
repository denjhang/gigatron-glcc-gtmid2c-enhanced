#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath> // For std::round
#include <map>
#include <fstream> // Include fstream for file operations

#include "midifile-master/include/MidiFile.h"
#include "midifile-master/include/MidiEvent.h"
#include "midifile-master/include/MidiMessage.h"
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
    virtual std::vector<CustomMidiEvent> parse(const std::string& filename, double max_duration_seconds, bool dynamic_allocation = false, bool no_velocity_change = false, IniParser* config_parser = nullptr) = 0;
    virtual long get_ppqn() = 0; // 每四分音符的脉冲数
    virtual long get_tempo() = 0; // 每四分音符的微秒数
    virtual ~MidiFileParser() = default;
};

class MidiFileParserImpl : public MidiFileParser {
public:
    std::vector<CustomMidiEvent> parse(const std::string& filename, double max_duration_seconds, bool dynamic_allocation = false, bool no_velocity_change = false, IniParser* /*config_parser*/ = nullptr) override {
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
                    int gigatron_channel;
                    int note = message.getKeyNumber();

                    if (_midi_channel_to_gigatron_channel_map.count(midi_channel)) {
                        // MIDI channel is already mapped
                        gigatron_channel = _midi_channel_to_gigatron_channel_map[midi_channel];
                        _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                    } else {
                        // MIDI channel is not yet mapped
                        if (dynamic_allocation) {
                            // 动态分配模式：智能处理单音轨复音
                            if (_midi_channel_to_gigatron_channel_map.size() < 4) {
                                // Assign a new Gigatron channel
                                gigatron_channel = _available_gigatron_channels.front();
                                _available_gigatron_channels.erase(_available_gigatron_channels.begin());
                                _midi_channel_to_gigatron_channel_map[midi_channel] = gigatron_channel;
                                _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                                _debug_log << "[DYNAMIC] Assigned new Gigatron Channel " << gigatron_channel << " to MIDI Channel " << midi_channel << std::endl;
                            } else {
                                // All 4 Gigatron channels are in use, apply intelligent allocation for polyphony
                                
                                // 检查当前MIDI通道是否已有活动音符
                                bool midi_channel_has_active_notes = false;
                                for (auto const& [note_key, note_state] : _active_notes) {
                                    if (note_state.active && note_state.channel == midi_channel) {
                                        midi_channel_has_active_notes = true;
                                        break;
                                    }
                                }
                                
                                if (midi_channel_has_active_notes) {
                                    // 当前MIDI通道已有活动音符，需要智能分配
                                    // 优先替换最久未使用的Gigatron通道
                                    int oldest_gigatron_channel = -1;
                                    long min_tick = -1;

                                    for (auto const& [g_chan, tick] : _gigatron_channel_last_note_on_tick) {
                                        if (oldest_gigatron_channel == -1 || tick < min_tick) {
                                            min_tick = tick;
                                            oldest_gigatron_channel = g_chan;
                                        }
                                    }

                                    int midi_channel_to_evict = -1;
                                    for (auto const& [m_chan, g_chan] : _midi_channel_to_gigatron_channel_map) {
                                        if (g_chan == oldest_gigatron_channel) {
                                            midi_channel_to_evict = m_chan;
                                            break;
                                        }
                                    }

                                    _debug_log << "[DYNAMIC] Evicting MIDI Channel " << midi_channel_to_evict << " from Gigatron Channel " << oldest_gigatron_channel << " (oldest note on tick: " << min_tick << ")" << std::endl;
                                    _midi_channel_to_gigatron_channel_map.erase(midi_channel_to_evict);
                                    _gigatron_channel_last_note_on_tick.erase(oldest_gigatron_channel);

                                    gigatron_channel = oldest_gigatron_channel;
                                    _midi_channel_to_gigatron_channel_map[midi_channel] = gigatron_channel;
                                    _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                                    _debug_log << "[DYNAMIC] Assigned Gigatron Channel " << gigatron_channel << " to new MIDI Channel " << midi_channel << " via FIFO (polyphony)" << std::endl;
                                } else {
                                    // 当前MIDI通道没有活动音符，可以安全替换
                                    // 使用FIFO策略
                                    int oldest_gigatron_channel = -1;
                                    long min_tick = -1;

                                    for (auto const& [g_chan, tick] : _gigatron_channel_last_note_on_tick) {
                                        if (oldest_gigatron_channel == -1 || tick < min_tick) {
                                            min_tick = tick;
                                            oldest_gigatron_channel = g_chan;
                                        }
                                    }

                                    int midi_channel_to_evict = -1;
                                    for (auto const& [m_chan, g_chan] : _midi_channel_to_gigatron_channel_map) {
                                        if (g_chan == oldest_gigatron_channel) {
                                            midi_channel_to_evict = m_chan;
                                            break;
                                        }
                                    }

                                    _debug_log << "[DYNAMIC] Evicting MIDI Channel " << midi_channel_to_evict << " from Gigatron Channel " << oldest_gigatron_channel << " (oldest note on tick: " << min_tick << ")" << std::endl;
                                    _midi_channel_to_gigatron_channel_map.erase(midi_channel_to_evict);
                                    _gigatron_channel_last_note_on_tick.erase(oldest_gigatron_channel);

                                    gigatron_channel = oldest_gigatron_channel;
                                    _midi_channel_to_gigatron_channel_map[midi_channel] = gigatron_channel;
                                    _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                                    _debug_log << "[DYNAMIC] Assigned Gigatron Channel " << gigatron_channel << " to new MIDI Channel " << midi_channel << " via FIFO" << std::endl;
                                }
                            }
                        } else {
                            // 静态分配模式：扫描所有MIDI通道，对有音符的通道直接分配
                            if (_midi_channel_to_gigatron_channel_map.size() < 4) {
                                // 还有可用的Gigatron通道，直接分配
                                gigatron_channel = _midi_channel_to_gigatron_channel_map.size() + 1; // 分配下一个可用的Gigatron通道
                                _midi_channel_to_gigatron_channel_map[midi_channel] = gigatron_channel;
                                _gigatron_channel_last_note_on_tick[gigatron_channel] = event.tick;
                                _debug_log << "[STATIC] Mapped MIDI Channel " << midi_channel << " to Gigatron Channel " << gigatron_channel << std::endl;
                            } else {
                                // 已经分配了4个Gigatron通道，忽略新的MIDI通道
                                _debug_log << "[STATIC] Skipping MIDI Channel " << midi_channel << " (already have 4 channels mapped in static mode)" << std::endl;
                                continue;
                            }
                        }
                    }

                    // 处理单音轨复音的FIFO分配
                    if (dynamic_allocation) {
                        // 检查当前MIDI通道是否已有复音队列
                        if (!_midi_channel_polyphony_queue.count(midi_channel)) {
                            _midi_channel_polyphony_queue[midi_channel] = std::vector<PolyphonyQueueItem>();
                        }

                        // 检查当前音符是否已经在队列中
                        bool note_already_in_queue = false;
                        for (const auto& item : _midi_channel_polyphony_queue[midi_channel]) {
                            if (item.note == note) {
                                note_already_in_queue = true;
                                break;
                            }
                        }

                        if (!note_already_in_queue) {
                            // 查找可用的Gigatron通道
                            std::vector<int> available_channels;
                            for (int ch = 1; ch <= 4; ch++) {
                                bool channel_in_use = false;
                                for (const auto& item : _midi_channel_polyphony_queue[midi_channel]) {
                                    if (item.gigatron_channel == ch) {
                                        channel_in_use = true;
                                        break;
                                    }
                                }
                                if (!channel_in_use) {
                                    available_channels.push_back(ch);
                                }
                            }

                            int assigned_channel;
                            if (!available_channels.empty()) {
                                // 有可用通道，使用第一个可用通道
                                assigned_channel = available_channels[0];
                                _debug_log << "[POLYPHONY] Assigned available Gigatron Channel " << assigned_channel << " to note " << note << " on MIDI Channel " << midi_channel << std::endl;
                            } else {
                                // 所有通道都被占用，使用FIFO策略替换最旧的音符
                                auto& queue = _midi_channel_polyphony_queue[midi_channel];
                                if (!queue.empty()) {
                                    auto oldest_item = queue.front();
                                    assigned_channel = oldest_item.gigatron_channel;
                                    queue.erase(queue.begin()); // 移除最旧的项
                                    _debug_log << "[POLYPHONY] Replaced oldest note " << oldest_item.note << " on Gigatron Channel " << assigned_channel << " with new note " << note << " on MIDI Channel " << midi_channel << std::endl;
                                } else {
                                    // 队列为空，使用默认通道
                                    assigned_channel = gigatron_channel;
                                }
                            }

                            // 添加新音符到队列
                            PolyphonyQueueItem new_item;
                            new_item.note = note;
                            new_item.gigatron_channel = assigned_channel;
                            new_item.start_tick = event.tick;
                            _midi_channel_polyphony_queue[midi_channel].push_back(new_item);

                            // 使用分配的通道而不是原始的gigatron_channel
                            gigatron_channel = assigned_channel;
                        }
                    }

                    // 创建音符状态跟踪
                    std::string note_key = std::to_string(gigatron_channel) + "_" + std::to_string(message.getKeyNumber());
                    NoteState note_state;
                    note_state.channel = gigatron_channel;
                    note_state.note = message.getKeyNumber();
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
                    new_event.note = message.getKeyNumber();
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
    IniParser* _config_parser = nullptr; // 配置解析器指针

public:
    MidiFileParserImpl(IniParser* config_parser = nullptr) : _config_parser(config_parser) {
        _debug_log.open("debug_midi_converter.log", std::ios_base::trunc); // 在构造函数中打开文件，每次运行清空
        if (!_debug_log.is_open()) {
            std::cerr << "Error: Could not open debug log file." << std::endl;
        } else {
            _debug_log << "Debug log opened." << std::endl;
        }
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
    }

    ~MidiFileParserImpl() {
        if (_debug_log.is_open()) {
            _debug_log << "Debug log closed." << std::endl;
            _debug_log.close(); // 在析构函数中关闭文件
        }
    }
};
int main(int argc, char* argv[]) {
    double max_duration_seconds = -1.0; // 默认不限制时长
    double pitch_bend_multiplier = 1.0; // 默认弯音轮放大倍数为 1.0
    bool dynamic_allocation = false; // 默认不使用动态分配
    int gigatron_ticks_per_second = 60; // 默认 Gigatron tick 精度为 60 (1/60 秒)
    int min_volume_boost = 0; // 默认最低音量抬升为 0
    int timer_compensation_target = 60; // 默认定时器补偿目标为 60
    bool no_pitch_bend = false; // 默认不禁用弯音和颤音
    bool no_velocity_change = false; // 默认不禁用力度变化
    double speed_multiplier = 1.0; // 默认速度倍数为 1.0 (正常速度)
    int default_volume_levels = 64; // 默认音量等级为64（不精简）
    int cmd_volume_levels = -1; // 命令行指定的音量等级，-1表示未指定
    
    // 通道波形强制指定参数
    int channel_waveforms[5] = {-1, -1, -1, -1, -1}; // 索引1-4对应通道1-4，-1表示使用默认波形
    
    // 配置文件参数
    std::string config_file = ""; // 默认不使用配置文件
    IniParser* config_parser = nullptr; // 配置解析器指针

    // 存储每个 (Gigatron Channel, Note) 对的最终 Note Off 时间点
    std::map<std::pair<int, int>, long> active_note_final_off_ticks;

    // 定义一个结构体来存储通道的最终状态
    struct ChannelState {
        int note;
        int vol;
        int wave;
        int pitch_bend;
        bool is_note_off;
    };

    // 用于跟踪每个Gigatron通道上次输出的状态，避免重复输出
    std::map<int, int> last_output_note;
    std::map<int, int> last_output_vol;
    std::map<int, int> last_output_wave;
    std::map<int, int> last_output_pitch_bend;
    std::map<int, bool> channel_is_on; // 跟踪通道是否正在播放音符

    // 初始化last_output_state
    for (int i = 1; i <= 4; ++i) { // Gigatron通道为1-4
        last_output_note[i] = -1;
        last_output_vol[i] = -1;
        last_output_wave[i] = -1;
        last_output_pitch_bend[i] = -9999; // 使用一个不太可能是真实弯音的值
        channel_is_on[i] = false;
    }

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <midi_file_path> <output_gbas_file_path> [options]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -d                          Enable dynamic channel allocation (default: static allocation)" << std::endl;
        std::cerr << "  -nv                         Disable velocity changes during note on (volume fixed at note on)" << std::endl;
        std::cerr << "  -np                         Disable pitch bend and modulation (quantize to semitones only, pitch=0)" << std::endl;
        std::cerr << "  -time <seconds>             Maximum duration in seconds (default: unlimited)" << std::endl;
        std::cerr << "  -pitch_multiple <value>     Pitch bend multiplier (default: 1.0)" << std::endl;
        std::cerr << "  -accuracy <levels>          Volume accuracy levels 1-64 (default: 64)" << std::endl;
        std::cerr << "  -vl <levels>               Volume accuracy levels 1-64 (alias for -accuracy)" << std::endl;
        std::cerr << "  -min_volume <value>         Minimum volume level 0-63 (default: 0)" << std::endl;
        std::cerr << "  -compensate <value>         Timer compensation target (default: 60)" << std::endl;
        std::cerr << "  -speed <value>              Playback speed multiplier (default: 1.0)" << std::endl;
        std::cerr << "  -ch1wave <wave>             Channel 1 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)" << std::endl;
        std::cerr << "  -ch2wave <wave>             Channel 2 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)" << std::endl;
        std::cerr << "  -ch3wave <wave>             Channel 3 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)" << std::endl;
        std::cerr << "  -ch4wave <wave>             Channel 4 waveform (0=noise, 1=triangle, 2=square, 3=sawtooth, -1=auto)" << std::endl;
        std::cerr << "  -config <file>              Use INI configuration file for instrument settings" << std::endl;
        std::cerr << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  " << argv[0] << " input.mid output.gbas" << std::endl;
        std::cerr << "  " << argv[0] << " ff1_open.mid ff1.gbas -d -nv -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 20 -compensate 60 -ch1wave 1 -ch2wave 0 -ch3wave 3 -ch4wave 1" << std::endl;
        std::cerr << "  " << argv[0] << " bwv813v.mid bwv813v.gbas -d -config midi_config.ini" << std::endl;
        return 1;
    }

    std::string midi_filepath = argv[1];
    std::string output_filepath = argv[2];
    
    // 从 midi_filepath 中提取文件名
    std::string midifile_name = midi_filepath;
    size_t last_slash = midifile_name.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        midifile_name = midifile_name.substr(last_slash + 1);
    }
    
    // 解析命令行参数
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-d") {
            dynamic_allocation = true;
        } else if (arg == "-nv") {
            no_velocity_change = true;
        } else if (arg == "-np") {
            no_pitch_bend = true;
        } else if (arg == "-time" && i + 1 < argc) {
            try {
                max_duration_seconds = std::stod(argv[++i]);
                if (max_duration_seconds <= 0) {
                    std::cerr << "Error: time must be a positive number." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid time argument. Must be a number." << std::endl;
                return 1;
            }
        } else if (arg == "-pitch_multiple" && i + 1 < argc) {
            try {
                pitch_bend_multiplier = std::stod(argv[++i]);
                if (pitch_bend_multiplier <= 0) {
                    std::cerr << "Error: pitch_multiple must be a positive number." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid pitch_multiple argument. Must be a number." << std::endl;
                return 1;
            }
        } else if (arg == "-accuracy" && i + 1 < argc) {
            try {
                cmd_volume_levels = std::stoi(argv[++i]);
                if (cmd_volume_levels < 1 || cmd_volume_levels > 64) {
                    std::cerr << "Error: accuracy must be between 1 and 64." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid accuracy argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-vl" && i + 1 < argc) {
            try {
                cmd_volume_levels = std::stoi(argv[++i]);
                if (cmd_volume_levels < 1 || cmd_volume_levels > 64) {
                    std::cerr << "Error: volume levels must be between 1 and 64." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid volume levels argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-min_volume" && i + 1 < argc) {
            try {
                min_volume_boost = std::stoi(argv[++i]);
                if (min_volume_boost < 0 || min_volume_boost > 63) {
                    std::cerr << "Error: min_volume must be between 0 and 63." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid min_volume argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-compensate" && i + 1 < argc) {
            try {
                timer_compensation_target = std::stoi(argv[++i]);
                if (timer_compensation_target <= 0) {
                    std::cerr << "Error: compensate must be a positive integer." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid compensate argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-speed" && i + 1 < argc) {
            try {
                speed_multiplier = std::stod(argv[++i]);
                if (speed_multiplier <= 0) {
                    std::cerr << "Error: speed must be a positive number." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid speed argument. Must be a number." << std::endl;
                return 1;
            }
        } else if (arg == "-ch1wave" && i + 1 < argc) {
            try {
                channel_waveforms[1] = std::stoi(argv[++i]);
                if (channel_waveforms[1] < -1 || channel_waveforms[1] > 3) {
                    std::cerr << "Error: ch1wave must be between -1 and 3 (-1=auto, 0=noise, 1=triangle, 2=square, 3=sawtooth)." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid ch1wave argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-ch2wave" && i + 1 < argc) {
            try {
                channel_waveforms[2] = std::stoi(argv[++i]);
                if (channel_waveforms[2] < -1 || channel_waveforms[2] > 3) {
                    std::cerr << "Error: ch2wave must be between -1 and 3 (-1=auto, 0=noise, 1=triangle, 2=square, 3=sawtooth)." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid ch2wave argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-ch3wave" && i + 1 < argc) {
            try {
                channel_waveforms[3] = std::stoi(argv[++i]);
                if (channel_waveforms[3] < -1 || channel_waveforms[3] > 3) {
                    std::cerr << "Error: ch3wave must be between -1 and 3 (-1=auto, 0=noise, 1=triangle, 2=square, 3=sawtooth)." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid ch3wave argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-ch4wave" && i + 1 < argc) {
            try {
                channel_waveforms[4] = std::stoi(argv[++i]);
                if (channel_waveforms[4] < -1 || channel_waveforms[4] > 3) {
                    std::cerr << "Error: ch4wave must be between -1 and 3 (-1=auto, 0=noise, 1=triangle, 2=square, 3=sawtooth)." << std::endl;
                    return 1;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid ch4wave argument. Must be an integer." << std::endl;
                return 1;
            }
        } else if (arg == "-config" && i + 1 < argc) {
            config_file = argv[++i];
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            return 1;
        }
    }

    // 如果指定了配置文件，则解析配置文件
    if (!config_file.empty()) {
        config_parser = new IniParser();
        if (!config_parser->parse(config_file)) {
            std::cerr << "Error: Failed to parse configuration file " << config_file << std::endl;
            delete config_parser;
            return 1;
        }
        std::cerr << "Successfully loaded configuration from " << config_file << std::endl;
    }

    std::ofstream output_file(output_filepath);
    if (!output_file.is_open()) {
        std::cerr << "Error: Could not open output file " << output_filepath << std::endl;
        if (config_parser) {
            delete config_parser;
        }
        return 1;
    }

    MidiFileParserImpl parser(config_parser);
    std::vector<CustomMidiEvent> midi_events = parser.parse(midi_filepath, max_duration_seconds, dynamic_allocation, no_velocity_change, config_parser);

    long ppqn = parser.get_ppqn();
    long tempo_us = parser.get_tempo(); // 微秒/四分音符

    // 计算每个 MIDI tick 对应的 Gigatron tick，并应用速度倍数
    // 速度倍数 < 1.0 表示减慢播放速度（时间间隔增大）
    // 速度倍数 > 1.0 表示加快播放速度（时间间隔减小）
    double gigatron_ticks_per_midi_tick = (static_cast<double>(tempo_us) * gigatron_ticks_per_second) / (static_cast<double>(ppqn) * 1000000.0 * speed_multiplier);


    // 按照时间戳排序 MIDI 事件
    std::sort(midi_events.begin(), midi_events.end(), [](const CustomMidiEvent& a, const CustomMidiEvent& b) {
        return a.timestamp < b.timestamp;
    });

    // 在应用 min_volume_boost 之前，找到所有 Note On 事件的实际 Gigatron 音量范围
    long sum_gigatron_volume = 0;
    int count_gigatron_volume = 0;
    bool found_any_note_on = false;

    for (const auto& event : midi_events) {
        if (!event.is_note_off && event.velocity > 0) { // 只考虑非零音量的 Note On 事件
            double base_velocity = event.velocity; // 0-127
            double volume_controller = event.volume; // 0-127
            double expression_controller = event.expression; // 0-127
            
            double normalized_volume = (base_velocity / 127.0) * (volume_controller / 127.0) * (expression_controller / 127.0);
            int current_gigatron_volume = static_cast<int>(std::round(normalized_volume * 63.0));

            if (current_gigatron_volume > 0) { // 忽略音量为0的事件
                sum_gigatron_volume += current_gigatron_volume;
                count_gigatron_volume++;
                found_any_note_on = true;
            }
        }
    }

    double volume_offset = 0.0;
    if (found_any_note_on && min_volume_boost > 0) {
        double original_avg_gigatron_volume = static_cast<double>(sum_gigatron_volume) / count_gigatron_volume;
        // 目标平均值设定为 min_volume_boost 和 63 的中间值，或者 min_volume_boost 自身，取决于实际情况
        // 这里我们简单地将目标平均值设定为 min_volume_boost 和 63 的平均值
        double target_avg_gigatron_volume = (static_cast<double>(min_volume_boost) + 63.0) / 2.0;
        
        volume_offset = target_avg_gigatron_volume - original_avg_gigatron_volume;
        std::cerr << "DEBUG: Original average Gigatron volume: " << original_avg_gigatron_volume << std::endl;
        std::cerr << "DEBUG: Target average Gigatron volume: " << target_avg_gigatron_volume << std::endl;
        std::cerr << "DEBUG: Calculated volume offset: " << volume_offset << std::endl;
    } else {
        std::cerr << "Warning: No Note On events found or min_volume_boost is 0. Volume boosting will not be applied based on average." << std::endl;
    }

    output_file << R"(_runtimePath_ "../runtime"
_runtimeStart_ &hFFFF
_codeRomType_ ROMv5a
'_enable8BitAudioEmu_ ON 'experimental

'本程序实现了基于查找表和sound on的声音合成引擎，尤其对查找表音高进行了精确调音。
'因此Gigatron目前是：
'1.4通道的复古波合成器，带有6/8 bit DAC音频输出
'2.支持4种内置波形，0噪音，1三角波，2方波，3锯齿波，每个通道可以自由使用这4个波形。
'3.支持自定义波形，通过改变oscH和oscL寄存器实现自定义
'4.低音质短PCM回放，实现方法参考at67的musicdemo
'5.音高范围C0到A7，跨越近8个八度
'6.软件定义音量，范围是0到63


'audio fix for ROMv5a
poke &h21, peek(&h21) OR 3
sound off
cls : mode 2
print "Gigatron 64K"
print "MIDI Engine by denjhang"
print ")" << midifile_name << R"("
'初始化



def index, fix_num ,oct, button, keystat, demo, old_timer, tick_note_ext
wave=1
vol=63
index=0
oct=5
tick_sum=0


call music_data


)" << std::endl;

// 添加 sound_engine9.gbas 中缺失的过程定义
output_file << R"(
proc fix

if oct==0
call fix_oct0
endif

if oct==1
call fix_oct1
endif

if oct==2
call fix_oct2
endif

if oct==3
call fix_oct3
endif

if oct==4
call fix_oct4
endif

if oct==5
call fix_oct5
endif

if oct==6
call fix_oct6
endif

if oct==7
call fix_oct6
endif

endproc

proc fix_oct0
if index == 0	'C
fix_num=67
endif
if index == 1	'C# (C和D的平均值)
fix_num=(67+74)/2
endif
if index == 2	'D
fix_num=74
endif
if index == 3	'D# (D和E的平均值)
fix_num=(74+86)/2
endif
if index == 4	'E
fix_num=86
endif
if index == 5	'F
fix_num=86
endif
if index == 6	'F# (F和G的平均值)
fix_num=(86+104)/2
endif
if index == 7	'G
fix_num=104
endif
if index == 8	'G# (G和A的平均值)
fix_num=(104+0)/2
endif
if index == 9	'A
fix_num=0
endif
if index == 10	'A# (A和B的平均值)
fix_num=(0+3)/2
endif
if index == 11	'B
fix_num=3
endif
endproc

proc fix_oct1
if index == 0	'C
fix_num=13
endif
if index == 1	'C#
fix_num=(13+34)/2
endif
if index == 2	'D
fix_num=34
endif
if index == 3	'D#
fix_num=(34+44)/2
endif
if index == 4	'E
fix_num=44
endif
if index == 5	'F
fix_num=63
endif
if index == 6	'F#
fix_num=(63+79)/2
endif
if index == 7	'G
fix_num=79
endif
if index == 8	'G#
fix_num=(79+103)/2
endif
if index == 9	'A
fix_num=103
endif
if index == 10	'A#
fix_num=(103+5)/2
endif
if index == 11	'B
fix_num=5
endif
endproc

proc fix_oct2
if index == 0	'C
fix_num=16
endif
if index == 1	'C#
fix_num=(16+58)/2
endif
if index == 2	'D
fix_num=58
endif
if index == 3	'D#
fix_num=(58+91)/2
endif
if index == 4	'E
fix_num=91
endif
if index == 5	'F
fix_num=115
endif
if index == 6	'F#
fix_num=(115+38)/2
endif
if index == 7	'G
fix_num=38
endif
if index == 8	'G#
fix_num=(38+87)/2
endif
if index == 9	'A
fix_num=87
endif
if index == 10	'A#
fix_num=(87+2)/2
endif
if index == 11	'B
fix_num=2
endif
endproc

proc fix_oct3
if index == 0	'C
fix_num=43
endif
if index == 1	'C#
fix_num=(43+102)/2
endif
if index == 2	'D
fix_num=102
endif
if index == 3	'D#
fix_num=(102+56)/2
endif
if index == 4	'E
fix_num=56
endif
if index == 5	'F
fix_num=95
endif
if index == 6	'F#
fix_num=(95+54)/2
endif
if index == 7	'G
fix_num=54
endif
if index == 8	'G#
fix_num=(54+27)/2
endif
if index == 9	'A
fix_num=27
endif
if index == 10	'A#
fix_num=(27+20)/2
endif
if index == 11	'B
fix_num=20
endif

endproc

proc fix_oct4
if index == 0	'C
fix_num=88
endif
if index == 1	'C#
fix_num=(88+88)/2
endif
if index == 2	'D
fix_num=88
endif
if index == 3	'D#
fix_num=(88+91)/2
endif
if index == 4	'E
fix_num=91
endif
if index == 5	'F
fix_num=62
endif
if index == 6	'F#
fix_num=(62+113)/2
endif
if index == 7	'G
fix_num=113
endif
if index == 8	'G#
fix_num=(113+56)/2
endif
if index == 9	'A
fix_num=56
endif
if index == 10	'A#
fix_num=(56+42)/2
endif
if index == 11	'B
fix_num=42
endif
endproc

proc fix_oct5
if index == 0	'C
fix_num=23
endif
if index == 1	'C#
fix_num=(23+51)/2
endif
if index == 2	'D
fix_num=51
endif
if index == 3	'D#
fix_num=(51+91)/2
endif
if index == 4	'E
fix_num=91
endif
if index == 5	'F
fix_num=120
endif
if index == 6	'F#
fix_num=(120+103)/2
endif
if index == 7	'G
fix_num=103
endif
if index == 8	'G#
fix_num=(103+136)/2
endif
if index == 9	'A
fix_num=136
endif
if index == 10	'A#
fix_num=(136+65)/2
endif
if index == 11	'B
fix_num=65
endif
endproc

proc fix_oct6
if index == 0	'C
fix_num=88
endif
if index == 1	'C#
fix_num=(88+88)/2
endif
if index == 2	'D
fix_num=88
endif
if index == 3	'D#
fix_num=(88+91)/2
endif
if index == 4	'E
fix_num=91
endif
if index == 5	'F
fix_num=62
endif
if index == 6	'F#
fix_num=(62+113)/2
endif
if index == 7	'G
fix_num=113
endif
if index == 8	'G#
fix_num=(113+56)/2
endif
if index == 9	'A
fix_num=56
endif
if index == 10	'A#
fix_num=(56+42)/2
endif
if index == 11	'B
fix_num=42
endif
endproc


proc fix_oct7
if index == 0	'C
fix_num=88
endif
if index == 1	'C#
fix_num=(88+88)/2
endif
if index == 2	'D
fix_num=88
endif
if index == 3	'D#
fix_num=(88+91)/2
endif
if index == 4	'E
fix_num=91
endif
if index == 5	'F
fix_num=62
endif
if index == 6	'F#
fix_num=(62+113)/2
endif
if index == 7	'G
fix_num=113
endif
if index == 8	'G#
fix_num=(113+56)/2
endif
if index == 9	'A
fix_num=56
endif
if index == 10	'A#
fix_num=(56+42)/2
endif
if index == 11	'B
fix_num=42
endif
endproc

proc stop_music
repeat
sound off
wait

&forever
endproc

dim enotes%(12) = 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
'定义音符数组 	  C5  C5# D5  D5# E5   F  F#  G5  G5# A5  A5# B5
def vis_ch,vis_note,vis_vol,vis_wave
proc eatSound_Timer,tick_note
pset 10+vis_note,40,vis_vol AND &h3F  '超简单可视化
set SOUND_TIMER, 1
wait_0:

if 	get("SOUND_TIMER") > 0

goto wait_0 '定时器不变化时，重新判断，此时cpu空闲
endif
tick_sum=tick_sum+1 				'定时器变化时，总tick加1
'gprintf "TIMER=%d,tick=%d",get("SOUND_TIMER"),tick_sum
if tick_sum==tick_note  		'马上进行对比，如果tick等于总tick，则播放声音
tick_note_ext=tick_note		'只赋值，然后回到乐谱程序播放4个通道的音符
 
else							'如果tick不等于总tick，则重新判断，此时cpu空闲
set SOUND_TIMER, 1

goto wait_0
endif

endproc

proc beep,ch,note,vol,wave,Pitch_Bend
  
 'MIDI note到本引擎的oct和index换算，已经验算
 oct=note/12-1
 index=note-12*(oct+1)
 
    local n
 call fix
    n = get("MUSIC_NOTE", peek(@enotes + index)+12*(oct-5))+fix_num+Pitch_Bend
 '查表和修正音高
 
    sound on, ch, n, vol, wave  ' 通道 2，频率-查表得出，音量-最大，波形1-三角波
 set SOUND_TIMER, 1 			'播放完成之后定时器重置，
 
 'gprintf "TIMER=%d,tick=%d,beep,%d,%d,%d,%d,%d,sound_frq=%d,pitch=%d"	,get("SOUND_TIMER"),tick_sum,ch,note,vol,wave,tick_note_ext,n,Pitch_Bend
  vis_ch=ch : vis_note=note : vis_vol=vol : vis_wave=wave

endproc


)" << std::endl;

long max_gigatron_tick = -1;
if (max_duration_seconds > 0) {
    // 应用速度倍数到最大时长
    // 如果速度减慢（speed_multiplier < 1.0），则需要更多的 Gigatron ticks 来表示相同的实际时间
    max_gigatron_tick = static_cast<long>(max_duration_seconds * 60.0 / speed_multiplier); // 1秒 = 60 Gigatron ticks
}

    // 将 MIDI 事件按时间戳分组，以便同时播放多个通道
    std::map<long, std::vector<CustomMidiEvent>> events_by_tick;
    
    for (const auto& event : midi_events) {
        long gigatron_start_tick = static_cast<long>(event.timestamp * gigatron_ticks_per_midi_tick);
        gigatron_start_tick = std::max(1L, gigatron_start_tick);
        
        CustomMidiEvent modified_event = event;
        // The channel in modified_event is already the mapped Gigatron channel from MidiFileParserImpl::parse
        
        events_by_tick[gigatron_start_tick].push_back(modified_event);
    }
    
    output_file << "proc music_data '先定时，再演奏，一次性演奏4个通道" << std::endl;
    output_file << "start:" << std::endl;
    // 获取第一个eatSound_Timer的时间数值减去1
    // 首先获取第一个事件的时间戳
    if (!events_by_tick.empty()) {
        long first_tick = events_by_tick.begin()->first;
        output_file << "\ttick_sum=" << (first_tick - 1) << std::endl;
    } else {
        output_file << "\ttick_sum=0" << std::endl;
    }
    
    std::map<long, std::vector<CustomMidiEvent>> macro_events; // 存储宏事件
    
    // 如果使用了配置文件，为每个Note On事件生成宏序列事件
    if (config_parser) {
        for (const auto& pair : events_by_tick) {
            long tick = pair.first;
            for (const auto& event : pair.second) {
                // 只处理Note On事件（非Note Off且速度大于0）
                if (!event.is_note_off && event.velocity > 0) {
                    // 获取乐器配置
                    int program = event.program;
                    // 使用 original_midi_channel 来判断是否是鼓声通道 (MIDI Channel 9, 即索引 9)
                    bool is_drum = (event.original_midi_channel == 9 && event.note >= 27 && event.note <= 87);
                    
                    // 获取精度
                    int instrument_config_accuracy = default_volume_levels; // 存储配置文件中的精度
                    if (config_parser) {
                        instrument_config_accuracy = is_drum ? get_drum_accuracy(event.note, config_parser) : get_instrument_accuracy(program, config_parser);
                    }

                    int current_effective_accuracy = instrument_config_accuracy; // 默认使用配置文件中的精度
                    // 如果命令行指定了精度，则使用命令行指定的精度，优先级高于配置文件
                    if (cmd_volume_levels != -1) {
                        current_effective_accuracy = cmd_volume_levels;
                    }
                    
                    // 计算音符持续时间（以Gigatron tick为单位）
                    long duration_ticks = static_cast<long>(event.duration * gigatron_ticks_per_midi_tick);
                    
                    // 如果有持续时间，生成宏序列事件
                    if (duration_ticks > 0) {
                        // 获取宏序列
                        std::vector<int> vol_sequence = is_drum ?
                            std::vector<int>{63} : get_instrument_volume_sequence(program, config_parser);
                        std::vector<int> wave_sequence = is_drum ?
                            std::vector<int>{get_drum_waveform(event.note, config_parser)} : get_instrument_waveform_sequence(program, config_parser);
                        std::vector<int> pitch_bend_sequence = is_drum ?
                            std::vector<int>{0} : get_instrument_pitch_bend_sequence(program, config_parser);
                        int note_offset = is_drum ? 0 : get_instrument_note_offset(program, config_parser);
                        
                        // 获取释放序列
                        std::vector<int> release_vol_sequence = is_drum ?
                            std::vector<int>{0} : get_instrument_release_volume_sequence(program, config_parser);
                        std::vector<int> release_wave_sequence = is_drum ?
                            std::vector<int>{0} : get_instrument_release_waveform_sequence(program, config_parser);
                        std::vector<int> release_pitch_bend_sequence = is_drum ?
                            std::vector<int>{0} : get_instrument_release_pitch_bend_sequence(program, config_parser);
                        
                        // 计算每个宏事件的时间间隔，基于 60 Gigatron ticks/second 和有效精度
                        long tick_increment = static_cast<long>(std::round(60.0 / current_effective_accuracy));
                        if (tick_increment == 0) tick_increment = 1; // 避免除以零或间隔为零

                        long note_on_gigatron_tick = tick; // Note On 事件的 Gigatron tick
                        long midi_note_off_gigatron_tick = tick + duration_ticks; // 原始 MIDI Note Off 应该发生的 Gigatron tick

                        // 计算释放宏的持续时间
                        long release_duration_ticks = release_vol_sequence.size() * tick_increment;
                        // 最终的 Note Off 时间点，包括释放宏的持续时间
                        long final_note_off_gigatron_tick = midi_note_off_gigatron_tick + release_duration_ticks;

                        // 存储最终的 Note Off 时间点，用于后续判断是否忽略原始 Note Off 事件
                        // 使用 (channel, note) 作为键，因为一个通道可能同时播放多个音符（在动态分配模式下）
                        // 并且宏是针对单个音符的
                        active_note_final_off_ticks[{event.channel, event.note}] = final_note_off_gigatron_tick;

                        // 生成音符持续期间的宏序列事件
                        for (size_t i = 0; i < vol_sequence.size(); ++i) {
                            long macro_tick = note_on_gigatron_tick + i * tick_increment;
                            
                            // 如果宏事件超出了原始 MIDI Note Off 的时间，则停止生成
                            if (macro_tick >= midi_note_off_gigatron_tick) {
                                break;
                            }

                            CustomMidiEvent macro_event = event;
                            macro_event.timestamp = macro_tick;
                            macro_event.duration = 0; // 宏事件没有持续时间
                            macro_event.is_macro_event = true; // 标记为宏事件
                            
                            // 应用宏值
                            if (i < vol_sequence.size()) {
                                int macro_vol_base = std::max(0, std::min(63, vol_sequence[i]));
                                // 对于宏事件，直接使用宏定义的音量，不进行简化和音量抬升
                                macro_event.volume = macro_vol_base;
                                std::cerr << "DEBUG: Macro Event - Tick: " << macro_tick << ", Channel: " << macro_event.channel
                                          << ", Note: " << macro_event.note << ", Macro Vol Base: " << macro_vol_base
                                          << ", Final Vol (Macro): " << macro_event.volume << std::endl;
                            }
                            
                            if (!wave_sequence.empty()) {
                                if (i < wave_sequence.size()) {
                                    macro_event.wave_value = wave_sequence[i];
                                } else {
                                    // 如果索引超出范围，使用序列中的最后一个波形值
                                    macro_event.wave_value = wave_sequence.back();
                                }
                            } else {
                                // 如果波形序列为空，使用默认波形（例如三角波）
                                macro_event.wave_value = 1;
                            }
                            
                            if (i < pitch_bend_sequence.size()) {
                                macro_event.pitch_bend = pitch_bend_sequence[i] / 100.0; // 转换为半音单位
                            }
                            
                            // 应用音高偏移
                            macro_event.note = event.note + note_offset;
                            
                            macro_events[macro_tick].push_back(macro_event);
                        }
                        
                        // 生成音符释放后的宏序列事件
                        for (size_t i = 0; i < release_vol_sequence.size(); ++i) {
                            long release_macro_tick = midi_note_off_gigatron_tick + i * tick_increment;
                            
                            CustomMidiEvent release_macro_event = event;
                            release_macro_event.timestamp = release_macro_tick;
                            release_macro_event.duration = 0; // 宏事件没有持续时间
                            release_macro_event.is_macro_event = true; // 标记为宏事件
                            release_macro_event.is_release_event = true; // 标记为释放事件
                            
                            // 应用释放宏值
                            if (i < release_vol_sequence.size()) {
                                int release_macro_vol_base = std::max(0, std::min(63, release_vol_sequence[i]));
                                release_macro_event.volume = release_macro_vol_base;
                                std::cerr << "DEBUG: Release Macro Event - Tick: " << release_macro_tick << ", Channel: " << release_macro_event.channel
                                          << ", Note: " << release_macro_event.note << ", Release Macro Vol Base: " << release_macro_vol_base
                                          << ", Final Vol (Release Macro): " << release_macro_event.volume << std::endl;
                            }
                            
                            if (!release_wave_sequence.empty()) {
                                if (i < release_wave_sequence.size()) {
                                    release_macro_event.wave_value = release_wave_sequence[i];
                                } else {
                                    // 如果索引超出范围，使用序列中的最后一个波形值
                                    release_macro_event.wave_value = release_wave_sequence.back();
                                }
                            } else {
                                // 如果释放波形序列为空，使用默认波形（例如三角波）
                                release_macro_event.wave_value = 1;
                            }
                            
                            if (i < release_pitch_bend_sequence.size()) {
                                release_macro_event.pitch_bend = release_pitch_bend_sequence[i] / 100.0; // 转换为半音单位
                            }
                            
                            // 应用音高偏移
                            release_macro_event.note = event.note + note_offset;
                            
                            macro_events[release_macro_tick].push_back(release_macro_event);
                        }

                        // 在最终的 Note Off 时间点添加一个 sound off 事件
                        // 只有当有释放宏时才添加这个最终的 Note Off 事件，否则使用原始的 Note Off
                        if (!release_vol_sequence.empty()) {
                            CustomMidiEvent final_note_off_event = event; // 复制原始 Note On 事件
                            final_note_off_event.timestamp = final_note_off_gigatron_tick;
                            final_note_off_event.duration = 0;
                            final_note_off_event.is_note_off = true; // 标记为最终的 Note Off 事件
                            final_note_off_event.is_macro_event = false; // 这不是宏事件，而是最终的音符关闭
                            final_note_off_event.is_release_event = false; // 这不是释放宏事件
                            final_note_off_event.volume = 0; // 确保最终的 Note Off 事件音量为 0
                            macro_events[final_note_off_gigatron_tick].push_back(final_note_off_event);
                        }
                    }
                }
            }
        }
        
        // 将宏事件合并到events_by_tick中
        for (const auto& pair : macro_events) {
            // 确保宏事件不会覆盖原始的Note On事件，而是添加到现有事件之后
            events_by_tick[pair.first].insert(events_by_tick[pair.first].end(),
                pair.second.begin(), pair.second.end());
        }
    }
    
    // 按时间戳排序并生成输出
    for (const auto& pair : events_by_tick) {
        long tick = pair.first;
        
        // 如果设置了最大时长，并且当前事件的时间戳超过了最大 Gigatron tick，则停止处理
        if (max_gigatron_tick != -1 && tick > max_gigatron_tick) {
            break; // 跳出循环
        }

        // 先输出定时调用
        output_file << "\tcall eatSound_Timer," << tick << std::endl;
        output_file << std::endl;
        
        // 排序当前tick内的事件，确保处理顺序一致
        std::vector<CustomMidiEvent> current_tick_events = pair.second;
        std::sort(current_tick_events.begin(), current_tick_events.end(), [](const CustomMidiEvent& a, const CustomMidiEvent& b) {
            return a.channel < b.channel;
        });

        // 存储当前tick内每个Gigatron通道的最终状态
        std::map<int, ChannelState> final_channel_states_for_tick;

        for (const auto& event : current_tick_events) {
            int channel = event.channel;

            // 确定当前事件的有效音量等级
            int current_effective_volume_levels = default_volume_levels;
            if (cmd_volume_levels != -1) {
                current_effective_volume_levels = cmd_volume_levels;
            } else if (config_parser) {
                // 假设如果存在config_parser，则使用其默认精度
                current_effective_volume_levels = config_parser->getDefaultAccuracy();
            }

            if (event.is_note_off) {
                // 检查这个 Note Off 事件是否是原始的 MIDI Note Off，但其对应的 Note On 有释放宏
                // 如果是，并且这个 Note Off 的时间戳早于最终的 Note Off 时间戳，则忽略它
                auto it = active_note_final_off_ticks.find({event.channel, event.note});
                if (it != active_note_final_off_ticks.end() && event.timestamp < it->second) {
                    // 这是一个被释放宏延长的 Note Off，忽略它
                    continue;
                }
                // 否则，这是一个真正的 Note Off 事件（要么没有释放宏，要么是最终的 Note Off 事件）
                // 此时，我们不立即输出 sound off，而是将其状态记录下来，在后续统一处理
                final_channel_states_for_tick[channel].is_note_off = true;
                final_channel_states_for_tick[channel].note = -1;
                final_channel_states_for_tick[channel].vol = 0; // 强制设置为0，表示音符关闭
                final_channel_states_for_tick[channel].wave = -1;
                final_channel_states_for_tick[channel].pitch_bend = -9999;
                continue; // 处理下一个事件
            }

            // 计算当前事件的最终音符、音量、波形和弯音单位
            double total_bend_semitones = event.pitch_bend;
            int note_offset = static_cast<int>(std::round(total_bend_semitones));
            double fine_bend_semitones = total_bend_semitones - note_offset;
            int final_note = convert_midi_note(event.note + note_offset);

            double base_velocity = event.velocity;
            double volume_controller = event.volume;
            double expression_controller = event.expression;
            double normalized_volume = (base_velocity / 127.0) * (volume_controller / 127.0) * (expression_controller / 127.0);
            int gigatron_volume = static_cast<int>(std::round(normalized_volume * 63.0));
            int vol;

            if (event.is_macro_event) {
                // 如果是宏事件，直接使用宏定义的音量，不进行简化和音量抬升
                vol = event.volume;
            } else {
                // 否则，应用音量简化和抬升
                int simplified_vol = simplify_volume(static_cast<int>(std::round(gigatron_volume + volume_offset)), current_effective_volume_levels);
                vol = apply_volume_boost(simplified_vol, min_volume_boost);
            }

            int wave;
            if (channel_waveforms[channel] != -1) {
                wave = channel_waveforms[channel];
            } else if (event.is_macro_event) {
                // 对于宏事件，波形值已经存储在event.wave_value中
                wave = event.wave_value;
            } else {
                wave = convert_midi_waveform(event.program, config_parser);
            }

            int final_pitch_bend_gigatron_unit = 0;
            if (!no_pitch_bend) {
                double current_pitch_bend_cents = fine_bend_semitones * 100.0;
                const double max_vibrato_depth_cents = 50.0;
                double vibrato_depth_cents = (static_cast<double>(event.modulation) / 127.0) * max_vibrato_depth_cents;
                current_pitch_bend_cents += vibrato_depth_cents;
                final_pitch_bend_gigatron_unit = static_cast<int>(current_pitch_bend_cents * pitch_bend_multiplier + (current_pitch_bend_cents > 0 ? 0.5 : -0.5));
            }

            // 更新当前tick内该通道的最终状态
            final_channel_states_for_tick[channel].note = final_note;
            final_channel_states_for_tick[channel].vol = vol;
            final_channel_states_for_tick[channel].wave = wave;
            final_channel_states_for_tick[channel].pitch_bend = final_pitch_bend_gigatron_unit;
            final_channel_states_for_tick[channel].is_note_off = false; // 只要有Note On或宏事件，就不是Note Off
        }

        // 遍历当前tick内每个通道的最终状态，并输出
        for (const auto& final_state_pair : final_channel_states_for_tick) {
            int channel = final_state_pair.first;
            const auto& state = final_state_pair.second;

            if (state.is_note_off) {
            } else {
                // 检查是否有任何参数自上次输出以来发生变化
                bool changed = false;
                if (state.note != last_output_note[channel] ||
                    state.vol != last_output_vol[channel] ||
                    state.wave != last_output_wave[channel] ||
                    state.pitch_bend != last_output_pitch_bend[channel])
                {
                    changed = true;
                }

                // 如果音量为0，且通道当前是开启状态，则需要关闭声音
                // 此时，通过将音量设置为0来隐式关闭声音，而不是显式输出 sound off
                if (state.vol == 0 && channel_is_on[channel]) {
                    // 标记为关闭，并重置上次输出的值，以便下次音量大于0时能重新输出 call beep
                    channel_is_on[channel] = false;
                    last_output_note[channel] = -1;
                    last_output_vol[channel] = -1;
                    last_output_wave[channel] = -1;
                    last_output_pitch_bend[channel] = -9999;
                }
                
                // 只有当音量大于0时才输出 call beep，或者当音量为0但需要更新状态时
                // 如果音量为0，且通道之前是开启状态，则不需要输出 call beep，因为已经通过上述逻辑处理了关闭
                if (state.vol > 0 || (state.vol == 0 && changed && !channel_is_on[channel])) {
                    if (changed || !channel_is_on[channel]) { // 如果有变化或者通道之前是关闭的，则输出
                        output_file << "\tcall beep," << channel << "," << state.note << "," << state.vol << "," << state.wave << "," << state.pitch_bend << std::endl;
                        // 更新上次输出的值
                        last_output_note[channel] = state.note;
                        last_output_vol[channel] = state.vol;
                        last_output_wave[channel] = state.wave;
                        last_output_pitch_bend[channel] = state.pitch_bend;
                        channel_is_on[channel] = true; // 标记为开启
                    }
                }
            }
        }
    }
    
    output_file << "\tsound off" << std::endl;
    output_file << "\tgoto start" << std::endl;
    output_file << "endproc" << std::endl;
 
    output_file.close();
    return 0;
}