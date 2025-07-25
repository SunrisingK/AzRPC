#ifndef _AzRPC_Provider_H_
#define _AzRPC_Provider_H_

#include <google/protobuf/service.h>
#include "ZooKeeperUtil.h"
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h> 
#include <muduo/net/TcpConnection.h>
#include <google/protobuf/descriptor.h> 
#include <functional>
#include <string>
#include <unordered_map>

class AzRPC_Provider {
public:
    void NotifyService(google::protobuf::Service* service);

    // 启动RPC服务结点, 提供RPC远程调用服务
    void Run();

    ~AzRPC_Provider();

private:
    muduo::net::EventLoop event_loop;

    struct ServiceInfo {
        google::protobuf::Service* service;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> method_map;
    };
    //保存服务对象和rpc方法
    std::unordered_map<std::string, ServiceInfo> service_map;

    void OnConnection(const muduo::net::TcpConnectionPtr& connection);
    void OnMessage(const muduo::net::TcpConnectionPtr& connection, muduo::net::Buffer* buffer, muduo::Timestamp receive_time);
    void SendRpcResponse(const muduo::net::TcpConnectionPtr& conn, google::protobuf::Message* response);
};

#endif