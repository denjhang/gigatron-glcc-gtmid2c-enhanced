# `music/sound.s` 源码分析文档

本文档将逐行分析 Gigatron TTL 计算机的 `music/sound.s` 汇编源码，解释其功能和实现细节。

## 文件结构概述

`sound.s` 文件主要定义了 Gigatron 上的声音控制和 MIDI 播放相关的汇编函数。它包含以下几个主要部分：

1.  **通用声音控制函数**: `sound_reset` (重置声音), `sound_all_off` (关闭所有声音), `sound_reset_waveforms` (重置波形), `sound_sine_waveform` (生成正弦波形)。
2.  **频率和音符处理函数**: `sound_freq` (设置频率), `sound_note` (设置音符), `note_on` (打开音符)。
3.  **MIDI 播放相关函数**: 在 `args.cpu >= 5` 的条件下，定义了 MIDI 播放所需的变量、中断处理和播放逻辑。

## 源码逐行分析

### 1. 通用声音控制

#### `code_reset()` - 声音重置和关闭所有声音

这个函数用于初始化声音系统，并提供一个关闭所有声音的入口。

```assembly
1 | def scope():
2 |
3 |     # ----------------------------------------
4 |     # general stuff
5 |
6 |     def code_reset():
7 |         # extern void sound_reset(int wave)
8 |         nohop()
9 |         label('sound_reset')
10 |         _MOVIW(0x1fa,T0)
11 |         LDW(R8-1);ORI(255);XORI(255)
12 |         PUSH();_CALLI('.go');POP()
13 |         label('sound_all_off')
14 |         _MOVIW(0x1fc,T0);LDI(0)
15 |         label('.go')
16 |         DOKE(T0);INC(T0+1)
17 |         DOKE(T0);INC(T0+1)
18 |         DOKE(T0);INC(T0+1)
19 |         DOKE(T0);RET()
20 |
21 |     module(name='sound_reset.s',
22 |            code=[('EXPORT','sound_reset'),
23 |                  ('EXPORT','sound_all_off'),
24 |                  ('CODE','sound_reset',code_reset)] )
```

*   **`1-2 | def scope():`**: Python 作用域定义，用于组织汇编代码块。
*   **`3-4 | # ---------------------------------------- # general stuff`**: 注释，表示通用功能部分。
*   **`6 | def code_reset():`**: 定义一个 Python 函数 `code_reset`，它将生成汇编代码。
*   **`7 | # extern void sound_reset(int wave)`**: C 语言风格的注释，说明 `sound_reset` 函数的外部接口。
*   **`8 | nohop()`**: 禁用优化，确保代码按预期生成。
*   **`9 | label('sound_reset')`**: 定义汇编标签 `sound_reset`，这是 `sound_reset` 函数的入口点。
*   **`10 | _MOVIW(0x1fa,T0)`**: 将立即数 `0x1fa` (声音控制寄存器地址) 移动到临时寄存器 `T0`。
*   **`11 | LDW(R8-1);ORI(255);XORI(255)`**: 这行代码看起来像是在清除或初始化某个值，`R8-1` 通常是栈上的参数。`ORI(255);XORI(255)` 实际上是将 `ACL` 寄存器清零。
*   **`12 | PUSH();_CALLI('.go');POP()`**: 调用 `.go` 标签处的代码，然后恢复栈。这是一种间接跳转到 `.go` 的方式。
*   **`13 | label('sound_all_off')`**: 定义汇编标签 `sound_all_off`，这是关闭所有声音的入口点。
*   **`14 | _MOVIW(0x1fc,T0);LDI(0)`**: 将立即数 `0x1fc` (另一个声音控制寄存器地址) 移动到 `T0`，并将 `0` 加载到 `ACL`。
*   **`15 | label('.go')`**: 定义内部标签 `.go`。
*   **`16 | DOKE(T0);INC(T0+1)`**: 将 `ACL` (当前为 `0`) 的值写入 `T0` 指向的地址，然后 `T0` 递增。这会关闭一个声道的输出。
*   **`17 | DOKE(T0);INC(T0+1)`**: 写入下一个地址，关闭第二个声道。
*   **`18 | DOKE(T0);INC(T0+1)`**: 写入下一个地址，关闭第三个声道。
*   **`19 | DOKE(T0);RET()`**: 写入最后一个地址，关闭第四个声道，然后返回。
*   **`21-24 | module(...)`**: 定义一个模块，导出 `sound_reset` 和 `sound_all_off` 标签，并将 `code_reset` 函数生成的代码与 `sound_reset` 标签关联。

#### `code_defwave()` - 重置波形

这个函数用于重置声音波形和初始化噪声发生器。

```assembly
26 |     def code_defwave():
27 |         nohop()
28 |         label('sound_reset_waveforms')
29 |         _MOVIW('SYS_ResetWaveforms_v4_50','sysFn')
30 |         LDI(0);SYS(50)
31 |         _MOVIW('SYS_ShuffleNoise_v4_46','sysFn')
32 |         LDI(0);SYS(46);SYS(46);SYS(46);SYS(46)
33 |         RET()
34 |
35 |     module(name='sound_reset_waveforms.s',
36 |            code=[('EXPORT','sound_reset_waveforms'),
37 |                  ('CODE','sound_reset_waveforms',code_defwave)] )
```

