# MIDI 转换器功能说明文档

## 目录

1. [项目概述](#项目概述)
2. [功能特性](#功能特性)
3. [命令行参数](#命令行参数)
4. [核心算法](#核心算法)
   1. [MIDI 文件解析](#midi-文件解析)
   2. [通道分配策略](#通道分配策略)
   3. [弯音轮和颤音轮处理](#弯音轮和颤音轮处理)
   4. [定时器补偿机制](#定时器补偿机制)
   5. [音量处理](#音量处理)
   6. [配置文件系统](#配置文件系统)
   7. [宏事件系统](#宏事件系统)
5. [开发过程](#开发过程)
6. [使用示例](#使用示例)
7. [技术细节](#技术细节)

## 项目概述

MIDI 转换器是一个将标准 MIDI 文件转换为 Gigatron 音频引擎可执行代码的工具。Gigatron 是一个 8 位复古计算机，具有 4 通道的音频合成能力。本转换器支持复杂的 MIDI 特性，包括弯音轮、颤音轮、动态通道分配、定时器补偿、配置文件支持和宏事件系统，以在有限的硬件资源上实现最佳的音乐播放效果。

## 功能特性

### 1. MIDI 文件解析
- 支持标准 MIDI 文件格式（.mid）
- 解析多轨道 MIDI 文件
- 自动处理 Note On/Note Off 事件对
- 支持速度、音量控制器、表情控制器
- 支持程序变更（乐器切换）
- 支持弯音轮灵敏度设置（RPN）

### 2. 弯音轮和颤音轮处理
- **弯音轮量化**：将 MIDI 弯音轮范围（-8192 到 8191）量化为小范围（-50 到 50）
- **大范围弯音处理**：超过半音范围的弯音自动展开为音符变化 + 小范围弯曲
- **颤音轮量化**：将调制轮值（0-127）量化为小范围（0-20）
- **弯音禁用选项**：通过 `-np` 参数完全禁用弯音和颤音输出
- **弯音轮灵敏度支持**：支持通过 RPN 消息设置弯音轮灵敏度（0-72 半音）

### 3. 通道波形强制指定
- **波形强制指定**：通过 `-ch1wave` 到 `-ch4wave` 参数强制指定每个通道的波形
- **支持波形类型**：0=噪音，1=三角波，2=方波，3=锯齿波，-1=自动（使用MIDI程序转换）
- **灵活配置**：可以为每个通道独立指定不同的波形，实现更丰富的音色组合

### 4. 通道分配策略
- **静态分配**：按 MIDI 通道顺序分配给 Gigatron 通道（1-4）
- **动态分配**：使用 FIFO（先进先出）策略管理 4 个 Gigatron 通道
- **智能通道切换**：当所有通道都在使用时，自动替换最久未使用的通道
- **单音轨复音支持**：在动态分配模式下，支持单个 MIDI 通道的复音播放，使用 FIFO 队列管理音符

### 5. 定时器补偿机制
- **精度补偿**：支持将低精度定时器补偿到高精度（如 30 → 60）
- **通道分配**：补偿时将通道事件分配到不同的定时器 tick
- **智能触发**：只有当目标精度明显高于当前精度时才进行补偿

### 6. 音量处理
- **音量映射**：MIDI 速度（0-127）映射到 Gigatron 音量（0-63）
- **最低音量抬升**：支持将非零音量压缩到指定最小值以上
- **复合音量计算**：结合基础力度、音量控制器和表情控制器
- **音量精简**：支持将音量值精简为指定的等级数量（1-64级）
- **音量抬升补偿**：自动计算音量偏移，确保平均音量达到目标值

### 7. 配置文件系统
- **INI 配置文件支持**：通过 `-config` 参数指定配置文件
- **乐器配置**：为每个 MIDI 乐器程序号配置特定参数
- **鼓声配置**：为每个鼓声音符配置特定参数
- **宏定义**：支持定义音量、波形、弯音和释放序列宏
- **精度设置**：为每个乐器设置独立的精度等级

### 8. 宏事件系统
- **音量宏序列**：定义音符持续期间的音量变化序列
- **波形宏序列**：定义音符持续期间的波形变化序列
- **弯音宏序列**：定义音符持续期间的弯音变化序列
- **释放宏序列**：定义音符释放后的音量、波形和弯音变化序列
- **宏事件生成**：自动根据配置生成宏事件，实现复杂的音效

### 9. 可视化功能
- **简单像素可视化**：在 Gigatron 屏幕上提供类似钢琴键盘的可视化
- **音符位置映射**：将音符映射到屏幕上的特定位置
- **音量颜色编码**：使用不同颜色表示音量大小
- **实时显示**：与音乐播放同步显示音符活动状态

## 命令行参数

```
Usage: midi_converter.exe <midi_file_path> <output_gbas_file_path> [options]
```

### 参数说明

#### 必需参数
- `midi_file_path`：输入 MIDI 文件路径
- `output_gbas_file_path`：输出 Gigatron BASIC 文件路径

#### 可选参数
- `-d`：启用动态通道分配（默认：静态分配）
- `-nv`：禁用音符开启时的力度变化（音量固定在音符开启时）
- `-np`：禁用弯音和颤音（量化为半音，音高=0）
- `-time <seconds>`：最大转换时长，单位秒（默认：无限制）
- `-pitch_multiple <value>`：弯音轮放大倍数（默认：1.0）
- `-accuracy <levels>`：音量精度等级 1-64（默认：64）
- `-vl <levels>`：音量精度等级 1-64（与 -accuracy 相同）
- `-min_volume <value>`：非零音量的最小值，范围 0-63（默认：0）
- `-compensate <value>`：定时器补偿目标精度（默认：60）
- `-speed <value>`：播放速度倍数（默认：1.0）
- `-ch1wave <wave>`：通道 1 波形（0=噪音，1=三角波，2=方波，3=锯齿波，-1=自动）
- `-ch2wave <wave>`：通道 2 波形（0=噪音，1=三角波，2=方波，3=锯齿波，-1=自动）
- `-ch3wave <wave>`：通道 3 波形（0=噪音，1=三角波，2=方波，3=锯齿波，-1=自动）
- `-ch4wave <wave>`：通道 4 波形（0=噪音，1=三角波，2=方波，3=锯齿波，-1=自动）
- `-config <file>`：使用 INI 配置文件进行乐器设置（默认：不使用配置文件）

## 核心算法

### MIDI 文件解析

程序使用 `MidiFile` 库解析 MIDI 文件，执行以下步骤：

1. **文件读取**：读取整个 MIDI 文件到内存
2. **时间分析**：计算每个事件的绝对时间戳
3. **音符链接**：自动匹配 Note On 和 Note Off 事件对
4. **事件分类**：将事件分类为 Note On/Off、控制器、弯音轮等
5. **RPN 处理**：处理注册参数号消息，支持弯音轮灵敏度设置

### 通道分配策略

#### 静态分配模式
- 按 MIDI 通道顺序（0-15）分配给 Gigatron 通道（1-4）
- 前 4 个有音符活动的 MIDI 通道获得 Gigatron 通道
- 超过后续 MIDI 通道

#### 动态分配模式
- 维护一个可用通道列表 `{1, 2, 3, 4}`
- 为每个新的 MIDI 通道分配一个可用 Gigatron 通道
- 当所有通道都在使用时，采用 FIFO 策略：
  - 记录每个 Gigatron 通道上次 Note On 的时间
  - 当需要新通道时，替换最久未使用的通道

#### 单音轨复音处理
- 在动态分配模式下，支持单个 MIDI 通道的复音播放
- 使用 FIFO 队列管理同一 MIDI 通道的多个音符
- 当所有 Gigatron 通道都被占用时，替换最旧的音符

### 弯音轮和颤音轮处理

#### 弯音轮量化算法
```cpp
// 将 MIDI 弯音轮值（-8192 到 8191）转换为半音
int raw_bend_value = message.getP2() * 128 + message.getP1();
int bend_value_centered = raw_bend_value - 8192;
double actual_semitone_bend = (static_cast<double>(bend_value_centered) / 8192.0) * pitch_bend_range;

// 将大范围弯音拆分为音符偏移和微调弯音
int note_offset = static_cast<int>(std::round(total_bend_semitones));
double fine_bend_semitones = total_bend_semitones - note_offset;

// 转换为 Gigatron 引擎单位
double current_pitch_bend_cents = fine_bend_semitones * 100.0;
int final_pitch_bend_gigatron_unit = static_cast<int>(current_pitch_bend_cents * pitch_bend_multiplier);
```

#### 颤音轮量化算法
```cpp
// 将调制轮值（0-127）量化为小范围（0-20）
const double max_vibrato_depth_cents = 50.0;
double vibrato_depth_cents = (static_cast<double>(modulation) / 127.0) * max_vibrato_depth_cents;
current_pitch_bend_cents += vibrato_depth_cents;
```

#### 弯音轮灵敏度处理
```cpp
// 处理 RPN 消息设置弯音轮灵敏度
if (controller_number == 101) { // RPN MSB
    _rpn_msb[midi_channel] = controller_value;
} else if (controller_number == 100) { // RPN LSB
    _rpn_lsb[midi_channel] = controller_value;
} else if (controller_number == 6) { // Data Entry MSB
    _data_entry_msb[midi_channel] = controller_value;
    // 如果是 RPN 0 (Pitch Bend Range)，则更新弯音轮灵敏度
    if (_rpn_msb[midi_channel] == 0 && _rpn_lsb[midi_channel] == 0) {
        double new_range = _data_entry_msb[midi_channel] + (_data_entry_lsb[midi_channel] != -1 ? _data_entry_lsb[midi_channel] / 100.0 : 0.0);
        _channel_pitch_bend_range[midi_channel] = std::min(72.0, std::max(0.0, new_range));
    }
}
```

### 定时器补偿机制

#### 补偿算法
```cpp
// 计算补偿比例
double compensation_ratio = static_cast<double>(timer_compensation_target) / gigatron_ticks_per_second;
bool needs_compensation = (compensation_ratio > 1.01);

// 如果需要补偿且当前tick有多个通道事件，则进行通道分配
if (needs_compensation && channel_events_list.size() > 1) {
    // 计算需要插入的补偿定时器数量
    int compensation_ticks = static_cast<int>(std::floor(compensation_ratio)) - 1;
    
    // 将通道事件分成两部分
    size_t half_size = channel_events_list.size() / 2;
    std::vector<std::pair<int, CustomMidiEvent>> first_half(channel_events_list.begin(), channel_events_list.begin() + half_size);
    std::vector<std::pair<int, CustomMidiEvent>> second_half(channel_events_list.begin() + half_size, channel_events_list.end());
    
    // 输出第一半通道事件
    // 插入补偿定时器调用
    // 输出第二半通道事件
}
```

### 音量处理

#### 音量映射算法
```cpp
// MIDI velocity 0-127 缩放到 0-63
int scaled_velocity = std::min(63, velocity / 2);

// 最低音量抬升
if (min_volume_boost > 0 && min_volume_boost < 63 && scaled_velocity > 0) {
    scaled_velocity = min_volume_boost + static_cast<int>(std::round(
        (static_cast<double>(scaled_velocity - 1) / 62.0) * (63 - min_volume_boost)
    ));
    scaled_velocity = std::min(63, scaled_velocity);
}

// 复合音量计算
double combined_volume_double = (base_velocity * volume_controller * expression_controller) / (127.0 * 127.0);
int vol = convert_midi_volume(static_cast<int>(std::round(combined_volume_double)), min_volume_boost);
```

#### 音量精简算法
```cpp
// 将音量值精简为指定的等级数量
int simplify_volume(int volume, int volume_levels) {
    if (volume_levels <= 1) return 0; // 如果只有1个等级，所有音量都为0
    if (volume_levels >= 64) return volume; // 如果等级数大于等于64，不进行精简
    
    // 将0-63的音量范围划分为volume_levels个等级
    int level_size = 64 / volume_levels;
    int level = volume / level_size;
    
    // 确保不超出范围
    if (level >= volume_levels) level = volume_levels - 1;
    
    // 返回该等级的中间值，以保持音量的相对关系
    return level * level_size + level_size / 2;
}
```

### 配置文件系统

#### 配置文件解析
程序使用 `IniParser` 类解析 INI 配置文件，支持以下配置项：

1. **乐器配置**：为每个 MIDI 乐器程序号（0-127）配置特定参数
2. **鼓声配置**：为每个鼓声音符（27-87）配置特定参数
3. **宏定义**：定义音量、波形、弯音和释放序列宏
4. **精度设置**：为每个乐器设置独立的精度等级

#### 配置文件格式示例
```ini
[Instrument_80]
accuracy=30
macros=wave,vol,pitch_bend,release_vol,release_wave,release_pitch_bend
wave_on=2
vol_on=63,60,55,50,45,40,35,30
pitch_bend_on=0,0,0,0,0,0,0,0
release_vol_on=30,20,10,0
release_wave_on=1,1,1,1
release_pitch_bend_on=0,0,0,0

[Drum_35]
accuracy=20
macros=wave,vol
wave_on=0
vol_on=63,50,30,10,0
```

### 宏事件系统

#### 宏事件生成算法
```cpp
// 为每个 Note On 事件生成宏序列事件
if (duration_ticks > 0) {
    // 获取宏序列
    std::vector<int> vol_sequence = get_instrument_volume_sequence(program, config_parser);
    std::vector<int> wave_sequence = get_instrument_waveform_sequence(program, config_parser);
    std::vector<int> pitch_bend_sequence = get_instrument_pitch_bend_sequence(program, config_parser);
    
    // 计算每个宏事件的时间间隔
    long tick_increment = static_cast<long>(std::round(60.0 / current_effective_accuracy));
    
    // 生成音符持续期间的宏序列事件
    for (size_t i = 0; i < vol_sequence.size(); ++i) {
        long macro_tick = note_on_gigatron_tick + i * tick_increment;
        
        CustomMidiEvent macro_event = event;
        macro_event.timestamp = macro_tick;
        macro_event.is_macro_event = true;
        macro_event.volume = vol_sequence[i];
        macro_event.wave_value = wave_sequence[i];
        macro_event.pitch_bend = pitch_bend_sequence[i] / 100.0;
        
        macro_events[macro_tick].push_back(macro_event);
    }
}
```

#### 释放宏处理
```cpp
// 生成音符释放后的宏序列事件
for (size_t i = 0; i < release_vol_sequence.size(); ++i) {
    long release_macro_tick = midi_note_off_gigatron_tick + i * tick_increment;
    
    CustomMidiEvent release_macro_event = event;
    release_macro_event.timestamp = release_macro_tick;
    release_macro_event.is_macro_event = true;
    release_macro_event.is_release_event = true;
    release_macro_event.volume = release_vol_sequence[i];
    release_macro_event.wave_value = release_wave_sequence[i];
    release_macro_event.pitch_bend = release_pitch_bend_sequence[i] / 100.0;
    
    macro_events[release_macro_tick].push_back(release_macro_event);
}
```

## 开发过程

### 第一阶段：基础功能实现
1. **MIDI 解析**：实现基本的 MIDI 文件读取和事件解析
2. **简单转换**：将 Note On/Off 事件转换为 Gigatron BASIC 代码
3. **通道映射**：实现简单的 MIDI 通道到 Gigatron 通道的映射

### 第二阶段：高级功能开发
1. **弯音轮支持**：添加弯音轮解析和量化功能
2. **颤音轮支持**：添加调制轮解析和量化功能
3. **动态通道分配**：实现 FIFO 策略的动态通道分配
4. **音量优化**：添加音量控制器和表情控制器支持

### 第三阶段：优化和扩展
1. **定时器补偿**：实现低精度到高精度的定时器补偿
2. **参数化设计**：添加命令行参数支持各种配置
3. **性能优化**：优化事件处理和代码生成逻辑
4. **错误处理**：完善错误处理和用户反馈

### 第四阶段：配置系统和宏功能
1. **配置文件系统**：实现 INI 配置文件解析，支持乐器和鼓声配置
2. **宏事件系统**：实现音量、波形、弯音和释放序列宏
3. **音量精简**：实现音量值精简为指定等级数量
4. **单音轨复音**：改进动态分配模式，支持单音轨复音处理
5. **释放事件处理**：添加释放宏序列处理

### 第五阶段：完善和测试
1. **弯音禁用**：添加 `-np` 参数完全禁用弯音和颤音
2. **调试日志系统**：添加详细的调试日志功能
3. **文档完善**：编写详细的使用说明和技术文档
4. **全面测试**：使用各种 MIDI 文件进行全面测试

## 使用示例

### 基本转换
```bash
./midi_converter.exe input.mid output.gbas
```

### 高级转换（动态分配 + 弯音量化）
```bash
./midi_converter.exe input.mid output.gbas -d -time 30 -pitch_multiple 1.0 -min_volume 30 -compensate 60
```

### 禁用弯音和颤音
```bash
./midi_converter.exe input.mid output.gbas -np -time 30 -pitch_multiple 1.0 -min_volume 30 -compensate 60
```

### 禁用力度变化
```bash
./midi_converter.exe input.mid output.gbas -nv -time 30 -pitch_multiple 1.0 -min_volume 30 -compensate 60
```

### 定时器补偿（30 精度补偿到 60）
```bash
./midi_converter.exe input.mid output.gbas -d -np -time 30 -pitch_multiple 1.0 -compensate 30 -min_volume 30
```

### 通道波形强制指定
```bash
./midi_converter.exe input.mid output.gbas -ch1wave 1 -ch2wave 2 -ch3wave 3 -ch4wave 0
```

### 音量精度控制
```bash
./midi_converter.exe input.mid output.gbas -vl 8 -time 30 -min_volume 20
```

### 速度控制
```bash
./midi_converter.exe input.mid output.gbas -speed 0.5 -time 30
```

### 使用配置文件
```bash
./midi_converter.exe input.mid output.gbas -config midi_config.ini
```

### 综合示例（动态分配 + 弯音量化 + 通道波形指定）
```bash
./midi_converter.exe input.mid output.gbas -d -nv -time 40 -pitch_multiple 5 -accuracy 20 -min_volume 20 -compensate 60 -ch1wave 1 -ch2wave 0 -ch3wave 3 -ch4wave 1
```

### 配置文件 + 命令行参数组合
```bash
./midi_converter.exe input.mid output.gbas -config midi_config.ini -d -time 30 -vl 16 -min_volume 15
```

## 技术细节

### 数据结构

#### CustomMidiEvent 结构体
```cpp
struct CustomMidiEvent {
    int channel;        // Gigatron 通道 (1-4)
    int original_midi_channel; // 原始 MIDI 通道 (0-15)
    int note;           // MIDI 音符索引 (0-127)
    double velocity;     // MIDI 速度 (0-127)
    double volume;       // 当前音量（控制器7）
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
```

#### NoteState 结构体
```cpp
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
```

#### PolyphonyQueueItem 结构体
```cpp
struct PolyphonyQueueItem {
    int note;           // MIDI 音符索引
    int gigatron_channel; // 分配的Gigatron通道
    long start_tick;    // 音符开始的tick
};
```

### 关键算法

#### 事件优化算法
1. **优先级处理**：Note Off 事件 > Note On 事件 > 其他事件
2. **通道合并**：同一通道同一 tick 的多个事件合并为一个
3. **平均值计算**：对音量、表情、弯音等参数计算平均值
4. **宏事件处理**：根据配置生成宏事件序列
5. **释放事件处理**：处理音符释放后的宏序列

#### 时间戳转换算法
```cpp
// 将 MIDI tick 转换为 Gigatron tick
double gigatron_ticks_per_midi_tick = (static_cast<double>(tempo_us) * gigatron_ticks_per_second) / (static_cast<double>(ppqn) * 1000000.0 * speed_multiplier);
long gigatron_start_tick = static_cast<long>(event.timestamp * gigatron_ticks_per_midi_tick);
```

#### 音量抬升补偿算法
```cpp
// 计算音量偏移，确保平均音量达到目标值
if (found_any_note_on && min_volume_boost > 0) {
    double original_avg_gigatron_volume = static_cast<double>(sum_gigatron_volume) / count_gigatron_volume;
    double target_avg_gigatron_volume = (static_cast<double>(min_volume_boost) + 63.0) / 2.0;
    volume_offset = target_avg_gigatron_volume - original_avg_gigatron_volume;
}
```

### 输出格式

生成的 Gigatron BASIC 代码包含以下部分：
1. **初始化代码**：设置变量和音频引擎
2. **音高修正表**：每个八度的音高修正值
3. **定时器处理**：`eatSound_Timer` 过程
4. **音符播放**：`beep` 过程
5. **主程序**：`music_data` 过程，包含所有音符事件和宏事件

### 通道波形强制指定算法
```cpp
// 使用强制指定的波形，如果没有指定则使用默认的MIDI程序转换
int wave;
if (channel_waveforms[channel] != -1) {
    wave = channel_waveforms[channel];
} else if (event.is_macro_event) {
    // 对于宏事件，波形值已经存储在event.wave_value中
    wave = event.wave_value;
} else {
    wave = convert_midi_waveform(event.program, config_parser);
}
```

### 调试日志系统
程序支持详细的调试日志输出，记录到 `debug_midi_converter.log` 文件：
- MIDI 事件解析信息
- 通道分配过程
- 宏事件生成过程
- 音量处理过程

### 依赖库
- **MidiFile**：用于 MIDI 文件解析
- **IniParser**：用于 INI 配置文件解析
- **标准 C++ 库**：iostream, vector, string, algorithm, cmath, map, fstream

### 10. 初始定时器优化
- **动态初始值设置**：自动将 `tick_sum` 设置为第一个事件的时间戳减1
- **精确同步**：确保音乐播放从正确的时刻开始，避免初始延迟
- **代码实现**：
```cpp
// 获取第一个eatSound_Timer的时间数值减去1
if (!events_by_tick.empty()) {
    long first_tick = events_by_tick.begin()->first;
    output_file << "\ttick_sum=" << (first_tick - 1) << std::endl;
} else {
    output_file << "\ttick_sum=0" << std::endl;
}
```

### 编译要求
- 支持 C++11 或更高标准的编译器
- 需要链接 MidiFile 库
- 需要包含 IniParser 头文件和源文件

### 性能考虑
- 使用 `std::map` 进行快速查找
- 事件按时间戳排序，避免重复排序
- 批量处理同一 tick 的事件，减少输出文件大小
- 宏事件预先生成，减少运行时计算开销

## 总结

MIDI 转换器是一个功能强大的工具，能够将复杂的 MIDI 文件转换为 Gigatron 可执行代码。通过智能的通道分配、精确的弯音轮和颤音轮量化、灵活的定时器补偿机制，以及细致的音量处理，转换器能够在有限的硬件资源上实现高质量的音乐播放效果。

项目的设计充分考虑了 Gigatron 硬件的限制，并通过多种优化技术确保生成的代码既高效又易于理解。详细的命令行参数支持使得用户可以根据具体需求灵活配置转换过程。