# `music\bwv846f.gbas.c` 音乐数组和 `music\sound.s` 声音驱动程序解析分析

## 简介
本文档旨在分析 `music\bwv846f.gbas.c` 文件中定义的音乐数据结构，并解释 `music\sound.s` 声音驱动程序如何解析和使用这些数据来生成音乐。

## 音乐数据结构 (`music\bwv846f.gbas.c`)

`music\bwv846f.gbas.c` 文件由 `gbas_to_c.py` 工具从 `bwv846f.gbas` 文件生成。它定义了一系列静态字节数组，这些数组包含了音乐的音符和控制数据。

### 宏定义
文件顶部定义了几个宏，用于简化音乐数据的编码：
- [`D(x)`](music/bwv846f.gbas.c:7): `x` 代表等待的帧数。
- [`X(c)`](music/bwv846f.gbas.c:8): `127+(c)`，表示通道 `c` 的音符关闭。
- [`N(c,n)`](music/bwv846f.gbas.c:9): `143+(c),(n)`，表示通道 `c` 开启，音符为 `n`。
- [`M(c,n,v)`](music/bwv846f.gbas.c:10): `159+(c),(n),(v)`，表示通道 `c` 开启，音符为 `n`，波形A为 `v`。
- [`W(c,n,v,w)`](music/bwv846f.gbas.c:11): `175+(c),(n),(v),(w)`，表示通道 `c` 开启，音符为 `n`，波形A为 `v`，波形X为 `w`。

这些宏将音乐事件编码为一系列字节值，其中第一个字节通常包含命令类型和通道信息，后续字节包含音符、波形等参数。

### 音乐数据数组
音乐数据被分割成多个 `nohop static const byte bwv846fXXX[]` 数组，例如 [`bwv846f000`](music/bwv846f.gbas.c:15), [`bwv846f001`](music/bwv846f.gbas.c:30) 等。每个数组都以 `0` 结尾，表示该段音乐数据的结束。

最后，[`nohop const byte *bwv846f[]`](music/bwv846f.gbas.c:286) 是一个指向这些音乐数据段的指针数组，也以 `0` 结尾，表示整个音乐的结束。这允许声音驱动程序按顺序播放这些音乐段。

**示例:**
[`D(12),W(1,60,64,3)`](music/bwv846f.gbas.c:16)
- `D(12)`: 等待12帧。
- `W(1,60,64,3)`: 通道1开启，音符60，波形A为64，波形X为3。

### 关键源码逐行解析 (`music\bwv846f.gbas.c`)

```c
// music/bwv846f.gbas.c
  7 | #define D(x) x                     /* wait x frames */
  8 | #define X(c) 127+(c)               /* channel c off */
  9 | #define N(c,n) 143+(c),(n)         /* channel c on, note=n */
 10 | #define M(c,n,v) 159+(c),(n),(v)   /* channel c on, note=n, wavA=v */
 11 | #define W(c,n,v,w) 175+(c),(n),(v),(w)   /* channel c on, note=n, wavA=v ,wavX=w*/
```
- 这些宏定义了音乐事件的编码方式。
- `D(x)`: 编码为单个字节 `x`，表示延迟 `x` 帧。
- `X(c)`: 编码为 `127 + c`，其中 `c` 是通道号。这个值在 `0x80` (128) 到 `0x8F` (143) 之间，用于表示音符关闭。
- `N(c,n)`: 编码为 `143 + c` 和音符 `n`。`143 + c` 的值在 `0x90` (144) 到 `0x9F` (159) 之间。
- `M(c,n,v)`: 编码为 `159 + c`、音符 `n` 和波形A `v`。`159 + c` 的值在 `0xA0` (160) 到 `0xAF` (175) 之间。
- `W(c,n,v,w)`: 编码为 `175 + c`、音符 `n`、波形A `v` 和波形X `w`。`175 + c` 的值在 `0xB0` (176) 到 `0xBF` (191) 之间。

```c
// music/bwv846f.gbas.c
 15 | nohop static const byte bwv846f000[] = {
 16 |   D(12),W(1,60,64,3),D(6),W(1,60,67,1),D(6),W(1,60,69,1),D(6),W(1,60,72,1),D(6),W(1,60,74,1),
 17 |   D(6),W(1,60,77,1),D(4),W(2,62,64,3),D(2),W(1,60,78,1),D(4),W(2,62,67,1),D(2),W(1,60,79,1),
 ...
 27 |   0
 28 | };
```
- `nohop static const byte bwv846f000[]`: 定义了一个静态的字节数组，其中 `nohop` 是一个属性，可能用于优化代码生成。
- 数组内容是宏展开后的字节序列。例如，`D(12)` 展开为 `12`。`W(1,60,64,3)` 展开为 `175+1`, `60`, `64`, `3`，即 `176, 60, 64, 3`。
- 每个音乐数据段都以 `0` 字节结束，作为段的终止符。

