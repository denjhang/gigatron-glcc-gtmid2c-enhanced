# Music 项目编译文档

## 1. 项目概述

`music` 项目是一个为 Gigatron TTL 计算机设计的音乐播放程序。它能够播放一系列 MIDI 歌曲，并在屏幕上实时显示歌曲名称、播放时间以及音频波形的可视化。用户可以通过键盘输入或方向键切换歌曲。

## 2. 编译过程

`music` 项目的编译通过 `Makefile` 进行管理。以下是编译该项目的步骤和相关配置：

### 2.1. 前提条件

-   `glcc.cmd`: Gigatron C 编译器。
-   `gtmid2c`: 用于将 `.gtmid` 格式的 MIDI 文件转换为 C 语言源文件的工具。
-   `make`: 构建自动化工具。

### 2.2. 编译步骤

1.  **进入项目目录**:
    首先，需要进入 `music` 项目所在的目录：
    ```bash
    cd music
    ```

2.  **执行 Make 命令**:
    在 `music` 目录下，运行 `make` 命令即可开始编译过程：
    ```bash
    make
    ```

### 2.3. Makefile 配置分析

以下是 `Makefile` 的关键配置和规则：

-   **`BINDIR=../`**: 定义了二进制工具（如 `glcc.cmd` 和 `gtmid2c`）所在的目录，即 `music` 目录的父目录。
-   **`ROM=dev7`**: 指定了目标 ROM 版本为 `dev7`。
-   **`CC=${BINDIR}glcc.cmd`**: 设置 C 编译器为 `../glcc.cmd`。
-   **`GTMID2C=${BINDIR}build/gtmid2c`**: 设置 MIDI 到 C 转换工具为 `../build/gtmid2c`。
-   **`CFLAGS=-map=64k,./music.ovl --no-runtime-bss`**: 编译器标志，指定了内存映射、覆盖文件和禁用运行时 BSS。
-   **`PGMS=music.gt1`**: 定义了最终生成的可执行文件名称。
-   **`MIDIS` 变量**: 列出了所有需要编译的 MIDI 源文件（例如 `midi/agony.c`, `midi/bath.c` 等）。这些 `.c` 文件是由对应的 `.gtmid` 文件转换而来。

-   **`all: ${PGMS}`**: 默认目标，依赖于 `music.gt1`。
-   **`music.gt1: music.c ${MIDIS}`**: 这是生成最终可执行文件的规则。它使用 `glcc.cmd` 编译器，将 `music.c` 和所有 MIDI C 源文件编译链接成 `music.gt1`。
    ```makefile
    ${CC} -o $@ -rom=${ROM} ${CFLAGS} $< ${MIDIS}
    ```
    其中：
    -   `$@` 代表目标文件 `music.gt1`。
    -   `$<` 代表第一个依赖文件 `music.c`。
    -   `${MIDIS}` 代表所有 MIDI C 源文件。
-   **`midi/%.c: midi/%.gtmid`**: 这是一个模式规则，用于将 `.gtmid` 文件转换为 `.c` 文件。它使用 `gtmid2c` 工具完成转换。
    ```makefile
    ${GTMID2C} -s 256 $< $@
    ```
    其中：
    -   `$<` 代表 `.gtmid` 源文件。
    -   `$@` 代表生成的 `.c` 文件。
-   **`clean` 目标**: 用于清理编译生成的文件。

### 2.4. 编译输出

成功执行 `make` 命令后，将会在 `music` 目录下生成 `music.gt1` 文件。此文件是 Gigatron TTL 计算机可执行的二进制程序。

## 3. 音乐程序分析

`music.c` 程序的核心功能是播放 MIDI 音乐并提供简单的用户交互和音频可视化。

### 3.1. 主要功能模块

-   **MIDI 歌曲管理**:
    -   `midis[]` 结构体数组存储了所有可播放的 MIDI 歌曲数据指针和名称。
    -   `NMIDIS` 宏定义了歌曲的数量。
    -   `mindex` 变量跟踪当前播放的歌曲索引。
-   **屏幕显示**:
    -   `header_name()`: 在屏幕上显示当前播放歌曲的名称。
    -   `header_time()`: 显示歌曲的播放时间。
    -   `gotoxy()` 和 `cprintf()` 用于控制文本输出位置和格式化。
-   **音频可视化 (`sample_display`)**:
    -   程序在内存地址 `0x8000` 和 `0x8080` 处使用了两个 128 字节的缓冲区 (`sample1`, `sample2`) 来捕获音频数据。
    -   `sample_display()` 函数通过读取 `0x13` 内存地址（通常是 Gigatron 的音频输入或状态寄存器）来获取音频采样数据。
    -   它在屏幕的特定区域 (`SAMPLEY`) 绘制这些采样数据，形成一个简单的音频波形可视化效果。
    -   `SYS_SetMemory()` 用于清空屏幕上的采样显示区域。
-   **用户交互 (`handle_keys`)**:
    -   通过 `serialRaw` (串口输入) 和 `buttonState` (按钮状态) 检测用户输入。
    -   用户可以通过数字键 ('1'-'9') 直接选择歌曲，或使用方向键（上/左用于上一首，下/右用于下一首）切换歌曲。
    -   当歌曲切换时，会调用 `midi_play(0)` 停止当前播放的歌曲。
