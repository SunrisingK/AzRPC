#include <iostream>
#include <string>
#include "../user.pb.h"
#include "src/include/AzRPC_Application.h"
#include "src/include/AzRPC_Provider.h"

/**
    UserService 原本是一个本地服务, 提供了两个本地方法: Login 和 GetFriendLists
    现在通过 RPC 框架, 这些方法可以被远程调用
*/

class UserService: public AzUser::UserServiceRpc {
public:
    // 本地登录方法, 用于处理实际的业务逻辑
    bool Login(std::string name, std::string pwd) {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name: " << name << " pwd: " << pwd << std::endl;
        return true;
    }

    /**
        重写基类 UserServiceRpc 的虚函数, 这些方法会被 RPC 框架直接调用
        1. 调用者（caller）通过 RPC 框架发送 Login 请求
        2. 服务提供者（callee）接收到请求后，调用下面重写的 Login 方法
    */

    void Login(::google::protobuf::RpcController* controller, const ::AzUser::LoginRequest* request, ::AzUser::LoginResponse* response, ::google::protobuf::Closure* done) {
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 调用本地业务逻辑处理登录
        bool login_result = Login(name, pwd);

        // 将响应结果写入response对象
        AzUser::ResultCode* code = response->mutable_result();
        code->set_errcode(0);       // 设置错误码为0, 表示成功
        code->set_errmsg("");       // 设置错误信息为空
        // 设置登录结果
        response->set_success(login_result);

        // 执行回调操作, 框架会自动将响应序列化并发送给调用者
        done->Run();
    }
};

int main(int argc, char const* argv[]) {
    // 调用框架的初始化操作，解析命令行参数并加载配置文件
    AzRPC_Application::Init(argc, argv);

    // 创建一个 RPC 服务提供者对象
    AzRPC_Provider provider;

    // 将 UserService 对象发布到 RPC 节点上，使其可以被远程调用
    provider.NotifyService(new UserService());

    // 启动 RPC 服务节点，进入阻塞状态，等待远程的 RPC 调用请求
    provider.Run();

    return 0;
}