*   **`26 | def code_defwave():`**: 定义 Python 函数 `code_defwave`。
*   **`27 | nohop()`**: 禁用优化。
*   **`28 | label('sound_reset_waveforms')`**: 定义汇编标签 `sound_reset_waveforms`。
*   **`29 | _MOVIW('SYS_ResetWaveforms_v4_50','sysFn')`**: 将系统函数 `SYS_ResetWaveforms_v4_50` 的地址加载到 `sysFn` 寄存器。
*   **`30 | LDI(0);SYS(50)`**: 将 `0` 加载到 `ACL`，然后调用系统函数 `SYS(50)`，这会重置所有波形。
*   **`31 | _MOVIW('SYS_ShuffleNoise_v4_46','sysFn')`**: 将系统函数 `SYS_ShuffleNoise_v4_46` 的地址加载到 `sysFn` 寄存器。
*   **`32 | LDI(0);SYS(46);SYS(46);SYS(46);SYS(46)`**: 将 `0` 加载到 `ACL`，然后连续调用四次系统函数 `SYS(46)`，这会多次打乱噪声发生器，以获得更好的随机性。
*   **`33 | RET()`**: 返回。
*   **`35-37 | module(...)`**: 定义模块，导出 `sound_reset_waveforms`。

#### `code_sinwave()` 和 `code_sindata()` - 正弦波形生成

这部分代码定义了一个正弦波形，并提供了生成该波形的函数。

```assembly
39 |     def code_sinwave():
40 |         nohop()
41 |         label('sound_sine_waveform')
42 |         LD(R8);ANDI(3);STW(R8)
43 |         LDI(v('soundTable')>>8);ST(R10+1);ST(R11+1)
44 |         LDI(0x1f);STW(R9)
45 |         label('.loop')
46 |         LSLW();LSLW();ORW(R8);ST(R10);ORI(128);ST(R11)
47 |         LD(R9);SUBI(16);_BGE('.x0')
48 |         LDWI('.data');ADDW(R9);_BRA('.x1')
49 |         label('.x0')
50 |         LDWI(v('.data')+31);SUBW(R9)
51 |         label('.x1')
52 |         PEEK();POKE(R10);XORI(63);POKE(R11)
53 |         LD(R9);SUBI(1);ST(R9);_BGE('.loop')
54 |         RET()
55 |
56 |     def code_sindata():
57 |         label('.data')
58 |         #  int(32.5+31*math.sin((i+0.5)*math.pi/32)) for i in range(16) ]
59 |         bytes(34, 37, 40, 42, 45, 48, 50, 53,
60 |               55, 57, 59, 60, 61, 62, 63, 63)
61 |
62 |     module(name='sound_sinwave.s',
63 |            code=[('EXPORT','sound_sine_waveform'),
64 |                  ('CODE','sound_sine_waveform',code_sinwave),
65 |                  ('DATA','sound_sine_wavedata',code_sindata,16,1)] )
```

*   **`39 | def code_sinwave():`**: 定义 Python 函数 `code_sinwave`。
*   **`41 | label('sound_sine_waveform')`**: 定义汇编标签 `sound_sine_waveform`。
*   **`42 | LD(R8);ANDI(3);STW(R8)`**: 将 `R8` 的低两位作为波形索引，存储回 `R8`。
*   **`43 | LDI(v('soundTable')>>8);ST(R10+1);ST(R11+1)`**: 将 `soundTable` 的高字节加载到 `R10+1` 和 `R11+1`，用于构建波形数据地址。
*   **`44 | LDI(0x1f);STW(R9)`**: 将 `0x1f` (31) 加载到 `R9`，作为循环计数器。
*   **`45 | label('.loop')`**: 循环开始标签。
*   **`46 | LSLW();LSLW();ORW(R8);ST(R10);ORI(128);ST(R11)`**: 这行代码计算波形数据的地址。`LSLW()` 左移两次，然后与 `R8` (波形索引) 进行或操作，结果存储在 `R10`。`ORI(128)` 设置 `R11` 的高位，可能用于某种波形属性。
*   **`47 | LD(R9);SUBI(16);_BGE('.x0')`**: 检查 `R9` 是否大于等于 16。
*   **`48 | LDWI('.data');ADDW(R9);_BRA('.x1')`**: 如果 `R9 < 16`，则计算 `.data` 加上 `R9` 的地址。
*   **`49 | label('.x0')`**: 标签 `.x0`。
*   **`50 | LDWI(v('.data')+31);SUBW(R9)`**: 如果 `R9 >= 16`，则计算 `.data` 加上 31 减去 `R9` 的地址，这可能是为了对称地访问正弦波数据。
*   **`51 | label('.x1')`**: 标签 `.x1`。
*   **`52 | PEEK();POKE(R10);XORI(63);POKE(R11)`**: 从计算出的地址读取数据，写入 `R10` 指向的地址，然后异或 `63` (可能用于反转波形或调整幅度)，再写入 `R11` 指向的地址。
*   **`53 | LD(R9);SUBI(1);ST(R9);_BGE('.loop')`**: `R9` 减 1，如果 `R9` 仍然大于等于 0，则继续循环。
*   **`54 | RET()`**: 返回。
*   **`56 | def code_sindata():`**: 定义 Python 函数 `code_sindata`，用于生成正弦波数据。
*   **`57 | label('.data')`**: 定义数据标签 `.data`。
*   **`58 | # int(32.5+31*math.sin((i+0.5)*math.pi/32)) for i in range(16) ]`**: 注释说明了正弦波数据的计算公式。
*   **`59-60 | bytes(...)`**: 实际的正弦波数据，16 个字节。
*   **`62-65 | module(...)`**: 定义模块，导出 `sound_sine_waveform`，并将 `code_sinwave` 和 `code_sindata` 关联到相应的标签和数据。