```c
// music/bwv846f.gbas.c
286 | nohop const byte *bwv846f[] = {
287 |   bwv846f000,
288 |   bwv846f001,
...
306 |   0
307 | };
```
- `nohop const byte *bwv846f[]`: 这是一个指向所有音乐数据段的指针数组。
- 数组中的每个元素都是一个 `bwv846fXXX` 数组的地址。
- 整个指针数组也以 `0` 结束，表示音乐播放列表的结束。

## 声音驱动程序解析逻辑 (`music\sound.s`)

`music\sound.s` 文件包含了 Gigatron 声音驱动程序的汇编代码，它通过中断服务程序（IRQ）来解析和播放音乐数据。

### 关键变量
- `_midi.q`: 指向当前播放的音乐数据段（`bwv846f` 数组中的一个指针）。
- `_midi.p`: 指向当前音乐数据段中的当前字节。
- `_midi.t`: 用于存储延迟帧数。
- `_midi.cmd`: 存储当前解析的命令字节。

### `midi_play` 函数
[`midi_play`](music/sound.s:266) 函数用于启动音乐播放。它初始化 `_midi.q` 和 `_midi.p` 为0，关闭所有声音，然后将传入的音乐数据（`bwv846f` 数组的地址）存储到 `_midi.q` 中，并设置 IRQ 处理程序。

### `_vIrqAltHandler` (IRQ 处理程序)
[`_vIrqAltHandler`](music/sound.s:245) 是一个虚拟中断服务程序，它在每个帧周期被调用。
- 它检查 `_midi.q` 是否为0，如果为0则表示没有音乐播放。
- 如果 `soundTimer` 达到0，则调用 [`midi_tick`](music/sound.s:301) 函数来处理下一个音乐事件。
- `_midi.t` 用于管理帧延迟。

### `.midi_tick` 函数
[`midi_tick`](music/sound.s:204) 函数是解析音乐数据的核心。
1.  **获取命令**:
    - 它首先从 `_midi.p` 指向的地址读取一个字节作为命令。
    - 如果 `_midi.p` 指向的字节为0（当前音乐段结束），则检查 `_midi.q` 指向的下一个音乐段。如果还有下一个音乐段，则更新 `_midi.p` 和 `_midi.q`。
2.  **处理命令**:
    - **延迟 (`D(x)`)**: 如果命令字节小于 `0x80` (128)，则表示一个延迟命令。命令值直接加到 `_midi.t` 中。
    - **音符关闭 (`X(c)`)**: 如果命令字节在 `0x80` 到 `0x8F` 之间，表示音符关闭。它将 `0xfc` 写入 `_midi.tmp`，然后将 `0` 写入该地址，从而关闭指定通道的声音。
    - **音符开启 (`N(c,n)`, `M(c,n,v)`, `W(c,n,v,w)`)**: 如果命令字节大于等于 `0x90`，则表示音符开启命令。
        - 命令字节减去 `0x90` 得到实际的命令类型和通道信息。
        - 根据命令类型，从 `_midi.p` 读取额外的参数（音符、波形A、波形X）。
        - [`midi_note`](music/sound.s:299) 函数被调用来处理音符开启事件，它会根据音符查找频率，并设置相应的声音寄存器。
        - 特别地，对于 `W(c,n,v,w)` 事件，会额外读取波形X的值并写入 `0xfb` 寄存器。

### `.midi_note` 函数
[`midi_note`](music/sound.s:178) 函数负责将音符转换为频率并设置声音硬件。
- 它根据音符值从 `notesTable` 中查找对应的频率数据。
- 将频率数据写入 `0xfc` 寄存器，并根据需要设置波形A（`0xfa`）和波形X（`0xfb`）寄存器。

通过这种方式，`sound.s` 驱动程序能够逐字节解析 `bwv846f.gbas.c` 中编码的音乐数据，并在每个帧周期更新声音硬件，从而播放出音乐。

### 关键源码逐行解析 (`music\sound.s`)

