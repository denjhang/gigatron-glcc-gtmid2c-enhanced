# `music/bwv846f.gbas.c` 音乐数据解析文档

本文档将分析 Gigatron TTL 计算机的 `music/bwv846f.gbas.c` 文件中定义的音乐数据结构，并结合修改后的 `music/sound.s` 中的汇编代码，解释这些音乐数据是如何被解析和播放的。

## `bwv846f.gbas.c` 文件结构概述

`bwv846f.gbas.c` 文件是由 `gbas_to_c.py` 工具从 `bwv846f.gbas` 文件生成的 C 语言源文件。它主要包含一系列 `nohop static const byte bwv846fXXX[]` 数组，这些数组存储了 MIDI 事件序列，以及一个 `nohop const byte *bwv846f[]` 数组，它是一个指向所有 MIDI 事件序列数组的指针数组。

## 宏定义

文件开头定义了几个宏，用于简化 MIDI 事件的表示。与 `agony.c` 不同，`bwv846f.gbas.c` 引入了一个新的宏 `W`，用于设置 `wavX` 寄存器。

*   **`#define D(x) x`**: 定义延迟事件。`x` 表示等待的帧数。
*   **`#define X(c) 127+(c)`**: 定义通道 `c` 的音符关闭事件。`127` 是一个基准值，加上通道号 `c` (0-3) 得到 `127` 到 `130`。
*   **`#define N(c,n) 143+(c),(n)`**: 定义通道 `c` 的音符打开事件。`143` 是一个基准值，加上通道号 `c` 得到 `143` 到 `146`。`n` 是音符值。
*   **`#define M(c,n,v) 159+(c),(n),(v)`**: 定义通道 `c` 的音符打开事件，并设置波形/音量 (`wavA`)。`159` 是一个基准值，加上通道号 `c` 得到 `159` 到 `162`。`n` 是音符值，`v` 是波形/音量值。
*   **`#define W(c,n,v,w) 175+(c),(n),(v),(w)`**: **新增宏**。定义通道 `c` 的音符打开事件，并设置波形/音量 (`wavA=v`) 和额外的波形参数 (`wavX=w`)。`175` 是一个基准值，加上通道号 `c` 得到 `175` 到 `178`。`n` 是音符值，`v` 是 `wavA` 值，`w` 是 `wavX` 值。
*   **`#define byte unsigned char`**: 定义 `byte` 为无符号字符。
*   **`#define nohop __attribute__((nohop))`**: 禁用优化，确保代码按预期生成。

## 音乐数据结构

`bwv846f.gbas.c` 中的音乐数据以字节数组的形式存储，每个数组代表一个 MIDI 序列片段。例如：

```c
nohop static const byte bwv846f000[] = {
  D(12),W(1,60,64,3),D(6),W(1,60,67,1),D(6),W(1,60,69,1),D(6),W(1,60,72,1),D(6),W(1,60,74,1),
  // ...
  0
};
```

每个宏展开后会生成一个或多个字节。例如：

*   `D(12)` 会生成一个字节 `12`。
*   `X(1)` 会生成一个字节 `127 + 1 = 128`。
*   `N(1,60)` 会生成两个字节 `143 + 1 = 144` 和 `60`。
*   `M(1,60,64)` 会生成三个字节 `159 + 1 = 160`、`60` 和 `64`。
*   `W(1,60,64,3)` 会生成四个字节 `175 + 1 = 176`、`60`、`64` 和 `3`。

这些字节序列构成了 MIDI 事件流。每个序列以 `0` 字节结束，表示该序列的结束。

最后，`bwv846f` 数组是一个指向这些序列的指针数组：

```c
nohop const byte *bwv846f[] = {
  bwv846f000,
  bwv846f001,
  // ...
  0
};
```

这个数组允许按顺序播放多个 MIDI 序列，形成完整的乐曲。

## `sound.s` 中的解析过程

`music/sound.s` 中的 `code_midi_tick()` 函数是解析这些音乐数据的核心。它通过 `_midi.p` 指针遍历 MIDI 事件流，并根据事件类型执行相应的操作。

### `code_midi_tick()` 的修改

`code_midi_tick()` 的主要作用是识别事件类型并分发。对于音符打开事件，它会将控制权交给 `code_midi_note()`。由于 `W` 宏的命令字节基值 (`175+c`) 大于 `M` 宏的基值 (`159+c`)，原有的 `SUBI(0x20)` 逻辑仍然能正确地将 `W` 宏事件识别为音符打开事件，并跳转到 `.midi_note` 处理。因此，`code_midi_tick()` **无需修改**。

### `code_midi_note()` 的修改

`code_midi_note()` 函数负责从 MIDI 事件流中读取音符值、波形/音量值 (`wavA`) 和新增的波形参数 (`wavX`)，并将其写入 Gigatron 的声音硬件寄存器。为了支持 `W` 宏，`code_midi_note()` 进行了以下修改：

**修改前的 `code_midi_note()` 关键代码片段:**

```assembly
            ADDI(0x10);STW(vLR)
            LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')
            LDW(vLR);_BLT('.freq')
            LDI(0xfa);ST('_midi.tmp')
            LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')
            label('.freq')
            LDI(0xfc);ST('_midi.tmp')
            LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')
            LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)
            LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')
```

