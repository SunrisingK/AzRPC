#include "AzRPC_Channel.h"
#include "AzRPC_Header.pb.h"
#include "ZooKeeperUtil.h"
#include "AzRPC_Application.h"
#include "AzRPC_Controller.h"
#include <memory>
#include <error.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "AzRPC_Logger.h"

std::mutex global_data_mtx;     // 全局互斥锁, 用于保护共享数据的线程安全

// RPC调用的核心方法, 将客户端的请求序列化并发送到服务端, 同时接收服务端的响应
void AzRPC_Channel::CallMethod(const ::google::protobuf::MethodDescriptor *method, ::google::protobuf::RpcController *controller, const ::google::protobuf::Message *request,::google::protobuf::Message *response, ::google::protobuf::Closure *done) {
    if (m_clientfd != -1) {
        // 客户端socket未初始化, 获取服务对象名和方法名
        const google::protobuf::ServiceDescriptor* sd = method->service();
        service_name = sd->name();
        method_name = method->name();

        // 客户端需要查询ZooKee, 找到提供服务的服务器地址
        ZkClient zkClient;
        zkClient.Start();
        // 查询服务器地址
        std::string host_data = QueryServiceHost(&zkClient, service_name, method_name, m_idx);
        m_ip = host_data.substr(0, m_idx);  // 从查询结果中获取ip地址
        std::cout << "ip: " << m_ip << std::endl;
        m_port = atoi(host_data.substr(m_idx + 1, host_data.size() - m_idx).c_str());
        std::cout << "port: " << m_port << std::endl;

        // 尝试连接服务器
        auto rt = newConnect(m_ip.c_str(), m_port);
        if (!rt) {
            LOG(ERROR) << "connect server error";
            return;
        }
        else {
            // 连接成功, 记录日志
            LOG(INFO) << "connect server success";
        }
    }

    // 将请求参数序列化为字符串, 计算其长度
    uint32_t args_size{};
    std::string args_str;
    // 序列化请求参数
    if (request->SerializeToString(&args_str)) {
        args_size = args_str.size();    // 获取序列化后的长度
    }
    else {
        // 序列化失败, 设置错误信息
        controller->SetFailed("serialize request fail");
        return;
    }

    // 定义RPC请求的头部信息
    AzRPC::RpcHeader azrpcHeader;
    azrpcHeader.set_service_name(service_name);
    azrpcHeader.set_method_name(method_name);
    azrpcHeader.set_args_size(args_size);

    // 将RPC头部信息序列化为字符串, 并计算其长度
    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (azrpcHeader.SerializeToString(&rpc_header_str)) {
        header_size = rpc_header_str.size();
    }
    else {
        // 序列化失败, 设置错误信息
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 将头部长度和头部信息拼接成完整的RPC请求报文
    std::string send_rpc_str;
    {
        google::protobuf::io::StringOutputStream string_output(&send_rpc_str);
        google::protobuf::io::CodedOutputStream coded_output(&string_output);
        // 写入头部长度
        coded_output.WriteVarint32(static_cast<uint32_t>(header_size));
        // 写入头部信息
        coded_output.WriteString(rpc_header_str);
    }
    send_rpc_str += args_str;   // 拼接请求参数

    // 发送RPC请求到服务器
    if (send(m_clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0) == -1) {
        close(m_clientfd);      // 发送失败, 关闭socket
        char errtxt[512] = {};
        // 打印错误信息
        std::cout << "send error: " << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;
        controller->SetFailed(errtxt);      // 设置错误信息
        return;
    }

    // 接受服务器响应
    char recv_buf[1024] = {0};
    int recv_size = recv(m_clientfd, recv_buf, 1024, 0);
    if (recv_size == -1) {
        char errtxt[512] = {};
        // 打印错误信息
        std::cout << "recv error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;
        controller->SetFailed(errtxt);  // 设置错误信息
        return;
    }

    // 将接收到的响应数据反序列化为response对象
    if (!response->ParseFromArray(recv_buf, recv_size)) {
        close(m_clientfd);      // 反序列化失败, 关闭socket
        char errtxt[512] = {};
        std::cout << "parse error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;
        controller->SetFailed(errtxt);
        return;
    }

    close(m_clientfd);
}

// 创建socket连接
bool AzRPC_Channel::newConnect(const char* ip, uint16_t port) {
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        char errtxt[512] = {};
        std::cout << "socket error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;
        LOG(ERROR) << "socket error: " << errtxt;
        return false;
    }

    // 设置服务器地址信息
    struct sockaddr_in server_addr;                 // IPv4地址族
    server_addr.sin_family = htons(port);           // 端口号
    server_addr.sin_addr.s_addr = inet_addr(ip);    // IP地址

    // 尝试连接服务器
    if (connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        close(clientfd);    // 连接失败关闭socket
        char errtxt[512] = {};
        std::cout << "connect error" << strerror_r(errno, errtxt, sizeof(errtxt)) << std::endl;
        LOG(ERROR) << "connect error: " << errtxt;
        return false;
    }

    // 保存socket文件描述符
    m_clientfd = clientfd;
    return true;
}

// 从ZooKeeper查询服务地址
std::string AzRPC_Channel::QueryServiceHost(ZkClient* zkclient, std::string service_name, std::string method_name, int &idx) {
    // 构造ZooKeeper路径
    std::string method_path = "/" + service_name + "/" + method_name;
    std::cout << "method_path: " << method_path << std::endl;

    std::unique_lock<std::mutex> lock(global_data_mtx);
    // 从ZooKeeper获取数据
    std::string host_data_1 = zkclient->GetData(method_path.c_str());
    lock.unlock();

    // 没有找到服务地址
    if (host_data_1 == "") {
        LOG(ERROR) << method_path + " is not exist!";
        return " ";
    }

    idx = host_data_1.find(":");    // 查找IP和端口的分隔符
    if (idx == -1) {
        LOG(ERROR) << method_path + " address is not valid!";
        return " ";
    }
    // 返回服务器地址
    return host_data_1;
}

// 构造函数, 支持延迟连接
AzRPC_Channel::AzRPC_Channel(bool connectNow): m_clientfd(-1), m_idx(0) {
    // 不需要立即连接
    if (!connectNow) {
        return;
    }

    // 最多3次尝试连接服务器
    auto rt = newConnect(m_ip.c_str(), m_port);
    int count = 3;
    while (!rt && count > 0) {
        rt = newConnect(m_ip.c_str(), m_port);
        --count;
    }
}