#### `code_freq()` - 设置频率

这个函数用于将给定的频率值转换为 Gigatron 声音硬件所需的频率键。

```assembly
67 |     def code_freq():
68 |         nohop()
69 |         label('sound_freq')
70 |         # The frequency key is obtained by multiplying the frequency
71 |         # in Hz by 4.1943 and converting in 7.7 fixed point encoding.
72 |         # Approximation: key = freq * 0x432 / 256.
73 |         LD(R8);ST(R8+1);_MOVIB(0xfc,R8)
74 |         label('_sound_freq_sub')
75 |         _MOVIW('SYS_LSRW4_50','sysFn')
76 |         LDW(R9);LSLW();STW(T0);LSLW();STW(T1)
77 |         SUBW(R9);SYS(50)
78 |         if args.cpu >= 7:
79 |             ADDV(T1);LD(T0+1);ADDV(T1)
80 |             LDW(T1);ADDV(T1);ANDI(0x7f);ST(T1)
81 |         else:
82 |             ADDW(T1);STW(T1);LD(T0+1);ADDW(T1);
83 |             ST(T0);LSLW();STW(T1);LD(T0);ANDI(0x7f);ST(T1)
84 |         LDW(T1);DOKE(R8);RET()
85 |         RET()
86 |
87 |     module(name='sound_freq.s',
88 |            code=[('EXPORT','sound_freq'),
89 |                  ('EXPORT','_sound_freq_sub'),
90 |                  ('CODE','sound_freq',code_freq)] )
```

*   **`67 | def code_freq():`**: 定义 Python 函数 `code_freq`。
*   **`69 | label('sound_freq')`**: 定义汇编标签 `sound_freq`。
*   **`70-72 | # The frequency key is obtained...`**: 注释说明了频率键的计算方法。
*   **`73 | LD(R8);ST(R8+1);_MOVIB(0xfc,R8)`**: 将 `R8` 的值存储到 `R8+1`，然后将 `0xfc` (声音频率寄存器地址) 移动到 `R8`。
*   **`74 | label('_sound_freq_sub')`**: 定义内部标签 `_sound_freq_sub`，这是频率计算的子程序入口。
*   **`75 | _MOVIW('SYS_LSRW4_50','sysFn')`**: 将系统函数 `SYS_LSRW4_50` (逻辑右移 4 位) 的地址加载到 `sysFn`。
*   **`76 | LDW(R9);LSLW();STW(T0);LSLW();STW(T1)`**: 将 `R9` 的值加载到 `ACL`，左移两次，存储到 `T0`，再左移两次，存储到 `T1`。这可能是为了进行乘法运算。
*   **`77 | SUBW(R9);SYS(50)`**: 从 `ACL` 减去 `R9`，然后调用 `SYS(50)` (逻辑右移 4 位)。
*   **`78-83 | if args.cpu >= 7: ... else: ...`**: 根据 CPU 版本选择不同的代码路径。这部分代码执行复杂的定点乘法和移位操作，将输入频率转换为 Gigatron 硬件所需的格式。
*   **`84 | LDW(T1);DOKE(R8);RET()`**: 将计算出的频率键从 `T1` 加载到 `ACL`，写入 `R8` 指向的地址 (声音频率寄存器)，然后返回。
*   **`85 | RET()`**: 额外的返回指令，可能是为了兼容性或错误处理。
*   **`87-90 | module(...)`**: 定义模块，导出 `sound_freq` 和 `_sound_freq_sub`。

#### `code_sound_on()` - 打开声音

这个函数用于打开一个声道的声音，并设置其音量和波形。

```assembly
92 |     def code_sound_on():
93 |         nohop()
94 |         label('sound_on')
95 |         LD(R8);ST(R8+1);_MOVIB(0xfa,R8)
96 |         LDI(127);SUBW(R10);POKE(R8);INC(R8)
97 |         LD(R11);ANDI(3);POKE(R8);INC(R8)
98 |         if args.cpu >= 6:
99 |             JGE('_sound_freq_sub')
100 |         else:
101 |             PUSH();_CALLJ('_sound_freq_sub');POP();RET()
102 |
103 |     module(name='sound_on.s',
104 |            code=[('EXPORT','sound_on'),
105 |                  ('IMPORT','_sound_freq_sub'),
106 |                  ('CODE','sound_on',code_sound_on)] )
```