-   **主循环 (`main`)**:
    -   初始化声音系统 (`sound_sine_waveform`, `sound_reset`)。
    -   清除采样显示区域。
    -   在屏幕上显示操作提示 "Use digits or arrows"。
    -   进入一个无限循环，持续执行以下操作：
        -   更新歌曲名称显示。
        -   调用 `midi_play()` 播放当前选定的 MIDI 歌曲。
        -   在一个内部循环中，当歌曲正在播放时，不断更新时间显示、采样可视化和处理用户按键。
        -   歌曲播放结束后，自动切换到下一首歌曲（或循环到第一首）。
        -   `_wait(30)` 引入短暂延迟，以控制循环速度。

### 3.2. 依赖库

-   `<stdio.h>`, `<conio.h>`, `<gigatron/libc.h>`: 标准输入输出、控制台 I/O 和 Gigatron C 标准库。
-   `<gigatron/sys.h>`: Gigatron 系统调用接口。
-   `<gigatron/sound.h>`: Gigatron 声音控制接口。

### 3.3. 内存使用注意事项

代码中将 `sample1` 和 `sample2` 缓冲区放置在 `0x8000` 和 `0x8080` 地址。注释中提到这通常是避免的区域，以防止在 32k 机器上崩溃。这表明该程序可能需要 64k 内存的 Gigatron 机器才能稳定运行，或者需要注意内存分配以避免冲突。

## 4. 音乐文件结构研究

通过分析 `D:\working\vscode-projects\Reference_Project\Gigatron\gigatron-lcc-master-2025.11.27\gigatron-lcc-master\gigatron\gtmid2c` Python 脚本，我们对 `.gtmid` 文件的结构以及它如何转换为 C 语言数组有了深入理解。

### 4.1. `.gtmid` 文件格式

`.gtmid` 文件是一种紧凑的二进制格式，用于存储 Gigatron 音乐事件。其结构如下：

1.  **文件头 (41 字节):**
    *   **魔术字符串 (6 字节):** 始终为 `gtMIDI`，用于标识文件类型。
    *   **歌曲名称 (32 字节):** 存储歌曲的名称，不足 32 字节的部分用空字符 (`\x00`) 填充。
    *   **`hasvolume` 标志 (1 字节):** 一个布尔值（0 或 1），指示后续的音符事件是否包含音量信息。
    *   **数据总大小 (2 字节，大端序短整型):** 表示文件头之后的数据部分的字节总数。

2.  **数据段 (可变大小):**
    此段包含一系列字节编码的音乐事件。`gtmid2c` 脚本逐字节解析这些事件。

### 4.2. 音乐事件编码

数据段中的每个字节或字节序列代表一个特定的音乐事件：

*   **延迟命令 (D(x))**:
    *   如果一个字节的值 `cmd` 在 `0` 到 `127` 之间，它表示一个延迟命令。
    *   `x` 的值即为 `cmd`，表示程序应等待 `cmd` 帧。
    *   在生成的 C 代码中，这被转换为 `D(cmd)` 宏调用。

*   **通道关闭命令 (X(c))**:
    *   如果一个字节的值 `cmd` 满足 `(cmd & 0xf0) == 128`，它表示一个通道关闭命令。
    *   通道 `c` 的计算方式为 `(cmd & 3) + 1`。
    *   在生成的 C 代码中，这被转换为 `X(channel)` 宏调用。

*   **音符开启命令 (N(c,n) 或 M(c,n,v))**:
    *   如果一个字节的值 `cmd` 满足 `(cmd & 0xf0) == 144`，它表示一个音符开启命令。
    *   通道 `c` 的计算方式为 `(cmd & 3) + 1`。
    *   紧随 `cmd` 字节之后的是一个字节 `note`，表示音符的音高（MIDI 音符编号）。
    *   如果文件头中的 `hasvolume` 标志为真，则在 `note` 字节之后还会有一个字节 `vol`，表示音量。
    *   `gtmid2c` 脚本会跟踪每个通道的当前音量。如果当前音符的音量 `vol` 与该通道上一个音符的音量不同，则生成 `M(channel, note, vol)` 宏调用；否则，生成 `N(channel, note)` 宏调用。

### 4.3. `gtmid2c` 工具的作用

`gtmid2c` 是一个 Python 脚本，负责将 `.gtmid` 格式的二进制音乐数据转换为 Gigatron C 编译器可识别的 C 语言源文件。其主要功能包括：

1.  **解析 `.gtmid` 文件**: 读取 `.gtmid` 文件的文件头和数据段，解析出音乐事件。
2.  **事件转换**: 将二进制事件转换为 C 语言宏调用 (`D`, `X`, `N`, `M`)。
3.  **分段处理**: 将转换后的事件序列分割成多个 `static const byte` 数组。每个数组的大小受 `--segment-size` 参数控制（默认 96 字节），以优化内存使用和编译器处理。每个分段数组以 `0` 字节作为结束标记。
4.  **生成指针数组**: 创建一个 `const byte *identifier[]` 数组，其中包含指向所有分段数据数组的指针，并以空指针 (`0`) 终止。这个指针数组是 `music.c` 程序中 `midis[]` 结构体所引用的数据。
5.  **宏定义**: 在生成的 C 文件中包含必要的宏定义，以便编译器正确解释数据数组中的事件。

