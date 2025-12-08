#include "ini_parser.h"
#include <iostream>
#include <algorithm>

IniParser::IniParser() : default_accuracy(30) {
    // 初始化默认精度为30
}

IniParser::~IniParser() {
    // 清理资源
}

bool IniParser::parse(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open INI file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::string current_section = "";
    
    while (std::getline(file, line)) {
        // 去除行首尾空白
        line = trim(line);
        
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 解析行
        parseLine(line, current_section);
    }
    
    file.close();
    return true;
}

int IniParser::getDefaultAccuracy() const {
    return default_accuracy;
}

const InstrumentConfig& IniParser::getInstrumentConfig(int instrument_id) const {
    static InstrumentConfig default_config;
    
    auto it = instruments.find(instrument_id);
    if (it != instruments.end()) {
        return it->second;
    }
    
    // 返回默认配置
    return default_config;
}

const InstrumentConfig& IniParser::getDrumConfig(int drum_id) const {
    static InstrumentConfig default_config;
    
    auto it = drums.find(drum_id);
    if (it != drums.end()) {
        return it->second;
    }
    
    // 返回默认配置
    return default_config;
}

bool IniParser::hasInstrument(int instrument_id) const {
    return instruments.find(instrument_id) != instruments.end();
}

bool IniParser::hasDrum(int drum_id) const {
    return drums.find(drum_id) != drums.end();
}

void IniParser::parseLine(const std::string& line, std::string& current_section) {
    // 检查是否是节标题
    if (line[0] == '[' && line.back() == ']') {
        current_section = line.substr(1, line.length() - 2);
        return;
    }
    
    // 解析键值对
    size_t equal_pos = line.find('=');
    if (equal_pos == std::string::npos) {
        return; // 不是有效的键值对
    }
    
    std::string key = trim(line.substr(0, equal_pos));
    std::string value = trim(line.substr(equal_pos + 1));
    
    // 处理General节
    if (current_section == "General") {
        if (key == "default_accuracy") {
            default_accuracy = std::stoi(value);
        }
        return;
    }
    
    // 处理乐器和鼓声配置
    if (current_section.find("Instrument_") == 0) {
        parseInstrumentConfig(line, current_section);
    } else if (current_section.find("Drum_") == 0) {
        parseInstrumentConfig(line, current_section);
    }
}

void IniParser::parseInstrumentConfig(const std::string& line, const std::string& section) {
    size_t equal_pos = line.find('=');
    if (equal_pos == std::string::npos) {
        return; // 不是有效的键值对
    }
    
    std::string key = trim(line.substr(0, equal_pos));
    std::string value = trim(line.substr(equal_pos + 1));
    
    // 提取ID
    int id = -1;
    if (section.find("Instrument_") == 0) {
        id = std::stoi(section.substr(11)); // "Instrument_" 长度为11
    } else if (section.find("Drum_") == 0) {
        id = std::stoi(section.substr(5)); // "Drum_" 长度为5
    }
    
    if (id == -1) {
        return;
    }
    
    // 确定使用哪个容器
    std::map<int, InstrumentConfig>* container = nullptr;
    if (section.find("Instrument_") == 0) {
        container = &instruments;
    } else if (section.find("Drum_") == 0) {
        container = &drums;
    }
    
    if (!container) {
        return;
    }
    
    // 如果配置不存在，创建一个默认配置
    if (container->find(id) == container->end()) {
        InstrumentConfig config;
        config.name = "Unknown";
        config.accuracy = default_accuracy;
        (*container)[id] = config;
    }
    
    // 更新配置
    InstrumentConfig& config = (*container)[id];
    
    if (key == "name") {
        config.name = value;
    } else if (key == "accuracy") {
        config.accuracy = std::stoi(value);
    } else if (key == "vol" || key == "note" || key == "wave" || key == "pitch_bend") {
        // 解析宏指令
        std::vector<MacroCommand> macros = parseMacros(key + ":" + value);
        config.macros.insert(config.macros.end(), macros.begin(), macros.end());
    }
}

