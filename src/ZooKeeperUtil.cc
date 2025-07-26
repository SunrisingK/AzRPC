#include "ZooKeeperUtil.h"
#include "AzRPC_Application.h"
#include "AzRPC_Logger.h"
#include <mutex>
#include <future>
#include <condition_variable>

std::mutex con_var_mtx;             // 全局锁, 用于保护共享变量的线程安全
std::condition_variable con_var;    // 条件变量, 用于线程间通信
bool is_connected = false;          // 标记 ZooKeeper 客户端是否连接成功

// 全局 watcher, 用于接收 ZooKeeper 服务器的通知
void global_watcher(zhandle_t* zh, int type, int status, const char* path, void* watcherCtx) {
    if (type == ZOO_SESSION_EVENT) {
        if (status == ZOO_CONNECTED_STATE) {
            std::lock_guard<std::mutex> lock(con_var_mtx);
            is_connected = true;  // 标记连接成功
            con_var.notify_all();  // 通知等待的线程
        }
    }
}

// 获取数据的回调函数
void get_data_completion(int rc, const char* value, int value_len, const struct Stat* stat, const void* data) {
    auto* promise = (std::promise<std::string>*)data;
    if (rc == ZOK) {
        promise->set_value(std::string(value, value_len));
    } 
    else {
        LOG(ERROR) << "get data failed, error: " << zerror(rc);
        promise->set_value("");
    }
    delete promise;
}

// 创建节点的回调函数
void create_completion(int rc, const char* path, const void* data) {
    auto* promise = (std::promise<bool>*)data;
    if (rc == ZOK) {
        LOG(INFO) << "znode create success... path: " << path;
        promise->set_value(true);
    } 
    else {
        LOG(ERROR) << "znode create failed... path: " << path << ", error: " << zerror(rc);
        promise->set_value(false);
    }
    delete promise;
}

// 检查节点是否存在的回调函数
struct ExistsCallbackData {
    ZkClient* client;
    std::string path;
    std::promise<bool>* promise;  // 可选：如果需要返回结果
};

void exists_completion(int rc, const struct Stat* stat, const void* data) {
    auto* cb_data = (ExistsCallbackData*)data;
    if (rc == ZNONODE) {
        // 节点不存在，触发创建
        cb_data->client->CreateAsync(cb_data->path.c_str(), "", 0, 0);
    } 
    else if (rc == ZOK) {
        LOG(INFO) << "znode already exists... path: " << cb_data->path;
    } 
    else {
        LOG(ERROR) << "zoo_aexists failed... path: " << cb_data->path << ", error: " << zerror(rc);
    }
    delete cb_data;  // 释放内存
}

// 构造函数
ZkClient::ZkClient() : m_zhandle(nullptr) {}

// 析构函数
ZkClient::~ZkClient() {
    if (m_zhandle != nullptr) {
        zookeeper_close(m_zhandle);
    }
}

// 启动 ZooKeeper 客户端
void ZkClient::Start() {
    std::string host = AzRPC_Application::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = AzRPC_Application::GetInstance().GetConfig().Load("zookeeperport");
    std::string connect_str = host + ":" + port;

    m_zhandle = zookeeper_init(connect_str.c_str(), global_watcher, 6000, nullptr, nullptr, 0);
    if (m_zhandle == nullptr) {
        LOG(ERROR) << "zookeeper_init error";
        exit(EXIT_FAILURE);
    }

    std::unique_lock<std::mutex> lock(con_var_mtx);
    con_var.wait(lock, [] { return is_connected; });
    LOG(INFO) << "zookeeper_init success";
}

// 异步创建节点
bool ZkClient::CreateAsync(const char* path, const char* data, int datalen, int state) {
    auto* promise = new std::promise<bool>();
    auto future = promise->get_future();

    int rc = zoo_acreate(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, create_completion, promise);
    
    if (rc != ZOK) {
        LOG(ERROR) << "Failed to initiate async create, error: " << zerror(rc);
        delete promise;
        return false;
    }
    
    return future.get();
}

// 异步获取节点数据
std::string ZkClient::GetDataAsync(const char* path) {
    auto* promise = new std::promise<std::string>();
    auto future = promise->get_future();

    int rc = zoo_aget(m_zhandle, path, 0, get_data_completion, promise);
    
    if (rc != ZOK) {
        LOG(ERROR) << "Failed to initiate async get, error: " << zerror(rc);
        delete promise;
        return "";
    }
    
    return future.get();
}

// 异步检查节点是否存在
bool ZkClient::ExistsAsync(const char* path) {
    auto* cb_data = new ExistsCallbackData{this, path, nullptr};  // 不返回 promise，仅用于回调
    int rc = zoo_aexists(m_zhandle, path, 0, exists_completion, cb_data);
    
    if (rc != ZOK) {
        LOG(ERROR) << "Failed to initiate async exists, error: " << zerror(rc);
        delete cb_data;
        return false;
    }
    
    return true;
}