通过 `gtmid2c` 工具，复杂的二进制音乐数据被有效地转换为易于 Gigatron C 程序编译和使用的结构化 C 语言数据。

### 4.4. C 音乐标准格式详解：以 `bath.c` 为例

为了更深入地理解 `gtmid2c` 生成的 C 音乐标准格式，我们以 [`music/midi/bath.c`](music/midi/bath.c) 文件为例，详细解析其各个组成部分。

#### 4.4.1. 文件头部注释

```c
/* extern const byte* bath[];
 * -- generated by gtmid2c from file midi/bath.gtmid
 *    memsize 4173 in 17 segments
 */
```

-   **`extern const byte* bath[];`**: 这行注释表明该文件定义了一个名为 `bath` 的外部字节数组指针，这个数组将在 `music.c` 中被引用。
-   **`-- generated by gtmid2c from file midi/bath.gtmid`**: 指明了该 C 文件是由 `gtmid2c` 工具从 `midi/bath.gtmid` 文件自动生成的。
-   **`memsize 4173 in 17 segments`**: 提供了原始音乐数据的总大小（4173 字节）以及被分割成的段数（17 段）。

#### 4.4.2. 宏定义

```c
#define D(x) x                     /* wait x frames */
#define X(c) 127+(c)               /* channel c off */
#define N(c,n) 143+(c),(n)         /* channel c on, note=n */
#define M(c,n,v) 159+(c),(n),(v)   /* channel c on, note=n, wavA=v */
#define byte unsigned char
#define nohop __attribute__((nohop))
```

这些宏是 C 音乐标准格式的核心，用于将音乐事件编码为字节序列：

-   **`D(x)`**: 延迟宏，`x` 表示等待的帧数。在生成的字节数组中，它直接展开为 `x` 的值。
-   **`X(c)`**: 通道关闭宏，`c` 是通道号（1-4）。它展开为 `127 + c`，即一个大于 127 的值，用于在字节流中标识通道关闭事件。
-   **`N(c,n)`**: 音符开启宏（无音量/波形参数），`c` 是通道号，`n` 是音符编号。它展开为 `143 + c, n`，即一个大于 143 的值后跟一个音符编号。
-   **`M(c,n,v)`**: 音符开启宏（带音量/波形参数），`c` 是通道号，`n` 是音符编号，`v` 是音量/波形参数。它展开为 `159 + c, n, v`，即一个大于 159 的值后跟音符编号和音量/波形参数。
-   **`byte`**: 将 `byte` 定义为 `unsigned char`，用于声明字节数组。
-   **`nohop`**: 这是一个 GCC 属性，用于告诉编译器不要对标记的函数或变量进行"跳转优化"。在 Gigatron 的上下文中，这可能与内存布局和执行效率有关。

#### 4.4.3. 音乐数据分段

```c
nohop static const byte bath000[] = {
  M(1,33,64),M(2,69,77),D(13),N(2,72),D(14),N(1,45),N(2,76),D(14),N(2,69),D(13),
  // ... 更多事件
  N(1,31),N(2,74),0
};
```

-   **`nohop static const byte bath000[]`**: 定义了一个名为 `bath000` 的静态常量字节数组。`nohop` 属性确保了特定的内存布局。
-   **数组内容**: 数组中的每个元素都是一个字节，代表一个音乐事件或事件的一部分。这些元素是通过展开 `D`、`X`、`N`、`M` 宏生成的。
    -   例如，`M(1,33,64)` 展开为 `160, 33, 64`。
    -   `D(13)` 展开为 `13`。
    -   `N(2,72)` 展开为 `145, 72`。
-   **结束标记**: 每个分段数组都以 `0` 字节作为结束标记，用于指示音乐数据段的结束。

`bath.c` 文件包含了从 `bath000` 到 `bath016` 共 17 个这样的分段数组，每个数组都包含一部分音乐数据。

#### 4.4.4. 指针数组

```c
nohop const byte *bath[] = {
  bath000,
  bath001,
  bath002,
  // ... 更多分段指针
  bath016,
  0
};
```

-   **`nohop const byte *bath[]`**: 定义了一个名为 `bath` 的常量字节数组指针数组。这个数组是 `music.c` 程序中 `midis[]` 结构体所引用的数据。
-   **数组内容**: 数组的每个元素都是一个指向音乐数据分段数组的指针。
-   **结束标记**: 数组以空指针 `0` 作为结束标记，用于指示歌曲数据段的结束。

通过这种分段和指针数组的方式，`gtmid2c` 工具能够有效地管理大型音乐数据，使其适应 Gigatron 的内存限制，并便于 `music.c` 程序按顺序加载和播放音乐。

#### 4.4.5. 音乐宏的解析机制

一个关键的问题是：这些音乐宏（`D`、`X`、`N`、`M`）是在哪里被解析的？答案是在 Gigatron 的系统级别（ROM）中，而不是在用户程序中。

