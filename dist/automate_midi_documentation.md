# automate_midi.py 脚本详细文档

## 概述

`automate_midi.py` 脚本是一个自动化工具，用于简化 MIDI 文件到 Gigatron TTL 计算机可执行音乐文件（`.gt1`）的转换和编译过程。它通过修改 `music.c` 和 `Makefile` 文件，执行 MIDI 转换器、GBAS 到 C 转换器以及 `make` 命令，最终生成 `.gt1` 文件，并将所有相关生成文件整理到预定义的目录中。

## 功能

该脚本执行以下主要功能：

1.  **文件存在性检查**：在开始处理之前，验证所有必需的输入文件和工具（如 `midi_converter.exe`、`gbas_to_c.py`、`Makefile`、`music.c`）是否存在。
2.  **修改 `music.c`**：动态更新 `music.c` 文件，以包含对新 MIDI 文件生成的音乐数据的引用。
3.  **修改 `Makefile`**：动态更新 `Makefile` 文件，以指示编译过程使用新的 MIDI 文件生成的 C 代码，并生成特定名称的 `.gt1` 文件。
4.  **MIDI 转换**：使用 `midi_converter.exe` 将输入的 `.mid` 文件转换为 Gigatron GBAS 格式的 `.gbas` 文件。
5.  **GBAS 到 C 转换**：使用 `gbas_to_c.py` 脚本将 `.gbas` 文件转换为 C 语言源文件（`.gbas.c`）。
6.  **编译**：执行 `make clean && make` 命令，编译项目并生成最终的 `.gt1` 音乐文件。
7.  **文件重命名**：将编译生成的 `music.gt1` 文件重命名为 `music-{filename_base}.gt1`，其中 `{filename_base}` 是原始 MIDI 文件的基本名称。此步骤包含一个等待机制，以确保在重命名之前 `music.gt1` 文件已完全生成。
8.  **文件整理**：将所有生成的和使用的文件移动到预定义的分类目录中，以保持项目目录的整洁。

## 使用方法

```bash
python automate_midi.py -mid <MIDI_FILE_PATH> [-time <TIME_PARAMETER>]
```

### 参数

*   `-mid <MIDI_FILE_PATH>`：
    *   **必需**。指定要处理的 MIDI 文件的路径（例如：`Moonlight2.mid`）。
*   `-time <TIME_PARAMETER>`：
    *   **可选**。为 `midi_converter.exe` 提供一个时间参数。

### 示例

```bash
python automate_midi.py -mid Moonlight2.mid -time 100
```

## 脚本工作流程

1.  **初始化**：
    *   解析命令行参数，获取 MIDI 文件名和可选的时间参数。
    *   从 MIDI 文件名中提取基本名称（不带扩展名）。
    *   定义生成的 `.gbas.c` 文件的名称。
2.  **环境检查**：
    *   检查 `midi_converter.exe`、`gbas_to_c.py`、`Makefile` 和 `music.c` 等关键文件是否存在于当前目录。
    *   如果任何必需文件缺失，脚本将报错并退出。
3.  **修改 `music.c`**：
    *   读取 `music.c` 的内容。
    *   修改第 18 行，将其更新为 `extern const byte* {filename_base}[];`。
    *   修改第 36 行，将其更新为 `  { {filename_base}, "{filename_base}" },`。
    *   将修改后的内容写回 `music.c`。
4.  **修改 `Makefile`**：
    *   读取 `Makefile` 的内容。
    *   修改第 11 行，将其更新为 `MIDIS={filename_base}.gbas.c`。
    *   修改第 9 行，将其更新为 `PGMS=music.gt1`。
    *   将修改后的内容写回 `Makefile`。
5.  **执行转换和编译**：
    *   **MIDI 转换**：执行 `midi_converter.exe {filename_mid} {filename_base}.gbas -d {time_param} -config midi_config.ini`。
    *   **GBAS 到 C 转换**：执行 `python gbas_to_c.py {filename_base}.gbas`。
    *   **编译**：执行 `make clean && make`。
6.  **重命名 `.gt1` 文件**：
    *   定义旧名称 `music.gt1` 和新名称 `music-{filename_base}.gt1`。
    *   进入一个循环，等待 `music.gt1` 文件生成（最长等待 30 秒）。
    *   如果新名称的文件已存在，则先删除它。
    *   将 `music.gt1` 重命名为 `music-{filename_base}.gt1`。
7.  **文件整理**：
    *   创建（如果不存在）以下目录：`music_midi`、`music_gt1`、`music_data_c`、`music_data_gbas`。
    *   将原始的 `.mid` 文件移动到 `music_midi`。
    *   将重命名后的 `.gt1` 文件移动到 `music_gt1`。
    *   将生成的 `.gbas.c` 文件移动到 `music_data_c`。
    *   将生成的 `.gbas` 文件移动到 `music_data_gbas`。
    *   在移动文件之前，如果目标位置已存在同名文件，则先删除目标文件。
8.  **完成**：打印成功消息。

## 依赖

*   Python 3.x
*   `midi_converter.exe` (MIDI 转换工具)
*   `gbas_to_c.py` (GBAS 到 C 转换脚本)
*   `Makefile` (项目编译文件)
*   `music.c` (主音乐源文件)
*   `shutil` 模块 (Python 标准库，用于文件移动操作)

## 注意事项

*   确保所有依赖文件（`midi_converter.exe`、`gbas_to_c.py`、`Makefile`、`music.c`）都位于脚本执行的当前目录。
*   脚本会修改 `music.c` 和 `Makefile` 文件。在运行脚本之前，请确保已备份这些文件或使用版本控制。
*   脚本会在目标目录中创建 `music_midi`、`music_gt1`、`music_data_c` 和 `music_data_gbas` 文件夹。
*   如果目标文件夹中已存在同名文件，脚本会先删除旧文件再移动新文件。