```assembly
// music/sound.s
183 |             ADDI(0x10);STW(vLR)
184 |             LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')
185 |             LDW(vLR);_BLT('.freq')
186 |             LDI(0xfa);ST('_midi.tmp')
187 |             LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')
188 |
189 |             # Check if it's a W(c,n,v,w) event (vLR >= 0xBF)
190 |             LDW(vLR);_BLT('.freq')
191 |             LDI(0xfb);ST('_midi.tmp')
192 |
193 |             # Set _midi.tmp to 0xfb for waveform
194 |             LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')
```
- [`ADDI(0x10);STW(vLR)`](music/sound.s:183): 将 `0x10` 加到 `vLR` (Link Register) 中，这可能是为了调整返回地址或作为临时存储。
- [`LDW('_midi.p');PEEK();INC('_midi.p');STW('_midi.cmd')`](music/sound.s:184): 从 `_midi.p` 指向的地址读取一个字节（当前命令），然后 `_midi.p` 自增，并将读取的字节存储到 `_midi.cmd`。
- [`LDW(vLR);_BLT('.freq')`](music/sound.s:185): 检查 `vLR` 的值。如果小于 `.freq` 的地址，则跳转到 `.freq`。这可能用于区分不同类型的音符事件（例如 `N` 和 `M`）。
- [`LDI(0xfa);ST('_midi.tmp')`](music/sound.s:186): 将 `0xfa` (波形A寄存器地址) 存储到 `_midi.tmp`。
- [`LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')`](music/sound.s:187): 从 `_midi.p` 读取下一个字节（波形A的值），`_midi.p` 自增，然后将该值写入 `_midi.tmp` 指向的地址（即波形A寄存器）。
- [`LDW(vLR);_BLT('.freq')`](music/sound.s:190): 再次检查 `vLR`。如果小于 `.freq`，则跳转。这用于检查是否是 `W(c,n,v,w)` 事件。
- [`LDI(0xfb);ST('_midi.tmp')`](music/sound.s:191): 将 `0xfb` (波形X寄存器地址) 存储到 `_midi.tmp`。
- [`LDW('_midi.p');PEEK();INC('_midi.p');POKE('_midi.tmp')`](music/sound.s:194): 从 `_midi.p` 读取下一个字节（波形X的值），`_midi.p` 自增，然后将该值写入 `_midi.tmp` 指向的地址（即波形X寄存器）。

```assembly
// music/sound.s
198 |             label('.freq')
199 |             LDI(0xfc);ST('_midi.tmp')
200 |             LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')
201 |             LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)
202 |             LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')
```
- [`label('.freq')`](music/sound.s:198): 标签 `.freq`。
- [`LDI(0xfc);ST('_midi.tmp')`](music/sound.s:199): 将 `0xfc` (频率寄存器地址) 存储到 `_midi.tmp`。
- [`LDWI(v('notesTable')-22);ADDW('_midi.cmd');ADDW('_midi.cmd');STW('_midi.cmd')`](music/sound.s:200): 计算 `notesTable` 中音符对应的频率数据的地址。`_midi.cmd` 此时存储的是音符值。`ADDW('_midi.cmd');ADDW('_midi.cmd')` 相当于 `_midi.cmd * 2`，因为频率数据可能是16位的。
- [`LUP(0);ST(vLR);LDW('_midi.cmd');LUP(1);ST(vLR+1)`](music/sound.s:201): 从计算出的地址加载频率数据（可能是16位），并存储到 `vLR`。
- [`LDW(vLR);DOKE('_midi.tmp');_CALLJ('.getcmd')`](music/sound.s:202): 将 `vLR` 中的频率数据写入 `_midi.tmp` 指向的地址（即频率寄存器），然后跳转到 `.getcmd` 继续处理下一个命令。

```assembly
// music/sound.s
209 |             label('.getcmd')
210 |             LDW('_midi.p');PEEK();_BNE('.docmd')
211 |             LDW('_midi.q');DEEK();_BEQ('.fin')
212 |             STW('_midi.p');INC('_midi.q');INC('_midi.q');_BRA('.getcmd')
```
- [`label('.getcmd')`](music/sound.s:209): 标签 `.getcmd`。
- [`LDW('_midi.p');PEEK();_BNE('.docmd')`](music/sound.s:210): 从 `_midi.p` 指向的地址读取一个字节。如果该字节不为0，则跳转到 `.docmd` 处理命令。
- [`LDW('_midi.q');DEEK();_BEQ('.fin')`](music/sound.s:211): 如果读取的字节为0（当前音乐段结束），则从 `_midi.q` 指向的地址读取一个字（下一个音乐段的地址）。如果该字为0，则表示整个音乐结束，跳转到 `.fin`。
- [`STW('_midi.p');INC('_midi.q');INC('_midi.q');_BRA('.getcmd')`](music/sound.s:212): 如果有下一个音乐段，则将 `_midi.q` 指向的地址存储到 `_midi.p` (开始处理下一个音乐段)，然后 `_midi.q` 自增2（指向下一个音乐段的指针），然后跳转回 `.getcmd`。