根据 [`D:\working\vscode-projects\Reference_Project\Gigatron\glcc-20251127-2.6.37\lib\gigatron-lcc\include\gigatron\sound.h`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h) 文件中的声明：

```c
/* Play midi music encoded in modified gtmidi format.
   Music plays autonomously using a vIrq handler (needs ROM>=v5a),
   Passing a null pointer stops the currently playing music.
   You can generate modified midi data from at67's .gtmid
   file using program gtmid2c provided with glcc.
   The .gtmid files themselves are derived from MidiTone files
   using at67's program `gtmidi` available in his Contrib dir. */
extern void midi_play(const byte *midi[]);
```

这表明：

1.  **系统调用**: [`midi_play`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:85) 是一个外部系统调用，它接受一个指向字节数组指针的指针（即 `bath` 数组）。
2.  **自主播放**: 音乐播放是通过一个 vIRQ（垂直中断）处理程序自主进行的。这意味着音乐数据的解析和播放是在系统级别（ROM 中）处理的，而不是在用户程序中。
3.  **专用格式**: `gtmid2c` 工具生成的 C 音乐数据格式是专门为这个系统调用设计的。ROM 中的 vIRQ 处理程序知道如何解析这种格式，并根据字节流中的命令（延迟、通道关闭、音符开启等）来控制声音硬件。

因此，`music.c` 程序的角色是：
-   提供用户界面（歌曲选择、显示等）。
-   调用 [`midi_play`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:85) 系统调用来启动或停止音乐播放。
-   而实际的音乐数据解析和声音生成是由 Gigatron ROM 中的底层系统处理的。

这种设计将复杂的音乐播放逻辑从用户程序中分离出来，使得用户程序可以非常简洁，同时利用了 ROM 中优化的音乐播放引擎。

### 4.5. C 音乐标准格式与 MIDI 到 C 转换器实现思路

通过对 `gtmid2c` 生成的 C 音乐标准格式（如 [`music/midi/bath.c`](music/midi/bath.c) 所示）的分析，我们可以为开发一个独立的 MIDI 到 C 转换器提供以下实现思路：

1.  **输入解析**:
    *   转换器应首先解析标准的 MIDI 文件 (SMF)，提取其中的音符开/关事件、速度事件、程序变化事件等。
    *   需要处理 MIDI 文件的不同轨道和通道信息。

2.  **时间同步与帧转换**:
    *   MIDI 文件的时间信息通常以节拍和时间单位 (ticks per beat) 表示。需要将这些时间信息转换为 Gigatron 的帧延迟 ([`D(x)`](music/midi/bath.c:7) 宏)。
    *   这需要考虑 MIDI 文件的速度 (tempo) 设置以及 Gigatron 的固定帧率（通常为 60 FPS）。
    *   计算公式可能涉及：`帧延迟 = (MIDI 节拍时间 / MIDI 每拍时间单位) * (60 / 每分钟节拍数) * Gigatron 帧率`。

3.  **通道映射**:
    *   将 MIDI 通道 (0-15) 映射到 Gigatron 的四个可用声音通道 (1-4)。可能需要实现一个通道分配策略，例如将多个 MIDI 通道合并到一个 Gigatron 通道，或者只选择最重要的几个 MIDI 通道。

4.  **音符与速度转换**:
    *   MIDI 音符编号 (0-127) 可以直接映射到 Gigatron 的音符编号。
    *   MIDI 速度 (velocity) 值 (0-127) 需要映射到 [`M(c,n,v)`](music/midi/bath.c:10) 宏中的 `v` 参数。`gtmid2c` 脚本将此 `v` 参数解释为波形或音量。转换器需要决定如何将 MIDI 速度映射到 Gigatron 的声音参数。一种简单的映射是线性缩放。

5.  **事件序列生成**:
    *   遍历解析后的 MIDI 事件，并根据事件类型生成相应的 C 宏调用：
        *   **音符开启**: 如果有速度信息，生成 `M(channel, note, velocity_mapped)`；否则生成 `N(channel, note)`。
        *   **音符关闭**: `gtmid2c` 脚本通过在音符持续时间结束后发出 [`X(channel)`](music/midi/bath.c:8) 命令来关闭通道。转换器需要计算音符的持续时间，并在适当的延迟后插入 [`X(channel)`](music/midi/bath.c:8)。
        *   **延迟**: 在事件之间插入 [`D(frames)`](music/midi/bath.c:7) 宏，以确保正确的时序。

6.  **C 代码结构生成**:
    *   生成 C 文件的头部，包含 `D`、`X`、`N`、`M` 宏定义。
    *   将生成的事件序列组织成多个 `nohop static const byte bathXXX[]` 数组。为了避免单个数组过大，可以设定一个最大字节数（例如 `gtmid2c` 默认的 96 字节）进行分段。
    *   每个分段数组以 `0` 字节作为结束标记。
    *   最后，生成一个 `nohop const byte *bath[]` 数组，包含所有分段数组的指针，并以 `0` 终止。

