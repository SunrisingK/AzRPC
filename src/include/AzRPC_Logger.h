#ifndef AzRPC_LOG_H
#define AzRPC_LOG_H
#include <glog/logging.h>
#include <string>

class AzRPC_Logger {
private:
    // 禁用拷贝构造函数和重载赋值函数
    AzRPC_Logger(const AzRPC_Logger&) = delete;
    AzRPC_Logger& operator=(const AzRPC_Logger&) = delete;
    
public:
    // 构造函数自动初始化glog
    explicit AzRPC_Logger(const char* argv) {
        google::InitGoogleLogging(argv);
        FLAGS_colorlogtostderr = true;    // 启用彩色日志
        FLAGS_logtostderr = true;         // 默认输出标准错误
    }

    ~AzRPC_Logger() {
        // 关闭logger
        google::ShutdownGoogleLogging();
    }

    // 提供静态日志
    static void Info(const std::string& message) {
        LOG(INFO) << message;
    }

    // 提供静态警告
    static void Warning(const std::string& message) {
        LOG(WARNING) << message;
    }

    // 提供静态错误
    static void ERROR(const std::string& message) {
        LOG(ERROR) << message;
    }

    // 提供静态重要错误
    static void Fatal(const std::string& message) {
        LOG(FATAL) << message;
    }
};

#endif