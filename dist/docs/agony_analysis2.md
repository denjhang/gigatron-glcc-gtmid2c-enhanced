# `music/midi/agony.c` 音乐数据解析文档

本文档将分析 Gigatron TTL 计算机的 `music/midi/agony.c` 文件中定义的音乐数据结构，并结合 `music/sound.s` 中的汇编代码，解释这些音乐数据是如何被解析和播放的。

## `agony.c` 文件结构概述

`agony.c` 文件是由 `gtmid2c` 工具从 MIDI 文件 `midi/agony.gtmid` 生成的 C 语言源文件。它主要包含一系列 `nohop static const byte agonyXXX[]` 数组，这些数组存储了 MIDI 事件序列，以及一个 `nohop const byte *agony[]` 数组，它是一个指向所有 MIDI 事件序列数组的指针数组。

## 宏定义

文件开头定义了几个宏，用于简化 MIDI 事件的表示：

*   **`#define D(x) x`**: 定义延迟事件。`x` 表示等待的帧数。
*   **`#define X(c) 127+(c)`**: 定义通道 `c` 的音符关闭事件。`127` 是一个基准值，加上通道号 `c` (0-3) 得到 `127` 到 `130`。
*   **`#define N(c,n) 143+(c),(n)`**: 定义通道 `c` 的音符打开事件。`143` 是一个基准值，加上通道号 `c` 得到 `143` 到 `146`。`n` 是音符值。
*   **`#define M(c,n,v) 159+(c),(n),(v)`**: 定义通道 `c` 的音符打开事件，并设置波形/音量。`159` 是一个基准值，加上通道号 `c` 得到 `159` 到 `162`。`n` 是音符值，`v` 是波形/音量值。
*   **`#define byte unsigned char`**: 定义 `byte` 为无符号字符。
*   **`#define nohop __attribute__((nohop))`**: 禁用优化，确保代码按预期生成。

## 音乐数据结构

`agony.c` 中的音乐数据以字节数组的形式存储，每个数组代表一个 MIDI 序列片段。例如：

```c
14 | nohop static const byte agony000[] = {
15 |   M(1,68,77),M(2,56,77),M(3,32,77),M(4,44,77),M(4,44,90),M(4,44,112),D(15),
16 |   X(2),X(3),D(15),N(2,73),D(15),N(1,71),D(7),X(2),D(3),X(1),D(5),N(1,70),X(4),
17 |   D(15),N(1,68),D(45),M(1,68,112),M(2,80,122),M(3,44,122),D(60),M(2,80,117),
18 |   N(4,56),M(3,44,97),D(15),M(3,82,115),M(2,58,107),M(4,46,95),D(15),M(4,83,112),
19 |   M(2,59,102),M(3,47,92),D(15),M(3,85,110),M(2,61,97),M(4,49,90),D(15),M(4,80,102),
20 |   N(1,68),M(2,56,77),M(3,32,77),M(4,80,117),D(15),X(2),X(3),X(4),D(45),M(4,75,77),
21 |   M(2,87,107),N(3,63),M(2,87,112),D(15),X(2),X(4),D(10),X(3),D(35),M(2,73,77),
22 |   M(3,85,112),N(4,61),M(3,85,107),D(15),X(2),X(3),D(45),M(3,92,117),X(1),X(4),
23 |   D(15),X(3),D(45),M(3,80,77),M(1,32,77),N(2,44),N(4,56),M(2,44,87),M(3,80,105),
24 |   M(2,44,105),D(15),M(2,75,77),X(1),X(3),X(4),D(15),N(2,71),N(1,44),D(15),M(1,68,92),
25 |   X(2),D(15),M(1,63,95),N(2,44),N(3,75),M(4,39,105),D(15),M(1,59,97),X(2),D(15),
26 |   M(1,56,100),N(2,44),D(15),M(3,75,77),M(1,51,102),X(2),D(15),M(1,63,77),N(2,80),
27 |   M(2,80,105),M(3,44,105),X(4),D(15),X(1),X(2),D(15),N(1,56),M(3,44,77),D(15),
28 |   N(3,61),N(1,75),D(15),M(1,51,97),M(2,80,77),N(3,44),M(3,44,87),N(4,75),0
29 | };
```