*   **`92 | def code_sound_on():`**: 定义 Python 函数 `code_sound_on`。
*   **`94 | label('sound_on')`**: 定义汇编标签 `sound_on`。
*   **`95 | LD(R8);ST(R8+1);_MOVIB(0xfa,R8)`**: 将 `R8` 的值存储到 `R8+1`，然后将 `0xfa` (声音音量/波形寄存器地址) 移动到 `R8`。
*   **`96 | LDI(127);SUBW(R10);POKE(R8);INC(R8)`**: 将 `127` 减去 `R10` (音量参数)，结果写入 `R8` 指向的地址，然后 `R8` 递增。
*   **`97 | LD(R11);ANDI(3);POKE(R8);INC(R8)`**: 将 `R11` (波形参数) 与 `3` 进行与操作 (取低两位)，结果写入 `R8` 指向的地址，然后 `R8` 递增。
*   **`98-101 | if args.cpu >= 6: ... else: ...`**: 根据 CPU 版本，跳转或调用 `_sound_freq_sub` 子程序来设置频率。
*   **`103-106 | module(...)`**: 定义模块，导出 `sound_on` 并导入 `_sound_freq_sub`。

#### `code_note()` - 设置音符

这个函数用于将 MIDI 音符值转换为 Gigatron 声音硬件所需的频率键。

```assembly
108 |     def code_note():
109 |         nohop()
110 |         label('sound_note')
111 |         LD(R8);ST(R8+1);_MOVIB(0xfc,R8)
112 |         label('_sound_note_sub')
113 |         LDWI(v('notesTable')-22);ADDW(R9);ADDW(R9);STW(T0)
114 |         LUP(0);ST(R9);LDW(T0);LUP(1);ST(R9+1)
115 |         LDW(R9);DOKE(R8);RET()
116 |
117 |     module(name='sound_note.s',
118 |            code=[('EXPORT','sound_note'),
119 |                  ('EXPORT','_sound_note_sub'),
120 |                  ('CODE','sound_note',code_note)] )
```

*   **`108 | def code_note():`**: 定义 Python 函数 `code_note`。
*   **`110 | label('sound_note')`**: 定义汇编标签 `sound_note`。
*   **`111 | LD(R8);ST(R8+1);_MOVIB(0xfc,R8)`**: 将 `R8` 的值存储到 `R8+1`，然后将 `0xfc` (声音频率寄存器地址) 移动到 `R8`。
*   **`112 | label('_sound_note_sub')`**: 定义内部标签 `_sound_note_sub`，这是音符转换的子程序入口。
*   **`113 | LDWI(v('notesTable')-22);ADDW(R9);ADDW(R9);STW(T0)`**: 计算 `notesTable` 中对应音符的地址。`R9` 包含 MIDI 音符值，`ADDW(R9);ADDW(R9)` 相当于 `R9 * 2`，因为 `notesTable` 存储的是字 (word) 类型数据。
*   **`114 | LUP(0);ST(R9);LDW(T0);LUP(1);ST(R9+1)`**: 从计算出的地址加载音符频率值到 `R9` 和 `R9+1`。
*   **`115 | LDW(R9);DOKE(R8);RET()`**: 将音符频率值从 `R9` 加载到 `ACL`，写入 `R8` 指向的地址 (声音频率寄存器)，然后返回。
*   **`117-120 | module(...)`**: 定义模块，导出 `sound_note` 和 `_sound_note_sub`。

#### `code_note_on()` - 打开音符并设置音量和波形

这个函数结合了 `sound_on` 和 `_sound_note_sub` 的功能，用于打开一个音符。

```assembly
122 |     def code_note_on():
123 |         nohop()
124 |         label('note_on')
125 |         LD(R8);ST(R8+1);_MOVIB(0xfa,R8)
126 |         LDI(127);SUBW(R10);POKE(R8);INC(R8)
127 |         LD(R11);ANDI(3);POKE(R8);INC(R8)
128 |         if args.cpu >= 6:
129 |             JGE('_sound_note_sub')
130 |         else:
131 |             PUSH();_CALLJ('_sound_note_sub');POP();RET()
132 |
133 |     module(name='note_on.s',
133 |            code=[('EXPORT','note_on'),
134 |                  ('IMPORT','_sound_note_sub'),
135 |                  ('CODE','note_on',code_note_on)] )
```

*   **`122 | def code_note_on():`**: 定义 Python 函数 `code_note_on`。
*   **`124 | label('note_on')`**: 定义汇编标签 `note_on`。
*   **`125 | LD(R8);ST(R8+1);_MOVIB(0xfa,R8)`**: 将 `R8` 的值存储到 `R8+1`，然后将 `0xfa` (声音音量/波形寄存器地址) 移动到 `R8`。
*   **`126 | LDI(127);SUBW(R10);POKE(R8);INC(R8)`**: 将 `127` 减去 `R10` (音量参数)，结果写入 `R8` 指向的地址，然后 `R8` 递增。
*   **`127 | LD(R11);ANDI(3);POKE(R8);INC(R8)`**: 将 `R11` (波形参数) 与 `3` 进行与操作 (取低两位)，结果写入 `R8` 指向的地址，然后 `R8` 递增。
*   **`128-131 | if args.cpu >= 6: ... else: ...`**: 根据 CPU 版本，跳转或调用 `_sound_note_sub` 子程序来设置音符频率。
*   **`133-136 | module(...)`**: 定义模块，导出 `note_on` 并导入 `_sound_note_sub`。