7.  **高级功能考虑**:
    *   **波形变化**: 如果需要支持 MIDI 的程序变化 (Program Change) 事件来改变音色，转换器需要扩展 `gtmid2c` 的宏定义或引入新的宏来控制 [`sound_waveform`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:65)。
    *   **弯音轮/微分音**: 如前所述，这需要更复杂的处理，可能涉及在 C 数组中编码频率变化序列，并在 `midi_play` 函数中动态调整 [`sound_freq`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:43)。

通过遵循这些步骤，可以构建一个将标准 MIDI 文件转换为 Gigatron C 音乐标准格式的工具，从而极大地简化 Gigatron 上的音乐创作和移植过程。

## 5. 实时声音参数修改能力分析

基于对 `music.c` 源代码和 `gtmid2c` 脚本的分析，我们可以评估该音乐程序在实时修改声音参数方面的能力：

### 5.1. 音量 (Volume)

*   **可修改性**: **是**。`gtmid2c` 脚本在转换 `.gtmid` 文件时，如果原始 MIDI 数据包含音量信息 (`hasvolume` 标志为真)，则会生成带有音量参数 `v` 的 `M(c,n,v)` 宏。这意味着音量可以随每个音符事件进行指定和改变。
*   **实时性**: 音量是事件驱动的，即在每个音符开启事件中可以设置其音量。这允许在歌曲播放过程中动态调整音量，但不是连续的模拟式调制，而是离散的事件级调整。

### 5.2. 波形 (Waveform)

*   **可修改性**: **是**，但需要修改程序逻辑。Gigatron 声音 API 提供了 [`sound_waveform(channel, wave)`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:65) 宏，允许为每个通道独立设置波形。支持的波形类型包括 `0:noise`、`1:triangle`、`2:pulse` 和 `3:sawtooth`。
*   **实时性**: 理论上可以通过调用 [`sound_waveform(channel, wave)`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:65) 宏实时改变波形。然而，当前的 `music.c` 程序仅在启动时通过 [`sound_sine_waveform(2)`](music/music.c:128) 设置了一个全局波形，并未利用此功能。要实现实时波形改变，需要修改 `gtmid2c` 工具以在 `.gtmid` 数据中编码波形信息，并相应地修改 `midi_play` 函数来解释和应用这些波形变化。

### 5.2.1. 每个通道的独立波形

Gigatron 的声音硬件支持为每个通道独立设置波形。通过 [`sound_waveform(channel, wave)`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:65) 宏，可以为不同的音乐通道分配不同的音色。例如，一个通道可以是方波，另一个通道可以是三角波。

### 5.2.2. 实时改变波形的其他方法

除了修改 `gtmid2c` 和 `midi_play` 函数外，如果需要更灵活的实时波形控制，可以直接在 `music.c` 的主循环中，根据特定的逻辑或用户输入，调用 [`sound_waveform(channel, wave)`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:65) 宏来改变波形。但这需要程序逻辑来决定何时以及如何改变波形，而不是依赖于 MIDI 数据。

### 5.3. 音高 (Pitch)

*   **可修改性**: **是**。`N(c,n)` 和 `M(c,n,v)` 宏都包含 `n` 参数，它直接代表 MIDI 音符编号，从而控制音高。每个音符事件都会指定其音高。
*   **实时性**: 音高是事件驱动的，随每个音符事件实时改变。这是音乐播放的核心功能。

### 5.4. 弯音轮 (Pitch Bend)

*   **可修改性**: **否**，在当前 `.gtmid` 格式和 `gtmid2c` 工具中不直接支持。Gigatron 的声音 API 中没有直接提供弯音轮功能。
*   **实时性**: 由于格式和工具不支持，因此无法通过此系统实时改变弯音轮。

### 5.4.1. 弯音轮或微分音的实现可能性

尽管 `.gtmid` 格式不直接支持，但通过 Gigatron 的底层声音 API，理论上可以实现弯音轮或微分音效果：

*   **弯音轮**: 可以通过在短时间内连续调用 [`sound_freq(channel, frequency)`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:43) 函数，以非常小的步长逐渐改变音高，从而模拟弯音轮效果。这需要程序在播放过程中动态计算和更新频率，会增加处理复杂性。
*   **微分音**: Gigatron 声音 API 提供了 [`sound_freq(channel, frequency)`](D:/working/vscode-projects/Reference_Project/Gigatron/gigatron-lcc-master-2025.11.27/gigatron-lcc-master/include/gigatron/gigatron/sound.h:43) 函数，允许直接设置通道的精确频率。通过计算所需的精确频率值并调用此函数，可以实现微分音。这需要将微分音信息编码到音乐数据中（可能需要扩展 `.gtmid` 格式或使用其他数据源），并在 `midi_play` 函数中进行解析和应用。

这些高级功能需要对 `gtmid2c` 工具和 `music.c` 程序进行显著修改，以引入新的数据编码和运行时处理逻辑。

## 6. 游戏中音乐与音效的冲突管理

在游戏开发中，同时播放背景音乐和音效是一个常见需求。Gigatron 声音系统提供了 4 个独立的声音通道，这为同时播放音乐和音效提供了基础。以下是如何避免冲突的策略：

