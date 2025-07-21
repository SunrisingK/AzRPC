#ifndef AzRPC_LOG_H
#define AzRPC_LOG_H
#include <glog/logging.h>
#include <string>

class AzRPCLogger {
private:
    // 禁用拷贝构造函数和重载赋值函数
    AzRPCLogger(const AzRPCLogger&) = delete;
    AzRPCLogger& operator=(const AzRPCLogger&) = delete;
    
public:
    // 构造函数自动初始化glog
    explicit AzRPCLogger(const char* argv) {
        google::InitGoogleLogging(argv);
        FLAGS_colorlogtostderr = true;    // 启用彩色日志
        FLAGS_logtostderr = true;         // 默认输出标准错误
    }

    ~AzRPCLogger() {
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