### 2. MIDI 播放相关功能

这部分代码在 `args.cpu < 5` 时提供一个占位符实现，在 `args.cpu >= 5` 时提供完整的 MIDI 播放功能，包括变量、中断处理和播放逻辑。

#### `if args.cpu < 5:` - 占位符 MIDI 播放

```assembly
139 |     # ----------------------------------------
140 |     # midi stuff
141 |
142 |     if args.cpu < 5:
143 |
144 |         def code_midi_play():
145 |             warning('midi_play() cannot work without vIRQ (needs rom>=v5a)', dedup=True)
146 |             nohop()
147 |             label('midi_playing')
148 |             label('midi_play')
149 |             label('midi_chain')
150 |             LDI(0);RET()
151 |
152 |         module(name='midi_play.s',
153 |                code=[('EXPORT','midi_play'),
154 |                      ('EXPORT','midi_playing'),
155 |                      ('EXPORT','midi_chain'),
156 |                      ('CODE','midi_play',code_midi_play)] )
```

*   **`142 | if args.cpu < 5:`**: 条件编译，如果 CPU 版本低于 5，则编译以下代码。
*   **`144 | def code_midi_play():`**: 定义 Python 函数 `code_midi_play`。
*   **`145 | warning(...)`**: 警告信息，说明 MIDI 播放需要 `rom>=v5a`。
*   **`147-149 | label(...)`**: 定义 `midi_playing`, `midi_play`, `midi_chain` 标签。
*   **`150 | LDI(0);RET()`**: 加载 `0` 并返回，表示这些函数在旧 CPU 版本下不执行任何操作。
*   **`152-156 | module(...)`**: 定义模块，导出这些标签。

#### `else:` - 完整 MIDI 播放功能 (`args.cpu >= 5`)

##### `code_midi_ivars()` - MIDI 播放器指针变量

```assembly
157 |     else:
158 |
159 |         def code_midi_ivars():
160 |             label('_midi.p')
161 |             words(0)
162 |             label('_midi.q')
163 |             words(0)
164 |
165 |         module(name='midi_ptrs.s',
166 |                code=[('EXPORT','_midi.q'),
167 |                      ('EXPORT','_midi.p'),
168 |                      ('DATA', 'midi_ptrs', code_midi_ivars, 4, 1),
169 |                      ('PLACE','midi_ptrs', 0x0000, 0x00ff)] )
```

*   **`157 | else:`**: 如果 CPU 版本大于等于 5，则编译以下代码。
*   **`159 | def code_midi_ivars():`**: 定义 Python 函数 `code_midi_ivars`。
*   **`160 | label('_midi.p')`**: 定义标签 `_midi.p`，用于存储 MIDI 数据指针。
*   **`161 | words(0)`**: 分配一个字 (2 字节) 空间，并初始化为 `0`。
*   **`162 | label('_midi.q')`**: 定义标签 `_midi.q`，用于存储 MIDI 队列指针。
*   **`163 | words(0)`**: 分配一个字 (2 字节) 空间，并初始化为 `0`。
*   **`165-169 | module(...)`**: 定义模块，导出 `_midi.q` 和 `_midi.p`，并将这些变量放置在 `0x0000` 到 `0x00ff` 的内存区域。

##### `code_midi_tvars()` - MIDI 临时变量

```assembly
170 |         def code_midi_tvars():
171 |             label('_midi.t')
172 |             space(2)
173 |             label('_midi.tmp')
174 |             space(2)
175 |             label('_midi.cmd')
176 |             space(2)
```

*   **`170 | def code_midi_tvars():`**: 定义 Python 函数 `code_midi_tvars`。
*   **`171 | label('_midi.t')`**: 定义标签 `_midi.t`，用于存储 MIDI 计时器。
*   **`172 | space(2)`**: 分配 2 字节空间。
*   **`173 | label('_midi.tmp')`**: 定义标签 `_midi.tmp`，用于存储 MIDI 临时数据。
*   **`174 | space(2)`**: 分配 2 字节空间。
*   **`175 | label('_midi.cmd')`**: 定义标签 `_midi.cmd`，用于存储 MIDI 命令。
*   **`176 | space(2)`**: 分配 2 字节空间。

##### `code_midi_note()` - MIDI 音符处理

这个函数处理 MIDI 音符事件，包括音符开/关和频率设置。

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