**修改后的 `code_midi_note()` 关键代码片段:**

```assembly
            # Read note 'n' and store it in _midi.tmp
            LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.tmp')

            # Check if it's an M or W command (has wavA parameter)
            # _midi.cmd still holds the command type (143+c, 159+c, or 175+c)
            LDW('_midi.cmd');SUBI(159);_BLT('.freq_wavA_skip') # If < 0, it's N command (143+c), skip wavA

            # It's an M or W command, read wavA (v) and write to 0xfa
            LDI(0xfa);ST(vACL) # Use vACL as temporary for address 0xfa
            LDW('_midi.p');PEEK();INC('_midi.p');POKE(vACL)

            # Check if it's a W command (has wavX parameter)
            LDW('_midi.cmd');SUBI(175);_BLT('.freq_wavX_skip') # If < 0, it's M command (159+c), skip wavX

            # It's a W command, read wavX (w) and write to 0xfb
            LDI(0xfb);ST(vACL) # Use vACL as temporary for address 0xfb
            LDW('_midi.p');PEEK();INC('_midi.p');POKE(vACL)

            label('.freq_wavX_skip') # M commands jump here after wavA
            label('.freq_wavA_skip') # N commands jump here after note 'n'

            # Now set the frequency
            LDI(0xfc);ST(vACL) # Use vACL as temporary for address 0xfc
            LDW('_midi.tmp');STW('_midi.cmd') # Move 'n' from _midi.tmp to _midi.cmd for notesTable lookup
            LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')
            LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)
            LDW(vLR);DOKE(vACL);_CALLJ('.getcmd')
```

**修改解析:**

1.  **读取音符 `n`**:
    *   `LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.tmp')`: 从 `_midi.p` 指向的地址读取音符值 `n`，并将其存储到 `_midi.tmp`。`_midi.p` 递增。
2.  **判断 `wavA` 参数 (M 或 W 宏)**:
    *   `LDW('_midi.cmd');SUBI(159);_BLT('.freq_wavA_skip')`: `_midi.cmd` 在进入 `code_midi_note()` 时仍然保存着 `code_midi_tick()` 传递的原始命令字节 (例如 `143+c`, `159+c`, `175+c`)。这里将其与 `159` 比较。如果小于 `159` (即 `N` 宏事件)，则跳转到 `.freq_wavA_skip`，跳过 `wavA` 和 `wavX` 的处理。
3.  **处理 `wavA` 参数**:
    *   `LDI(0xfa);ST(vACL)`: 将 `0xfa` (声音 `wavA` 寄存器地址) 存储到 `vACL`。
    *   `LDW('_midi.p');PEEK();INC('_midi.p');POKE(vACL)`: 从 `_midi.p` 指向的地址读取 `wavA` 值 `v`，并将其写入 `0xfa` 寄存器。`_midi.p` 递增。
4.  **判断 `wavX` 参数 (W 宏)**:
    *   `LDW('_midi.cmd');SUBI(175);_BLT('.freq_wavX_skip')`: 将 `_midi.cmd` 与 `175` 比较。如果小于 `175` (即 `M` 宏事件)，则跳转到 `.freq_wavX_skip`，跳过 `wavX` 的处理。
5.  **处理 `wavX` 参数**:
    *   `LDI(0xfb);ST(vACL)`: 将 `0xfb` (声音 `wavX` 寄存器地址) 存储到 `vACL`。
    *   `LDW('_midi.p');PEEK();INC('_midi.p');POKE(vACL)`: 从 `_midi.p` 指向的地址读取 `wavX` 值 `w`，并将其写入 `0xfb` 寄存器。`_midi.p` 递增。
6.  **设置频率**:
    *   `label('.freq_wavX_skip')` 和 `label('.freq_wavA_skip')`: 这些标签用于不同类型的事件跳过不必要的参数处理。
    *   `LDI(0xfc);ST(vACL)`: 将 `0xfc` (声音频率寄存器地址) 存储到 `vACL`。
    *   `LDW('_midi.tmp');STW('_midi.cmd')`: 将之前存储在 `_midi.tmp` 中的音符值 `n` 重新加载到 `_midi.cmd`，以便用于 `notesTable` 的查找。
    *   `LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')`: 计算 `notesTable` 中对应音符频率的地址。
    *   `LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)`: 从计算出的地址读取 16 位频率值到 `vLR`。
    *   `LDW(vLR);DOKE(vACL);_CALLJ('.getcmd')`: 将 `vLR` 中的频率值写入 `0xfc` 寄存器，设置频率。然后跳转回 `code_midi_tick()` 的 `.getcmd` 标签，继续处理下一个 MIDI 事件。

### 总结

通过上述修改，`music/sound.s` 中的 `code_midi_note()` 函数现在能够完全支持 `bwv846f.gbas.c` 中引入的 `W` 宏。它能够根据命令类型动态地读取和设置 `wavA` (0xfa) 和 `wavX` (0xfb) 寄存器，从而实现更丰富的波形控制。`code_midi_tick()` 保持不变，因为它已经能够正确地将所有音符打开事件分发到 `code_midi_note()`。