#ifndef _AzRPC_Config_H
#define _AzRPC_Config_H
#include <unordered_map>
#include <string>

class AzRPC_Config {
public:
    // 加载配置文件
    void LoadConfigFile(const char* config_file);

    // 查找key对应的value
    std::string Load(const std::string& key);

private:
    std::unordered_map<std::string, std::string> config_map;
    
    // 去掉字符串前后的空格
    void Trim(std::string& read_buf);
};

#endif