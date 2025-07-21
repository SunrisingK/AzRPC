#ifndef _ZooKeeperUtil_H_
#define _ZooKeeperUtil_H_
#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>

// 封装zk客户端
class ZkClient {
public:
    // zkclient启动连接zkserver
    void Start();
    
    // 根据指定的path在zkserver中创建一个结点
    void Create(const char* path, const char* data, int datalen, int state=0);

    // 根据参数指定znode结点路径, 或者znode结点值
    std::string GetData(const char* path);

private:
    // 客户端句柄
    zhandle_t* m_zhandle;
};

#endif