每个宏展开后会生成一个或多个字节。例如：

*   `D(15)` 会生成一个字节 `15`。
*   `X(2)` 会生成一个字节 `127 + 2 = 129`。
*   `N(2,73)` 会生成两个字节 `143 + 2 = 145` 和 `73`。
*   `M(1,68,77)` 会生成三个字节 `159 + 1 = 160`、`68` 和 `77`。

这些字节序列构成了 MIDI 事件流。每个序列以 `0` 字节结束，表示该序列的结束。

最后，`agony` 数组是一个指向这些序列的指针数组：

```c
110 | nohop const byte *agony[] = {
111 |   agony000,
112 |   agony001,
113 |   agony002,
114 |   agony003,
115 |   agony004,
116 |   agony005,
117 |   0
118 | };
```

这个数组允许按顺序播放多个 MIDI 序列，形成完整的乐曲。

## `sound.s` 中的解析过程

`music/sound.s` 中的 `code_midi_tick()` 函数是解析这些音乐数据的核心。它通过 `_midi.p` 指针遍历 MIDI 事件流，并根据事件类型执行相应的操作。

让我们回顾 `code_midi_tick()` 的关键部分：

```assembly
192 |         def code_midi_tick():
193 |             nohop()
194 |             label('.midi_tick')
195 |             PUSH()
196 |             # obtain command
197 |             label('.getcmd')
198 |             LDW('_midi.p');PEEK();_BNE('.docmd')
199 |             LDW('_midi.q');DEEK();_BEQ('.fin')
200 |             STW('_midi.p');INC('_midi.q');INC('_midi.q');_BRA('.getcmd')
201 |             # process command
202 |             label('.docmd')
203 |             INC('_midi.p');STW('_midi.cmd')
204 |             ANDI(3);INC(vACL);ST(v('_midi.tmp')+1)
205 |             # delay
206 |             LDW('_midi.cmd');SUBI(0x80);_BGE('.xcmd')
207 |             if args.cpu >= 7:
208 |                 LD('_midi.cmd');ADDV('_midi.t')
209 |             else:
210 |                 LD('_midi.cmd');ADDW('_midi.t');STW('_midi.t')
211 |             POP();RET();
212 |             # note off
213 |             label('.xcmd')
214 |             SUBI(0x10);_BGE('.ncmd')
215 |             LDI(0xfc);ST('_midi.tmp')
216 |             LDI(0);DOKE('_midi.tmp');_BRA('.getcmd')
217 |             # note on
218 |             label('.ncmd')
219 |             SUBI(0x20)
220 |             if args.cpu >= 6:
221 |                 JLT('.midi_note')
222 |             else:
223 |                 _BGE('.fin');CALLI('.midi_note')
224 |             # end
225 |             label('.fin')
226 |             POP();POP() # pop one more level
227 |             LDI(0);ST('soundTimer');STW('_midi.q')
228 |             label('.ret')
229 |             RET()
```

### 解析流程