```assembly
// music/sound.s
214 |             label('.docmd')
215 |             INC('_midi.p');STW('_midi.cmd')
216 |             ANDI(3);INC(vACL);ST(v('_midi.tmp')+1)
217 |             # delay
218 |             LDW('_midi.cmd');SUBI(0x80);_BGE('.xcmd')
219 |             if args.cpu >= 7:
220 |                 LD('_midi.cmd');ADDV('_midi.t')
221 |             else:
222 |                 LD('_midi.cmd');ADDW('_midi.t');STW('_midi.t')
223 |             POP();RET();
```
- [`label('.docmd')`](music/sound.s:214): 标签 `.docmd`。
- [`INC('_midi.p');STW('_midi.cmd')`](music/sound.s:215): `_midi.p` 自增（跳过已读取的命令字节），并将当前命令存储到 `_midi.cmd`。
- [`ANDI(3);INC(vACL);ST(v('_midi.tmp')+1)`](music/sound.s:216): 这部分代码可能用于处理通道信息，将通道号存储到某个寄存器或内存位置。
- [`LDW('_midi.cmd');SUBI(0x80);_BGE('.xcmd')`](music/sound.s:218): 将 `_midi.cmd` 的值减去 `0x80`。如果结果大于或等于0，则表示命令字节大于等于 `0x80`，跳转到 `.xcmd` (音符关闭或音符开启)。否则，这是一个延迟命令。
- [`LD('_midi.cmd');ADDV('_midi.t')` / `LD('_midi.cmd');ADDW('_midi.t');STW('_midi.t')`](music/sound.s:220-222): 将延迟值 (`_midi.cmd` 的值) 加到 `_midi.t` 中。
- [`POP();RET()`](music/sound.s:223): 弹出栈并返回。

```assembly
// music/sound.s
224 |             # note off
225 |             label('.xcmd')
226 |             SUBI(0x10);_BGE('.ncmd')
227 |             LDI(0xfc);ST('_midi.tmp')
228 |             LDI(0);DOKE('_midi.tmp');_BRA('.getcmd')
```
- [`label('.xcmd')`](music/sound.s:225): 标签 `.xcmd` (处理音符关闭)。
- [`SUBI(0x10);_BGE('.ncmd')`](music/sound.s:226): 将 `_midi.cmd` 的值减去 `0x10`。如果结果大于或等于0，则表示命令字节大于等于 `0x90`，跳转到 `.ncmd` (音符开启)。否则，这是一个音符关闭命令 (`0x80` 到 `0x8F`)。
- [`LDI(0xfc);ST('_midi.tmp')`](music/sound.s:227): 将 `0xfc` (频率寄存器地址) 存储到 `_midi.tmp`。
- [`LDI(0);DOKE('_midi.tmp');_BRA('.getcmd')`](music/sound.s:228): 将 `0` 写入 `_midi.tmp` 指向的地址（即频率寄存器），从而关闭声音。然后跳转回 `.getcmd`。

```assembly
// music/sound.s
229 |             # note on
230 |             label('.ncmd')
231 |             SUBI(0x23) # Adjusted to include W(c,n,v,w) events (0xAF-0xB2)
232 |             if args.cpu >= 6:
233 |                 JLT('.midi_note')
234 |             else:
235 |                 _BGE('.fin');CALLI('.midi_note')
```
- [`label('.ncmd')`](music/sound.s:230): 标签 `.ncmd` (处理音符开启)。
- [`SUBI(0x23)`](music/sound.s:231): 将 `_midi.cmd` 的值减去 `0x23` (35)。这可能是为了将命令值调整到 `midi_note` 函数可以处理的范围，或者用于区分不同类型的音符开启事件。
- [`JLT('.midi_note')` / `_BGE('.fin');CALLI('.midi_note')`](music/sound.s:233-235): 如果结果小于0，则跳转到 `.midi_note` 处理音符开启。否则，可能是一个未知的命令，跳转到 `.fin`。

```assembly
// music/sound.s
237 |             label('.fin')
238 |             POP();POP() # pop one more level
239 |             LDI(0);ST('soundTimer');STW('_midi.q')
240 |             label('.ret')
241 |             RET()
```
- [`label('.fin')`](music/sound.s:237): 标签 `.fin` (音乐结束)。
- [`POP();POP()`](music/sound.s:238): 弹出栈两次，恢复之前的状态。
- [`LDI(0);ST('soundTimer');STW('_midi.q')`](music/sound.s:239): 将 `soundTimer` 和 `_midi.q` 设置为0，停止音乐播放。
- [`RET()`](music/sound.s:241): 返回。
