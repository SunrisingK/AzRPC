#include "AzRPC_Application.h"
#include <cstdlib>
#include <unistd.h>

AzRPC_Config AzRPC_Application::m_config;   // 全局配置对象
std::mutex AzRPC_Application::mtx;          // 线程安全的互斥锁

// 单例对象指针
AzRPC_Application* AzRPC_Application::m_application = nullptr;

// 初始化函数, 解析命令行参数并加载配置文件
void AzRPC_Application::Init(int argc, char const* argv[]) {
    // 如果命令行参数少于2个，说明没有指定配置文件
    if (argc < 2) {
        std::cout << "格式: command -i <配置文件路径>" << std::endl;
        exit(EXIT_FAILURE);
    }

    int o;
    std::string config_file;
    // 使用getopt解析命令行参数, -i表示制定配置文件
    while (-1 != (o = getopt(argc, const_cast<char* const*>(argv), "i:"))) {
        switch (o) {
            case 'i':   // 如果参数是-i，后面的值就是配置文件的路径
                config_file = optarg;  // 将配置文件路径保存到config_file
                break;
            case '?':  // 如果出现未知参数（不是-i），提示正确格式并退出
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
                break;
            case ':':  // 如果-i后面没有跟参数，提示正确格式并退出
                std::cout << "格式: command -i <配置文件路径>" << std::endl;
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

    // 记载配置文件
    m_config.LoadConfigFile(config_file.c_str());
}

// 获取单例对象的引用, 保证全局只有一个实例
AzRPC_Application& AzRPC_Application::GetInstance() {
    std::lock_guard<std::mutex> lock(mtx);
    if (m_application == nullptr) {
        // 未创建单例时创建一个实例
        m_application = new AzRPC_Application();
        atexit(deleteInstance);  // 注册atexit函数，程序退出时自动销毁单例对象
    }
    return *m_application;  // 返回单例对象的引用
}

// 程序退出时自动调用该函数, 销毁单例对象
void AzRPC_Application::deleteInstance() {
    if (m_application) {
        delete m_application;
    }
}

// 获取全局配置对象的引用
AzRPC_Config& AzRPC_Application::GetConfig() {
    return m_config;
}