1.  **获取命令 (`.getcmd`)**:
    *   `LDW('_midi.p');PEEK();_BNE('.docmd')`: 从 `_midi.p` 指向的地址读取一个字节 (MIDI 事件数据)。如果该字节非零，说明是一个有效的 MIDI 事件，跳转到 `.docmd` 进行处理。
    *   `LDW('_midi.q');DEEK();_BEQ('.fin')`: 如果读取的字节是 `0` (表示当前 MIDI 序列结束)，则检查 `_midi.q`。`_midi.q` 是一个指向 `agony` 数组的指针，它存储了下一个 MIDI 序列的地址。如果 `_midi.q` 指向的地址内容为 `0`，说明所有 MIDI 序列都已播放完毕，跳转到 `.fin` 结束播放。
    *   `STW('_midi.p');INC('_midi.q');INC('_midi.q');_BRA('.getcmd')`: 如果 `_midi.q` 指向的地址内容非 `0`，说明还有下一个 MIDI 序列。将 `_midi.q` 指向的下一个 MIDI 序列的地址加载到 `_midi.p`，然后 `_midi.q` 递增两次 (因为 `agony` 数组存储的是字地址)，继续从新的 `_midi.p` 获取命令。

2.  **处理命令 (`.docmd`)**:
    *   `INC('_midi.p');STW('_midi.cmd')`: `_midi.p` 递增，指向下一个字节。当前读取的字节 (MIDI 事件类型) 存储在 `_midi.cmd` 中。
    *   `LDW('_midi.cmd');SUBI(0x80);_BGE('.xcmd')`: 将 `_midi.cmd` (MIDI 事件类型) 与 `0x80` (128) 进行比较。
        *   如果 `_midi.cmd < 0x80` (即 `D(x)` 宏生成的字节 `x`)，表示这是一个延迟事件。
            *   `LD('_midi.cmd');ADDW('_midi.t');STW('_midi.t')` (或 `ADDV` for CPU >= 7): 将延迟值 `x` 加到 `_midi.t` (MIDI 计时器) 中。
            *   `POP();RET()`: 恢复寄存器并返回，等待计时器达到延迟时间。
        *   如果 `_midi.cmd >= 0x80`，则继续处理音符事件。

3.  **音符关闭 (`.xcmd`)**:
    *   `SUBI(0x10);_BGE('.ncmd')`: 从 `_midi.cmd` 减去 `0x10` (16)。
        *   如果 `_midi.cmd - 0x80 < 0x10` (即 `0x80 <= _midi.cmd < 0x90`，对应 `X(c)` 宏生成的 `127+c`，即 `127` 到 `130`)，表示这是一个音符关闭事件。
            *   `LDI(0xfc);ST('_midi.tmp')`: 将 `0xfc` (频率寄存器地址) 存储到 `_midi.tmp`。
            *   `LDI(0);DOKE('_midi.tmp');_BRA('.getcmd')`: 将 `0` 写入频率寄存器，从而关闭当前通道的声音，然后跳转到 `.getcmd` 获取下一个命令。

4.  **音符打开 (`.ncmd`)**:
    *   `SUBI(0x20)`: 从 `_midi.cmd` 减去 `0x20` (32)。
        *   如果 `_midi.cmd - 0x80 - 0x10 < 0x20` (即 `0x90 <= _midi.cmd < 0xB0`，对应 `N(c,n)` 宏生成的 `143+c`，即 `143` 到 `146`)，表示这是一个音符打开事件 (不带音量/波形)。
            *   `CALLI('.midi_note')`: 调用 `.midi_note` 子程序来处理音符打开。
        *   如果 `_midi.cmd - 0x80 - 0x10 >= 0x20` (即 `_midi.cmd >= 0xB0`，对应 `M(c,n,v)` 宏生成的 `159+c`，即 `159` 到 `162`)，表示这是一个音符打开事件 (带音量/波形)。
            *   `CALLI('.midi_note')`: 调用 `.midi_note` 子程序来处理音符打开。

### `code_midi_note()` 的作用

`code_midi_note()` 函数 (在 `sound.s` 的 178-190 行) 负责从 MIDI 事件流中读取音符值和音量/波形值，并将其写入 Gigatron 的声音硬件寄存器。

**源码片段:**

