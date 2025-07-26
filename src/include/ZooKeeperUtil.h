#ifndef ZOOKEEPER_UTIL_H
#define ZOOKEEPER_UTIL_H

#include <zookeeper/zookeeper.h>
#include <string>
#include <mutex>
#include <future>
#include <condition_variable>

// 封装zk客户端
class ZkClient {
public:
    ZkClient();
    ~ZkClient();

    void Start();  // 启动 ZooKeeper 客户端
    bool CreateAsync(const char* path, const char* data, int datalen, int state);  // 异步创建节点
    std::string GetDataAsync(const char* path);  // 异步获取节点数据
    bool ExistsAsync(const char* path);  // 异步检查节点是否存在

private:
    zhandle_t* m_zhandle;  // ZooKeeper 客户端句柄
};

#endif // ZOOKEEPER_UTIL_H