*   **`180 | label('.midi_note')`**: 定义内部标签 `.midi_note`。
*   **`181 | ADDI(0x10);STW(vLR)`**: 将 `0x10` 加到 `ACL`，然后存储到 `vLR` (返回地址寄存器)。这可能是在调整栈帧或参数。
*   **`182 | LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')`**: 从 `_midi.p` 指向的地址读取一个字，`_midi.p` 递增，然后将读取的值存储到 `_midi.cmd`。
*   **`183 | LDW(vLR);_BLT('.freq')`**: 比较 `vLR` 的值，如果小于某个值，则跳转到 `.freq`。
*   **`184 | LDI(0xfa);ST('_midi.tmp')`**: 将 `0xfa` (音量/波形寄存器地址) 存储到 `_midi.tmp`。
*   **`185 | LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')`**: 从 `_midi.p` 读取音量/波形数据，`_midi.p` 递增，然后写入 `_midi.tmp` 指向的寄存器。
*   **`186 | label('.freq')`**: 标签 `.freq`。
*   **`187 | LDI(0xfc);ST('_midi.tmp')`**: 将 `0xfc` (频率寄存器地址) 存储到 `_midi.tmp`。
*   **`188 | LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')`**: 计算 `notesTable` 中音符频率的地址，存储到 `_midi.cmd`。
*   **`189 | LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)`**: 从 `_midi.cmd` 指向的地址加载频率值到 `vLR`。
*   **`190 | LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')`**: 将频率值写入 `_midi.tmp` 指向的寄存器，然后调用 `.getcmd`。

##### `code_midi_tick()` - MIDI 节拍处理

这个函数是 MIDI 播放的核心，它在每个节拍处理 MIDI 命令。

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

*   **`194 | label('.midi_tick')`**: 定义内部标签 `.midi_tick`。
*   **`195 | PUSH()`**: 将当前寄存器推入栈中。
*   **`197 | label('.getcmd')`**: 获取 MIDI 命令的标签。
*   **`198 | LDW('_midi.p');PEEK();_BNE('.docmd')`**: 从 `_midi.p` 指向的地址读取数据，如果非零则跳转到 `.docmd`。
*   **`199 | LDW('_midi.q');DEEK();_BEQ('.fin')`**: 从 `_midi.q` 指向的地址读取数据，如果为零则跳转到 `.fin`。
*   **`200 | STW('_midi.p');INC('_midi.q');INC('_midi.q');_BRA('.getcmd')`**: 将 `_midi.p` 的值存储到 `_midi.q`，`_midi.q` 递增两次，然后跳转到 `.getcmd`。这可能是在处理 MIDI 链表。
*   **`202 | label('.docmd')`**: 处理 MIDI 命令的标签。
*   **`203 | INC('_midi.p');STW('_midi.cmd')`**: `_midi.p` 递增，然后将 `ACL` 存储到 `_midi.cmd`。
*   **`204 | ANDI(3);INC(vACL);ST(v('_midi.tmp')+1)`**: 对 `ACL` 进行位操作，然后存储到 `_midi.tmp+1`。
*   **`206 | LDW('_midi.cmd');SUBI(0x80);_BGE('.xcmd')`**: 从 `_midi.cmd` 减去 `0x80`，如果结果大于等于 0，则跳转到 `.xcmd` (音符关闭)。
*   **`207-210 | if args.cpu >= 7: ... else: ...`**: 根据 CPU 版本，将 `_midi.cmd` 的值加到 `_midi.t` (MIDI 计时器)，实现延迟。
*   **`211 | POP();RET()`**: 恢复寄存器并返回。
*   **`213 | label('.xcmd')`**: 音符关闭处理标签。
*   **`214 | SUBI(0x10);_BGE('.ncmd')`**: 从 `ACL` 减去 `0x10`，如果结果大于等于 0，则跳转到 `.ncmd` (音符打开)。
*   **`215 | LDI(0xfc);ST('_midi.tmp')`**: 将 `0xfc` (频率寄存器地址) 存储到 `_midi.tmp`。
*   **`216 | LDI(0);DOKE('_midi.tmp');_BRA('.getcmd')`**: 将 `0` 写入频率寄存器 (关闭音符)，然后跳转到 `.getcmd`。
*   **`218 | label('.ncmd')`**: 音符打开处理标签。
*   **`219 | SUBI(0x20)`**: 从 `ACL` 减去 `0x20`。
*   **`220-223 | if args.cpu >= 6: ... else: ...`**: 根据 CPU 版本，跳转或调用 `.midi_note` 来处理音符打开事件。
*   **`225 | label('.fin')`**: 结束标签。
*   **`226 | POP();POP()`**: 恢复两个寄存器。
*   **`227 | LDI(0);ST('soundTimer');STW('_midi.q')`**: 将 `soundTimer` 和 `_midi.q` 清零。
*   **`228 | label('.ret')`**: 返回标签。
*   **`229 | RET()`**: 返回。

##### `code_midi_irq()` - MIDI 中断处理

这个函数是 Gigatron 虚拟中断 (vIRQ) 的替代处理程序，用于在中断中处理 MIDI 节拍。