```assembly
178 |         def code_midi_note():
179 |             nohop()
180 |             label('.midi_note')
181 |             ADDI(0x10);STW(vLR)
182 |             LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')
183 |             LDW(vLR);_BLT('.freq')
184 |             LDI(0xfa);ST('_midi.tmp')
185 |             LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')
186 |             label('.freq')
187 |             LDI(0xfc);ST('_midi.tmp')
188 |             LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')
189 |             LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)
190 |             LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')
```

**逐行解析:**

*   **`180 | label('.midi_note')`**: `code_midi_note` 函数的入口标签。
*   **`181 | ADDI(0x10);STW(vLR)`**: 这行代码将 `0x10` 加到 `ACL` (累加器)，然后将结果存储到 `vLR` (虚拟返回地址寄存器)。这通常用于调整栈帧或处理函数参数。
*   **`182 | LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')`**:
    *   `LDW('_midi.p')`: 将 `_midi.p` (指向当前 MIDI 事件数据的指针) 的值加载到 `ACL`。
    *   `PEEK()`: 从 `ACL` 指向的内存地址读取一个字节，并将其加载到 `ACL`。这个字节就是 MIDI 事件的第一个参数，通常是音符值 `n`。
    *   `INC('_midi.p')`: `_midi.p` 递增，指向下一个字节。
    *   `STW('_midi.cmd')`: 将读取到的音符值 `n` 存储到 `_midi.cmd` 变量中。
*   **`183 | LDW(vLR);_BLT('.freq')`**:
    *   `LDW(vLR)`: 将 `vLR` 的值加载到 `ACL`。
    *   `_BLT('.freq')`: 如果 `ACL` 的值小于某个阈值 (这里 `vLR` 的值在 `181` 行被修改过，其具体含义取决于调用上下文，但通常用于判断是否需要处理音量/波形参数)，则跳转到 `.freq` 标签，跳过音量/波形设置。这对应于 `N(c,n)` 类型的事件，它只有音符没有音量。
*   **`184 | LDI(0xfa);ST('_midi.tmp')`**: 如果没有跳转到 `.freq` (即是 `M(c,n,v)` 类型的事件)，则将 `0xfa` (声音音量/波形寄存器地址) 存储到 `_midi.tmp`。
*   **`185 | LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')`**:
    *   `LDW('_midi.p');PEEK()`: 从 `_midi.p` 指向的地址读取一个字节，这个字节是音量/波形值 `v`。
    *   `INC('_midi.p')`: `_midi.p` 再次递增，指向下一个事件。
    *   `POKE('_midi.tmp')`: 将读取到的音量/波形值 `v` 写入 `_midi.tmp` (即 `0xfa`) 指向的寄存器，从而设置声音的音量和波形。
*   **`186 | label('.freq')`**: 标签 `.freq`，音量/波形设置完成后或跳过音量/波形设置后会到达这里。
*   **`187 | LDI(0xfc);ST('_midi.tmp')`**: 将 `0xfc` (声音频率寄存器地址) 存储到 `_midi.tmp`。
*   **`188 | LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')`**:
    *   `LDWI(v('notesTable')-22)`: 加载 `notesTable` 的基地址 (减去 `22` 是因为 `notesTable` 可能从某个偏移量开始)。
    *   `ADDW('_midi.cmd');ADDW('_midi.cmd')`: 将之前存储的音符值 `n` (在 `_midi.cmd` 中) 乘以 2，并加到 `notesTable` 的基地址上。这是因为 `notesTable` 存储的是 16 位字 (word) 频率值，每个音符占用两个字节。
    *   `STW('_midi.cmd')`: 将计算出的 `notesTable` 中对应音符频率的地址存储回 `_midi.cmd`。
*   **`189 | LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)`**:
    *   `LUP(0);ST(vLR)`: 从 `_midi.cmd` 指向的地址 (即 `notesTable` 中音符频率的低字节地址) 读取低字节，存储到 `vLR`。
    *   `LDW('_midi.cmd');LUP(1);ST(vLR+1)`: 从 `_midi.cmd` 指向的地址 (即 `notesTable` 中音符频率的高字节地址) 读取高字节，存储到 `vLR+1`。
    *   这两行代码将 `notesTable` 中对应音符的 16 位频率值加载到 `vLR` 和 `vLR+1`。