### 6.1. 通道分配策略

最直接的方法是为音乐和音效分配不同的通道：

-   **音乐通道**: 分配 2-3 个通道（如通道 1 和 2）给背景音乐。MIDI 播放器 [`midi_play`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:85) 会自动使用这些通道来播放音乐数据。
-   **音效通道**: 分配 1-2 个通道（如通道 3 和 4）专门用于游戏音效。

### 6.2. 音效实现方法

音效可以通过以下几种方式实现：

#### 6.2.1. 使用底层声音 API

使用 Gigatron 的底层声音 API 直接控制音效通道：

```c
// 播放一个简单的音效
void play_jump_sound() {
    // 在通道 3 上播放一个短促的音符
    sound_on(3, 60, 100, 2); // 通道 3, MIDI 音符 60 (C4), 音量 100, 脉冲波形
    sound_set_timer(10); // 持续 10 帧
}

// 播放爆炸音效
void play_explosion_sound() {
    sound_on(4, 40, 127, 0); // 通道 4, 低音, 最大音量, 噪声波形
    sound_set_timer(30); // 持续 30 帧
}
```

#### 6.2.2. 使用预定义的音效数据

对于复杂的音效，可以预先定义音效数据：

```c
// 定义音效数据（需要包含宏定义）
#define D(x) x                     /* wait x frames */
#define X(c) 127+(c)               /* channel c off */
#define M(c,n,v) 159+(c),(n),(v)   /* channel c on, note=n, wavA=v */
#define byte unsigned char

const byte jump_sound[] = {
    M(3,60,100), D(5), X(3), 0
};

const byte explosion_sound[] = {
    M(4,40,127), D(10), M(4,35,100), D(10), M(4,30,50), D(10), X(4), 0
};

// 播放音效的函数
void play_sound_effect(const byte *sound_data) {
    // 注意：此函数会中断音乐播放，实际应用中应使用底层声音API
    // 停止当前音乐
    midi_play(0);
    
    // 播放音效
    midi_play(sound_data);
    
    // 等待音效播放完成
    while (midi_playing()) {
        _wait(1);
    }
    
    // 注意：音乐无法恢复到中断位置，需要重新开始
}
```

### 6.3. 冲突避免策略

#### 6.3.1. 通道隔离

最简单的方法是确保音乐和音效使用不同的通道，这样它们就不会直接冲突。MIDI 播放器通常会使用前几个通道，所以音效应该使用后面的通道。

#### 6.3.2. 音量平衡

为了避免音效盖过音乐，需要合理设置音量：

```c
// 设置音乐通道的音量
sound_volume(1, 80); // 音乐通道 1
sound_volume(2, 80); // 音乐通道 2

// 设置音效通道的音量
sound_volume(3, 120); // 音效通道 3
sound_volume(4, 120); // 音效通道 4
```

#### 6.3.3. 优先级管理

在某些情况下，可能需要实现音效优先级系统：

```c
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_MEDIUM = 1,
    PRIORITY_HIGH = 2
} SoundPriority;

volatile SoundPriority current_effect_priority = PRIORITY_LOW;

void play_sound_with_priority(const byte *sound_data, SoundPriority priority) {
    // 如果当前音效优先级较低，则允许新音效播放
    if (priority >= current_effect_priority) {
        current_effect_priority = priority;
        play_sound_effect(sound_data);
        
        // 音效播放完成后重置优先级
        current_effect_priority = PRIORITY_LOW;
    }
}
```

### 6.4. 高级技巧

#### 6.4.1. 动态通道分配

对于更复杂的游戏，可以实现动态通道分配系统：

```c
// 注意：Gigatron 系统调用中没有 is_channel_active() 函数
// 以下代码需要通过其他方式实现通道状态跟踪

// 全局变量跟踪通道状态
static byte channel_status = 0;  // 每个bit代表一个通道的状态

int allocate_sound_channel() {
    // 检查通道3和4的状态（bit 2和bit 3）
    if (!(channel_status & (1 << 2))) return 3;
    if (!(channel_status & (1 << 3))) return 4;
    
    // 如果都忙，返回优先级较低的通道4
    return 4;
}

void play_dynamic_sound_effect(const byte *sound_data) {
    int channel = allocate_sound_channel();
    // 标记通道为使用中
    channel_status |= (1 << (channel - 1));
    
    // 使用底层声音API播放音效
    sound_on(channel, 60, 100, 2);
    sound_set_timer(10);
    
    // 播放完成后释放通道（需要在适当的地方调用）
    // channel_status &= ~(1 << (channel - 1));
}
```

#### 6.4.2. 音乐淡出淡入

在播放重要音效时，可以暂时降低音乐音量：

```c
void play_important_sound_effect(const byte *sound_data) {
    // 淡出音乐
    for (int vol = 80; vol >= 20; vol -= 10) {
        sound_volume(1, vol);
        sound_volume(2, vol);
        _wait(2);
    }
    
    // 播放音效
    play_sound_effect(sound_data);
    
    // 淡入音乐
    for (int vol = 20; vol <= 80; vol += 10) {
        sound_volume(1, vol);
        sound_volume(2, vol);
        _wait(2);
    }
}
```

