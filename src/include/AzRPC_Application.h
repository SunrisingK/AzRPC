#ifndef _AzRPC_Application_H
#define _AzRPC_Application_H
#include "AzRPC_Config.h"
#include "AzRPC_Channel.h"
#include "AzRPC_Controller.h"
#include <mutex>

// RPC基础类, 负责框架的一些初始化操作(单例模式)
class AzRPC_Application {
public:
    static void Init(int argc, char const* argv[]);
    static AzRPC_Application& GetInstance();
    static void deleteInstance();
    static AzRPC_Config& GetConfig();

private:
    static AzRPC_Config m_config();
    static AzRPC_Application* m_application;    // 全局唯一单利访问对象
    static std::mutex mtx;
    AzRPC_Application(const AzRPC_Application&)=delete;
    AzRPC_Application(AzRPC_Application&&)=delete;
};

#endif