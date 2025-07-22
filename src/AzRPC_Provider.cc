#include "AzRPC_Provider.h"
#include "AzRPC_Application.h"
#include "AzRPC_Header.pb.h"
#include "AzRPC_Logger.h"
#include <iostream>

// 注册服务对象及其方法, 以便服务端能够处理客户端的RPC请求
void AzRPC_Provider::NotifyService(google::protobuf::Service* service) {
    // 服务端需要知道客户端需要调用的服务对象和方法
    // 这些信息会保存在ServiceInfo数据结构中
    ServiceInfo service_info;

    // 参数类型设置为 google::protobuf::Service, 是因为所有由 protobuf 生成的服务类
    // 都继承自 google::protobuf::Service, 这样可以通过基类指针指向子类对象, 实现动态多态。
    // 通过动态多态调用 service->GetDescriptor()
    // GetDescriptor() 方法会返回 protobuf 生成的服务类的描述信息(ServiceDescriptor)


    // 通过 ServiceDescriptor获取该服务类中定义的方法列表
    // 并进行相应的注册和管理
    const google::protobuf::ServiceDescriptor* psd = service->GetDescriptor();

    // 获取服务的名字
    std::string service_name = psd->name();
    // 获取服务端对象service的方法数量
    int method_count = psd->method_count();

    // 打印服务名
    std::cout << "service_name = " << service_name << std::endl;

    // 遍历服务中的所有方法, 并注册到服务信息中
    for (int i = 0; i < method_count; ++i) {
        // 获取服务中的方法描述
        const google::protobuf::MethodDescriptor* pmd = psd->method(i);
        std::string method_name = pmd->name();
        std::cout << "method_name = " << method_name << std::endl;
        // 将方法名和方法描述符存入map
        service_info.method_map.emplace(method_name, pmd);
    }
    service_info.service = service;     // 保存服务对象
    service_map.emplace(service_name, service_info);    // 将服务信息存入服务map
}

// 启动RPC服务器结点, 开始提供远程网络调用服务
void AzRPC_Provider::Run() {
    // 读取配置文件中的RPC服务器IP和端口
    std::string ip = AzRPC_Application::GetInstance().GetConfig().Load("rpcserverip");
    int port = atoi(AzRPC_Application::GetInstance().GetConfig().Load("rpcserverport").c_str());

    // 使用muduo网络库, 创建地址对象
    muduo::net::InetAddress address(ip, port);

    // 创建TpcServer对象
    std::shared_ptr<muduo::net::TcpServer> server = std::make_shared<muduo::net::TcpServer>(&event_loop, address, "AzRPC_Provider");

    // 绑定连接回调和消息回调, 分离网络链接业务和消息处理业务
    server->setConnectionCallback(std::bind(&AzRPC_Provider::OnCennection, this, std::placeholders::_1));
    server->setMessageCallback(std::bind(&AzRPC_Provider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


    // 使用muduo库的线程数量
    server->setThreadNum(4);

    // 将当前RPC节点上要发布的服务全部注册到ZooKeeper上，让RPC客户端可以在ZooKeeper上发现服务
    ZkClient zkclient;
    zkclient.Start();   // 连接Zookeeper服务器
    // service_name为永久结点, method_name为临时结点
    for (auto& sp: service_map) {
        // service_name 在ZooKeeper中的目录是"/"+service_name
        std::string service_path = "/" + sp.first;
        // 创建服务结点
        zkclient.Create(service_path.c_str(), nullptr, 0);
        for (auto& mp: sp.second.method_map) {
            std::string method_path = service_path + "/" +mp.first;
            char method_path_data[128] = {0};
            // 将IP和端口信息存入结点数据
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示这个节点是临时节点, 在客户端断开连接后, ZooKeeper会自动删除这个节点
            zkclient.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // RPC服务端准备启动, 打印信息
    std::cout << "AzRPC_Provider start service at ip: " << ip << " port: " << port << std::endl;

    // 启动网络服务
    server->start();
    // 进入事件循环
    event_loop.loop();
}