std::vector<MacroCommand> IniParser::parseMacros(const std::string& macro_str) {
    std::vector<MacroCommand> result;
    
    // 分割宏类型和宏指令
    size_t colon_pos = macro_str.find(':');
    if (colon_pos == std::string::npos) {
        return result;
    }
    
    std::string macro_type = trim(macro_str.substr(0, colon_pos));
    std::string commands_str = trim(macro_str.substr(colon_pos + 1));
    
    // 创建宏指令
    MacroCommand macro;
    macro.type = macro_type;
    
    // 解析宏指令元素
    std::vector<std::string> tokens = split(commands_str, ' ');
    
    bool in_release_section = false; // 标志，指示当前是否在解析释放阶段的宏
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::string token = trim(tokens[i]);
        
        if (token.empty()) continue;
        
        // 检查是否是 "release" 关键字
        if (token == "release") {
            in_release_section = true;
            continue; // 跳过 "release" 关键字本身
        }
        
        MacroElement element;
        
        // 解析不同类型的宏指令元素
        if (token == "loop_start") {
            element.type = "loop_start";
            if (in_release_section) macro.release_elements.push_back(element);
            else macro.on_elements.push_back(element);
        } else if (token == "loop_end") {
            element.type = "loop_end";
            if (in_release_section) macro.release_elements.push_back(element);
            else macro.on_elements.push_back(element);
        } else if (token.find(">=") == 0) {
            // 解析 >= 格式，如 >=10 40
            element.type = "transition";
            element.duration = std::stoi(token.substr(2));
            
            // 下一个token是目标值
            if (i + 1 < tokens.size()) {
                element.value = std::stoi(tokens[++i]);
                if (in_release_section) macro.release_elements.push_back(element);
                else macro.on_elements.push_back(element);
            }
        } else if (token.find("~") != std::string::npos && token.find("=") != std::string::npos) {
            // 解析范围格式，如 35~45=10
            size_t tilde_pos = token.find('~');
            size_t equal_pos = token.find('=');
            
            element.type = "range";
            element.start_value = std::stoi(token.substr(0, tilde_pos));
            element.end_value = std::stoi(token.substr(tilde_pos + 1, equal_pos - tilde_pos - 1));
            element.duration = std::stoi(token.substr(equal_pos + 1));
            
            if (in_release_section) macro.release_elements.push_back(element);
            else macro.on_elements.push_back(element);
        } else if (token.find("=") != std::string::npos && token.find("~") == std::string::npos) {
            // 解析持续值格式，如 =5
            element.type = "hold";
            element.duration = std::stoi(token.substr(1));
            
            // 前一个token应该是值
            if (in_release_section) {
                if (!macro.release_elements.empty() && macro.release_elements.back().type == "value") {
                    element.value = macro.release_elements.back().value;
                    macro.release_elements.back() = element;
                } else if (i > 0) {
                    try {
                        element.value = std::stoi(tokens[i-1]);
                        macro.release_elements.push_back(element);
                    } catch (...) {
                        // 如果解析失败，跳过
                    }
                }
            } else {
                if (!macro.on_elements.empty() && macro.on_elements.back().type == "value") {
                    element.value = macro.on_elements.back().value;
                    macro.on_elements.back() = element;
                } else if (i > 0) {
                    try {
                        element.value = std::stoi(tokens[i-1]);
                        macro.on_elements.push_back(element);
                    } catch (...) {
                        // 如果解析失败，跳过
                    }
                }
            }
        } else {
            // 尝试解析为数值
            try {
                element.type = "value";
                element.value = std::stoi(token);
                if (in_release_section) macro.release_elements.push_back(element);
                else macro.on_elements.push_back(element);
            } catch (...) {
                // 如果不是数值，可能是其他特殊指令，暂时跳过
            }
        }
    }
    
    result.push_back(macro);
    return result;
}

std::string IniParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> IniParser::split(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {
            result.push_back(item);
        }
    }
    
    return result;
}