```assembly
231 |         def code_midi_irq():
232 |             nohop()
233 |             label('_vIrqAltHandler')
234 |             LDW('_midi.q');_BEQ('.rti0')
235 |             PUSH()
236 |             LDI(255);ST('soundTimer')
237 |             _BRA('.irq1')
238 |             label('.irq0')
239 |             CALLI('.midi_tick')
240 |             CALLI('_vBlnAvoid')
241 |             label('.irq1')
242 |             LD('frameCount');ADDW('_vIrqTicks');SUBW('_midi.t');_BGE('.irq0')
243 |             label('.rti')
244 |             POP()
245 |             ST('frameCount')
246 |             LDW('_midi.t');ST('_vIrqTicks')
247 |             XORW('_vIrqTicks');_BNE('.rti0') # return to carry in virqticks
248 |             POP();LDWI(0x400);LUP(0)         # no carry
249 |             label('.rti0')
250 |             RET()
```

*   **`233 | label('_vIrqAltHandler')`**: 定义汇编标签 `_vIrqAltHandler`，这是虚拟中断的替代处理程序入口。
*   **`234 | LDW('_midi.q');_BEQ('.rti0')`**: 如果 `_midi.q` 为零，则跳转到 `.rti0` (不处理 MIDI)。
*   **`235 | PUSH()`**: 将当前寄存器推入栈中。
*   **`236 | LDI(255);ST('soundTimer')`**: 将 `255` 存储到 `soundTimer`。
*   **`237 | _BRA('.irq1')`**: 跳转到 `.irq1`。
*   **`238 | label('.irq0')`**: 标签 `.irq0`。
*   **`239 | CALLI('.midi_tick')`**: 调用 `.midi_tick` 处理 MIDI 节拍。
*   **`240 | CALLI('_vBlnAvoid')`**: 调用 `_vBlnAvoid` (可能用于避免屏幕闪烁)。
*   **`241 | label('.irq1')`**: 标签 `.irq1`。
*   **`242 | LD('frameCount');ADDW('_vIrqTicks');SUBW('_midi.t');_BGE('.irq0')`**: 检查是否达到下一个 MIDI 节拍的时间，如果达到则跳转到 `.irq0`。
*   **`243 | label('.rti')`**: 返回中断标签。
*   **`244 | POP()`**: 恢复寄存器。
*   **`245 | ST('frameCount')`**: 存储 `frameCount`。
*   **`246 | LDW('_midi.t');ST('_vIrqTicks')`**: 将 `_midi.t` 存储到 `_vIrqTicks`。
*   **`247 | XORW('_vIrqTicks');_BNE('.rti0')`**: 异或 `_vIrqTicks`，如果非零则跳转到 `.rti0`。
*   **`248 | POP();LDWI(0x400);LUP(0)`**: 恢复寄存器，加载 `0x400`。
*   **`249 | label('.rti0')`**: 标签 `.rti0`。
*   **`250 | RET()`**: 返回。

##### `code_midi_play()` - 启动 MIDI 播放

这个函数用于启动 MIDI 序列的播放。

```assembly
252 |         def code_midi_play():
253 |             nohop()
254 |             label('midi_play')
255 |             PUSH()
256 |             LDI(0);STW('_midi.q');STW('_midi.p')
257 |             CALLI('sound_all_off')
258 |             LDW(R8);BEQ('.play3')
259 |             # arrange speedy start
260 |             CALLI('_vBlnAvoid')
261 |             LD('frameCount');INC(vACL);_BEQ('.play1')
262 |             CALLI('_clock.sub')
263 |             STW('_vIrqTicks')
264 |             LDW(LAC+2);STW(v('_vIrqTicks')+2)
265 |             # set interrupt
266 |             label('.play1')
267 |             LDW(R8);STW('_midi.q')
268 |             LDI(0xff);ST('frameCount')
269 |             # wait for first play
270 |             label('.play2')
271 |             LDW('_midi.p');_BEQ('.play2')
272 |             # done
273 |             label('.play3')
274 |             POP();RET()
275 |
276 |         module(name='midi_play.s',
277 |                code=[('EXPORT','midi_play'),
278 |                      ('EXPORT','_vIrqAltHandler'),
279 |                      ('IMPORT','_midi.p'),
280 |                      ('IMPORT','_midi.q'),
281 |                      ('IMPORT','sound_all_off'),
282 |                      ('IMPORT','_vIrqTicks'),
283 |                      ('IMPORT','_vBlnAvoid'),
284 |                      ('IMPORT','_clock.sub'),
285 |                      ('BSS',   'midi_tvars', code_midi_tvars, 6, 1),
286 |                      ('PLACE', 'midi_tvars', 0x0000, 0x00ff),
287 |                      ('CODE',  'midi_note', code_midi_note),
288 |                      ('PLACE', 'midi_note', 0x0100, 0x7fff),
289 |                      ('CODE',  'midi_tick', code_midi_tick),
290 |                      ('PLACE', 'midi_tick', 0x0100, 0x7fff),
291 |                      ('CODE',  '_vIrqAltHandler', code_midi_irq),
292 |                      ('PLACE', '_vIrqAltHandler', 0x0100, 0x7fff),
293 |                      ('CODE',  'midi.play', code_midi_play) ] )
```