### 6.5. 最佳实践建议

1.  **通道规划**: 在游戏设计阶段就规划好音乐和音效的通道分配。
2.  **音效简洁**: 保持音效简短，避免长时间占用通道。
3.  **音量平衡**: 仔细调整音乐和音效的音量，确保两者都能清晰听到。
4.  **测试**: 在不同游戏场景下测试音乐和音效的混合效果。
5.  **性能考虑**: 频繁的声音操作可能影响游戏性能，需要优化音效播放逻辑。

通过合理使用 Gigatron 的 4 通道声音系统，可以实现丰富的游戏音频体验，同时避免音乐和音效之间的冲突。

## 7. 音乐播放控制详解

基于对 [`music.c`](music/music.c) 和 [`sound.h`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h) 的深入分析，我们可以详细了解 Gigatron 音乐播放系统的各种控制功能。

### 7.1. 播放音乐

#### 7.1.1. 基本播放

音乐播放通过 [`midi_play()`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:85) 函数实现：

```c
// 播放指定的 MIDI 音乐
void play_music(int index) {
    if (index >= 0 && index < NMIDIS) {
        midi_play(midis[index].midi);  // 开始播放音乐
        startclk = _clock();            // 记录开始时间
    }
}
```

在 [`music.c`](music/music.c:136) 的主循环中，音乐播放是这样实现的：
```c
midi_play(midis[mindex].midi);  // 播放当前选中的音乐
```

#### 7.1.2. 播放状态检查

使用 [`midi_playing()`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:88) 函数检查音乐是否正在播放：

```c
while(midi_playing()) {
    // 音乐正在播放时执行的代码
    header_time();      // 更新时间显示
    sample_display();    // 更新音频可视化
    handle_keys();      // 处理用户输入
}
```

### 7.2. 停止音乐

#### 7.2.1. 立即停止

通过传递空指针给 [`midi_play()`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:85) 函数来立即停止音乐：

```c
void stop_music() {
    midi_play(0);  // 传递空指针停止当前播放的音乐
}
```

在 [`music.c`](music/music.c:119) 中，当用户切换歌曲时会调用：
```c
if (nindex != mindex) {
    midi_play(0);  // 停止当前音乐
    mindex = nindex - 1; /* because the main loop adds one */
}
```

### 7.3. 暂停和继续播放

#### 7.3.1. 暂停功能的实现

Gigatron 的 MIDI 播放系统本身不提供直接的暂停功能，但可以通过以下方法实现：

```c
// 全局变量用于保存暂停状态
static const byte *paused_music = 0;
static int is_paused = 0;

void pause_music() {
    if (midi_playing() && !is_paused) {
        // 保存当前播放位置（这需要扩展 MIDI 系统）
        // 简单实现：停止音乐并标记为暂停状态
        midi_play(0);
        is_paused = 1;
        paused_music = midis[mindex].midi;
    }
}

void resume_music() {
    if (is_paused && paused_music) {
        // 恢复播放（从开头开始）
        midi_play(paused_music);
        is_paused = 0;
        startclk = _clock();  // 重置时间计数
    }
}
```

#### 7.3.2. 高级暂停实现（需要系统扩展）

要实现真正的暂停（从暂停位置继续），需要扩展 MIDI 播放系统以支持位置保存和恢复：

```c
// 扩展的暂停结构
typedef struct {
    const byte *current_segment;
    const byte *current_position;
    int frames_until_next;
} MusicState;

static MusicState saved_state;

void advanced_pause_music() {
    if (midi_playing()) {
        // 保存当前播放状态（需要系统级支持）
        // 这需要修改 ROM 中的 MIDI 播放器
        // 注意：以下函数在Gigatron系统调用中不存在
        // 这些功能需要系统级扩展才能实现
        // saved_state.current_segment = get_current_segment();
        // saved_state.current_position = get_current_position();
        // saved_state.frames_until_next = get_frames_until_next();
        
        midi_play(0);
    }
}

void advanced_resume_music() {
    // 注意：restore_midi_state() 函数在Gigatron系统调用中不存在
    // 此功能需要系统级扩展才能实现
    // restore_midi_state(&saved_state);
}
```

### 7.4. 切换曲目

#### 7.4.1. 基本切换

在 [`music.c`](music/music.c:100-122) 中，曲目切换通过 [`handle_keys()`](music/music.c:100) 函数实现：

```c
void handle_keys() {
    int nindex = mindex;
    byte chr = serialRaw;
    byte btn = buttonState;
    
    // 数字键直接选择
    if (chr >= '1' && chr < '1' + NMIDIS) {
        nindex = chr - '1';
    }
    // 方向键切换
    else if ((btn & (buttonUp|buttonLeft)) != (buttonUp|buttonLeft)) {
        buttonState |= buttonLeft | buttonUp;
        nindex -= 1;  // 上一首
    }
    else if ((btn & (buttonDown|buttonRight)) != (buttonDown|buttonRight)) {
        buttonState |= buttonDown | buttonRight;
        nindex += 1;  // 下一首
    }
    
    // 循环处理
    if (nindex < 0)
        nindex = NMIDIS-1;
    else if (nindex >= NMIDIS)
        nindex = 0;
    
    // 如果曲目改变，停止当前音乐并切换
    if (nindex != mindex) {
        midi_play(0);  // 停止当前音乐
        mindex = nindex - 1; /* because the main loop adds one */
    }
}
```

