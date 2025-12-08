#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

// 宏指令元素结构体，用于存储单个宏指令元素
struct MacroElement {
    std::string type;        // 元素类型：value, range, loop_start, loop_end, release
    int value;               // 数值（适用于value类型）
    int start_value;         // 范围开始值（适用于range类型）
    int end_value;           // 范围结束值（适用于range类型）
    int duration;            // 持续时间（tick数）
    std::vector<int> sequence; // 序列值（适用于wave等序列类型）
};

// 宏指令结构体，用于存储解析后的宏指令
struct MacroCommand {
    std::string type;        // 宏类型：vol, note, wave, pitch_bend
    std::vector<MacroElement> on_elements; // 音符开启阶段的宏指令元素列表
    std::vector<MacroElement> release_elements; // 音符释放阶段的宏指令元素列表
};

// 乐器配置结构体
struct InstrumentConfig {
    std::string name;        // 乐器名称
    int accuracy;            // 精度（tick时间，1/秒）
    std::vector<MacroCommand> macros; // 宏指令列表
};

// INI解析器类
class IniParser {
public:
    IniParser();
    ~IniParser();
    
    // 解析INI文件
    bool parse(const std::string& filename);
    
    // 获取默认精度
    int getDefaultAccuracy() const;
    
    // 获取乐器配置
    const InstrumentConfig& getInstrumentConfig(int instrument_id) const;
    
    // 获取鼓声配置
    const InstrumentConfig& getDrumConfig(int drum_id) const;
    
    // 检查乐器是否存在
    bool hasInstrument(int instrument_id) const;
    
    // 检查鼓声是否存在
    bool hasDrum(int drum_id) const;
    
private:
    int default_accuracy;
    std::map<int, InstrumentConfig> instruments; // 乐器配置，键为乐器ID
    std::map<int, InstrumentConfig> drums;       // 鼓声配置，键为鼓声ID
    
    // 解析一行
    void parseLine(const std::string& line, std::string& current_section);
    
    // 解析乐器/鼓声配置
    void parseInstrumentConfig(const std::string& line, const std::string& section);
    
    // 解析宏指令
    std::vector<MacroCommand> parseMacros(const std::string& macro_str);
    
    // 去除字符串两端的空白字符
    std::string trim(const std::string& str);
    
    // 分割字符串
    std::vector<std::string> split(const std::string& str, char delimiter);
};

#endif // INI_PARSER_H