#include "AzRPC_Config.h"
#include <memory>

// 加载配置文件, 解析配置文件中的键值对
void AzRPC_Config::LoadConfigFile(const char* config_file) {
    // 使用智能指针管理文件指针, 确保文件在退出时自动关闭
    std::unique_ptr<FILE, decltype(&fclose)> pf(fopen(config_file, "r"), &fclose);
    
    // 文件打开失败就退出程序
    if (pf == nullptr) {
        exit(EXIT_FAILURE);
    }

    // 用于存储从文件中读取的每一行内容
    char buf[1024];
    while (fgets(buf, 1024, pf.get()) != nullptr) {
        // 将读取的内容转为字符串
        std::string read_buf(buf);
        Trim(read_buf);     // 去掉字符串前后的空格

        // 忽略注释行(以#开头)和空行
        if (read_buf[0] == '#' || read_buf.empty()) {
            continue;
        }

        // 查找键值对的分隔符'='
        int index = read_buf.find('=');
        if (index == -1) continue;

        // 提取 key
        std::string key = read_buf.substr(0, index);
        Trim(key);

        // 查找行尾的换行符
        int endindex = read_buf.find('\n', index);
        std::string value = read_buf.substr(index + 1, endindex - index - 1);
        Trim(value);

        // 将键值对存入配置map中
        config_map.insert({key, value});
    }
}

// 根据key查找对应的vale
std::string AzRPC_Config::Load(const std::string& key) {
    auto it = config_map.find(key);
    // 没有找到key就返回空字符串
    if (it == config_map.end()) {
        return "";    
    }

    return it->second;
}

// 去掉字符串前后空格的函数
void AzRPC_Config::Trim(std::string& read_buf) {
    // 先去掉前面的空格
    int index = read_buf.find_first_not_of(' ');
    if (index != -1) {
        read_buf = read_buf.substr(index, read_buf.size() - index);
    }

    // 再去掉后面的空格
    index = read_buf.find_last_not_of(' ');
    if (index != -1) {
        read_buf = read_buf.substr(0, index + 1);
    }
}