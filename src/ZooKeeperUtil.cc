#include "ZooKeeperUtil.h"
#include "AzRPC_Application.h"
#include <mutex>
#include "AzRPC_Logger.h"
#include <condition_variable>

std::mutex con_var_mtx;             // 全局锁, 用于保护共享变量的线程安全
std::condition_variable con_var;    // 条件变量, 用于线程间通信
bool is_connected = false;          // 标记ZooKeeper客户端是否连接成功

// 全局watcher, 用于接收ZooKeeper服务器的通知
void global_watcher(zhandle_t* zh, int type, int status, const char* path, void* watcherCtx) {
    // 回调消息类型和会话相关的事件
    if (type == ZOO_SESSION_EVENT) {
        // ZooKeeper客户端和服务器连接成功
        if (status == ZOO_CONNECTED_STATE) {
            // 加锁保护
            std::lock_guard<std::mutex> lock(con_var_mtx);
            is_connected = true;        // 标记连接成功
        }
    }
    // 通知所有等待的线程
    con_var.notify_all();
}

// 构造函数, 初始化客户端句柄为空
ZkClient::ZkClient(): m_zhandle(nullptr) {}

// 析构函数, 关闭ZooKeeper连接
ZkClient::~ZkClient() {
    if (m_zhandle != nullptr) {
        zookeeper_close(m_zhandle);
    }
}

// 启动ZooKeeper客户端, 连接ZooKeeper服务器
void ZkClient::Start() {
    // 从配置文件中读取ZooKeeper服务器的IP和端口
    std::string host = AzRPC_Application::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = AzRPC_Application::GetInstance().GetConfig().Load("zookeeperport");
    std::string connect_str = host + ":" + port;

    /*
    zookeeper_mt：多线程版本
    ZooKeeper的API客户端程序提供了三个线程：
    1. API调用线程
    2. 网络I/O线程（使用pthread_create和poll）
    3. watcher回调线程（使用pthread_create）
    */

    // 使用zookeeper_init初始化一个ZooKeeper客户端对象，异步建立与服务器的连接
    m_zhandle = zookeeper_init(connect_str.c_str(), global_watcher, 6000, nullptr, nullptr, 0);
    if (m_zhandle == nullptr) {
        // 初始化失败
        LOG(ERROR) << "zookeeper_init error";
        exit(EXIT_FAILURE);     // 退出程序
    }

    // 等待连接成功
    std::unique_lock<std::mutex> lock(con_var_mtx);
    con_var.wait(lock, [] {return is_connected;});      // 阻塞等待直到连接成功
    LOG(INFO) << "zookeeper_init success";              // 记录日志, 表示连接成功
}

// 创建ZooKeeper结点
void ZkClient::Create(const char* path, const char* data, int datalen, int state) {
    char path_buffer[128];      // 存储创建的结点路径
    int bufferlen = sizeof(path_buffer);

    // 检查结点是否存在
    int flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (flag == ZNONODE) {
        // 不存在则创建ZooKeeper结点
        flag = zoo_create(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (flag == ZOK) {
            LOG(INFO) << "znode create success... path: " << path;
        }
        else {
            LOG(ERROR) << "znode create failed... path:" << path;
            exit(EXIT_FAILURE);  // 退出程序
        }
    }
    else {
        LOG(INFO) << "znode create success... path: " << path;
    }
}

// 获取ZooKeeper结点的数据
std::string ZkClient::GetData(const char* path) {
    char buf[64];    // 存储结点数据
    int bufferlen = sizeof(buf);

    std::string res = "";
    //  获取指定结点的数据
    int flag = zoo_get(m_zhandle, path, 0, buf, &bufferlen, nullptr);
    if (flag != ZOK) {
        // 获取失败返回空串
        LOG(ERROR) << "zoo_get error";
    }
    else {
        res = std::string(buf);
    }
    return res;
}