*   **`190 | LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')`**:
    *   `LDW(vLR)`: 将 `vLR` (现在包含音符频率值) 加载到 `ACL`。
    *   `DOKE('_midi.tmp')`: 将 `ACL` 中的频率值写入 `_midi.tmp` (即 `0xfc`) 指向的频率寄存器，从而设置声音的频率。
    *   `_CALLJ('.getcmd')`: 调用 `.getcmd` 标签，继续获取下一个 MIDI 命令。

**示例解析:**

假设 `_midi.p` 当前指向 `agony000` 数组中的 `M(1,68,77)` 事件。

1.  `code_midi_tick()` 识别到 `M(1,68,77)` 事件，并调用 `.midi_note`。此时 `_midi.p` 指向 `68` (音符值)。
2.  **`182 | LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')`**:
    *   `_midi.p` 指向 `68`。`PEEK()` 读取 `68`。
    *   `_midi.p` 递增，指向 `77`。
    *   `_midi.cmd` 存储 `68` (音符值)。
3.  **`183 | LDW(vLR);_BLT('.freq')`**: 假设 `vLR` 的值表示这是一个 `M` 事件，不跳转到 `.freq`。
4.  **`184 | LDI(0xfa);ST('_midi.tmp')`**: `_midi.tmp` 存储 `0xfa` (音量/波形寄存器地址)。
5.  **`185 | LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')`**:
    *   `_midi.p` 指向 `77`。`PEEK()` 读取 `77` (音量值)。
    *   `_midi.p` 递增，指向下一个事件的起始字节。
    *   `POKE('_midi.tmp')`: 将 `77` 写入 `0xfa` 寄存器，设置音量和波形。
6.  **`186 | label('.freq')`**: 到达 `.freq` 标签。
7.  **`187 | LDI(0xfc);ST('_midi.tmp')`**: `_midi.tmp` 存储 `0xfc` (频率寄存器地址)。
8.  **`188 | LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')`**:
    *   使用 `_midi.cmd` 中存储的音符值 `68`，计算 `notesTable` 中对应频率的地址。
9.  **`189 | LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)`**: 从计算出的地址读取 16 位频率值到 `vLR`。
10. **`190 | LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')`**: 将 `vLR` 中的频率值写入 `0xfc` 寄存器，设置频率。然后跳转回 `code_midi_tick()` 的 `.getcmd` 标签，继续处理下一个 MIDI 事件。

**总结:**

`code_midi_note()` 的核心职责是：
1.  从 MIDI 事件流中读取音符值。
2.  如果事件包含音量/波形参数，则读取该参数并写入相应的声音硬件寄存器。
3.  根据音符值在 `notesTable` 中查找对应的频率。
4.  将查找到的频率写入声音硬件的频率寄存器。
5.  最后，跳转回 `code_midi_tick()` 的 `.getcmd` 标签，继续处理下一个 MIDI 事件。

通过这种方式，`agony.c` 中定义的紧凑型 MIDI 事件数据被 `sound.s` 中的汇编代码有效地解析并转换为 Gigatron 硬件可识别的声音控制信号。

### 总结

`agony.c` 中的音乐数据以一种紧凑的字节码形式存储，通过宏定义将 MIDI 事件编码为单字节或多字节序列。`sound.s` 中的 `code_midi_tick()` 函数作为核心解析器，在每个系统节拍中遍历这些字节序列。它根据字节值判断事件类型 (延迟、音符关闭、音符打开)，并调用相应的子程序 (如 `code_midi_note()`) 来操作 Gigatron 的声音硬件寄存器，从而实现 MIDI 音乐的播放。`_midi.q` 数组则用于实现多个音乐片段的链式播放。