#### 7.4.2. 平滑切换

使用 [`midi_chain()`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:94) 函数实现无缝切换：

```c
void smooth_track_change(int new_index) {
    if (new_index >= 0 && new_index < NMIDIS) {
        // 在当前曲目结束时自动播放下一首
        if (midi_chain(midis[new_index].midi)) {
            // 成功设置链式播放
            mindex = new_index;
        } else {
            // 如果不在最后一段，则立即切换
            midi_play(0);
            midi_play(midis[new_index].midi);
            mindex = new_index;
        }
    }
}
```

### 7.5. 循环播放

#### 7.5.1. 单曲循环

```c
void single_track_loop() {
    // 当音乐播放结束时重新开始
    if (!midi_playing()) {
        midi_play(midis[mindex].midi);
        startclk = _clock();  // 重置时间
    }
}
```

#### 7.5.2. 播放列表循环

在 [`music.c`](music/music.c:142-144) 的主循环中实现了播放列表循环：

```c
// 主循环中的循环逻辑
for(;;) {
    header_name();
    midi_play(midis[mindex].midi);
    while(midi_playing()) {
        header_time();
        sample_display();
        handle_keys();
    }
    
    // 当前曲目播放完毕，切换到下一首
    mindex = mindex + 1;
    if (mindex >= NMIDIS)
        mindex = 0;  // 循环到第一首
    _wait(30);
}
```

#### 7.5.3. 随机播放

```c
void random_play() {
    static int last_index = -1;
    int new_index;
    
    do {
        // 注意：rand() 函数可能不是标准Gigatron库的一部分
        // 可以使用简单的伪随机数生成器替代
        static unsigned int seed = 12345;
        seed = seed * 1103515245 + 12345;
        new_index = (seed / 65536) % NMIDIS;
    } while (new_index == last_index && NMIDIS > 1);
    
    last_index = new_index;
    midi_play(midis[new_index].midi);
    mindex = new_index;
}
```

### 7.6. 高级播放控制

#### 7.6.1. 音量控制

```c
void set_music_volume(int volume) {
    // 设置音乐通道的音量
    sound_volume(1, volume);  // 通道 1
    sound_volume(2, volume);  // 通道 2
}

void fade_music_in(int target_volume, int steps) {
    int current_volume = 0;
    for (int i = 0; i <= steps; i++) {
        current_volume = (target_volume * i) / steps;
        set_music_volume(current_volume);
        _wait(2);  // 短暂延迟
    }
}

void fade_music_out(int steps) {
    for (int i = steps; i >= 0; i--) {
        int volume = (127 * i) / steps;
        set_music_volume(volume);
        _wait(2);
    }
}
```

#### 7.6.2. 播放进度控制

```c
// 获取当前播放时间（秒）
unsigned int get_playback_time() {
    return (_clock() - startclk) / 60;
}

// 获取曲目总长度（需要扩展系统）
unsigned int get_track_length(int index) {
    // 注意：此功能需要预先计算并存储曲目长度
    // 或者扩展系统以支持获取曲目长度
    // return track_lengths[index];
    return 0;  // 临时返回值
}

// 跳转到指定位置
void seek_to_position(int seconds) {
    // 注意：Gigatron MIDI 播放器不支持位置跳转
    // 此功能需要系统级扩展才能实现
}
```

### 7.7. 最佳实践

1.  **状态管理**: 使用全局变量跟踪播放状态、当前曲目索引等。
2.  **用户反馈**: 在屏幕上显示当前播放状态、曲目信息等。
3.  **错误处理**: 检查索引范围、播放状态等，避免程序崩溃。
4.  **性能优化**: 避免频繁的音乐操作，使用适当的延迟。
5.  **用户体验**: 提供直观的控制方式（数字键、方向键等）。

通过这些控制功能，可以构建功能完整的音乐播放器，满足不同的使用场景和用户需求。

**总结**:

该 Gigatron 音乐程序支持在事件级别实时改变**音量**和**音高**。然而，**波形**在程序启动时固定，无法实时改变，并且**弯音轮**功能在此 `.gtmid` 格式和 `gtmid2c` 工具的实现中不被支持。

对于游戏开发，Gigatron 的 4 通道声音系统提供了足够的灵活性来同时管理背景音乐和音效，通过合理的通道分配和冲突避免策略，可以实现良好的音频体验。

在音乐播放控制方面，Gigatron 提供了基本的播放、停止和状态检查功能，通过 [`midi_play()`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:85)、[`midi_playing()`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:88) 和 [`midi_chain()`](D:/working/vscode-projects/Reference_Project/Gigatron/glcc-20251127-2.6.37/lib/gigatron-lcc/include/gigatron/sound.h:94) 等系统调用，可以实现播放列表循环、曲目切换等基本功能。更高级的功能如暂停/继续、位置跳转等需要系统级扩展或巧妙的编程技巧。