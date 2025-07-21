#ifndef _AzRPC_Channel_H_
#define _AzRPC_Channel_H_
#include <google/protobuf/service.h>
#include "ZooKeeperUtil.h"

class AzRPC_Channel: public google::protobuf::RpcChannel {
public:
    AzRPC_Channel(bool connectNow);
    virtual ~AzRPC_Channel() {}

    void CallMethod(const ::google::protobuf::MethodDescriptor* method, ::google::protobuf::RpcController* controller, const ::google::protobuf::Message *request, ::google::protobuf::Message* response, ::google::protobuf::Closure* done) override;

private:
    int m_clientfd;                 // 存放客户套接字
    std::string service_name;
    std::string m_ip;
    uint16_t m_port;
    std::string method_name;

    int m_idx;                      // 用来区分服务器ip和port的下标
    bool newConnect(const char* ip, uint16_t port);

    std::string QueryServiceHost(ZkClient* zkclient, std::string service_name, std::string method_name, int& idx);
};

#endif