*   **`254 | label('midi_play')`**: 定义汇编标签 `midi_play`。
*   **`255 | PUSH()`**: 将当前寄存器推入栈中。
*   **`256 | LDI(0);STW('_midi.q');STW('_midi.p')`**: 将 `_midi.q` 和 `_midi.p` 清零。
*   **`257 | CALLI('sound_all_off')`**: 调用 `sound_all_off` 关闭所有声音。
*   **`258 | LDW(R8);BEQ('.play3')`**: 如果 `R8` 为零，则跳转到 `.play3` (不播放 MIDI)。
*   **`260 | CALLI('_vBlnAvoid')`**: 调用 `_vBlnAvoid`。
*   **`261 | LD('frameCount');INC(vACL);_BEQ('.play1')`**: 检查 `frameCount`，如果为零则跳转到 `.play1`。
*   **`262 | CALLI('_clock.sub')`**: 调用 `_clock.sub`。
*   **`263 | STW('_vIrqTicks')`**: 存储 `_vIrqTicks`。
*   **`264 | LDW(LAC+2);STW(v('_vIrqTicks')+2)`**: 存储 `_vIrqTicks` 的高字节。
*   **`266 | label('.play1')`**: 标签 `.play1`。
*   **`267 | LDW(R8);STW('_midi.q')`**: 将 `R8` (MIDI 数据地址) 存储到 `_midi.q`。
*   **`268 | LDI(0xff);ST('frameCount')`**: 将 `0xff` 存储到 `frameCount`。
*   **`270 | label('.play2')`**: 等待第一次播放的标签。
*   **`271 | LDW('_midi.p');_BEQ('.play2')`**: 如果 `_midi.p` 为零，则继续等待。
*   **`273 | label('.play3')`**: 播放完成标签。
*   **`274 | POP();RET()`**: 恢复寄存器并返回。
*   **`276-293 | module(...)`**: 定义模块，导出 `midi_play` 和 `_vIrqAltHandler`，导入相关变量和函数，并将 `midi_tvars`, `midi_note`, `midi_tick`, `_vIrqAltHandler` 的代码放置在指定的内存区域。

##### `code_midi_playing()` - 检查 MIDI 是否正在播放

```assembly
295 |         def code_midi_playing():
296 |             nohop()
297 |             label('midi_playing')
298 |             LDW('_midi.q');RET()
299 |
300 |         module(name='midi_playing.s',
301 |                code=[('EXPORT','midi_playing'),
302 |                      ('IMPORT','_midi.q'),
303 |                      ('CODE', 'midi_playing', code_midi_playing)] )
```

*   **`297 | label('midi_playing')`**: 定义汇编标签 `midi_playing`。
*   **`298 | LDW('_midi.q');RET()`**: 加载 `_midi.q` 的值并返回。如果 `_midi.q` 非零，表示 MIDI 正在播放。
*   **`300-303 | module(...)`**: 定义模块，导出 `midi_playing` 并导入 `_midi.q`。

##### `code_midi_chain()` - MIDI 链式播放

这个函数用于将多个 MIDI 序列链接起来播放。

```assembly
305 |         def code_midi_chain():
306 |             nohop()
307 |             label('midi_chain')
308 |             PUSH();CALLI('_vIrqAvoid');POP()
309 |             LDW('_midi.q');_BEQ('.ret0')
310 |             DEEK();_BNE('.ret0')
311 |             LDW(R8);STW('_midi.q')
312 |             RET()
313 |             label('.ret0')
314 |             LDI(0);RET()
315 |
316 |         module(name='midi_chain.s',
317 |                code=[('EXPORT','midi_chain'),
318 |                      ('IMPORT','_midi.q'),
319 |                      ('IMPORT','_vIrqAvoid'),
320 |                      ('CODE', 'midi_chain', code_midi_chain)] )
```

*   **`307 | label('midi_chain')`**: 定义汇编标签 `midi_chain`。
*   **`308 | PUSH();CALLI('_vIrqAvoid');POP()`**: 调用 `_vIrqAvoid`，然后恢复寄存器。
*   **`309 | LDW('_midi.q');_BEQ('.ret0')`**: 如果 `_midi.q` 为零，则跳转到 `.ret0`。
*   **`310 | DEEK();_BNE('.ret0')`**: 从 `_midi.q` 指向的地址读取数据，如果非零则跳转到 `.ret0`。
*   **`311 | LDW(R8);STW('_midi.q')`**: 将 `R8` (下一个 MIDI 序列的地址) 存储到 `_midi.q`。
*   **`312 | RET()`**: 返回。
*   **`313 | label('.ret0')`**: 返回标签。
*   **`314 | LDI(0);RET()`**: 加载 `0` 并返回。
*   **`316-320 | module(...)`**: 定义模块，导出 `midi_chain` 并导入 `_midi.q` 和 `_vIrqAvoid`。

### 3. 作用域结束和文件元数据

```assembly
323 | scope()
324 |
325 | # Local Variables:
326 | # mode: python
327 | # indent-tabs-mode: ()
328 | # End:
329 |
```

*   **`323 | scope()`**: 调用 `scope` 函数，执行其中定义的 Python 代码，从而生成汇编指令。
*   **`325-328 | # Local Variables: ... # End:`**: Emacs 编辑器的文件局部变量设置，指定文件模式为 Python，并且不使用制表符缩进。