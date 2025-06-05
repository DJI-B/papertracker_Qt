//
// Created by JellyfishKnight on 25-3-1.
//

#ifndef OSC_HPP
#define OSC_HPP

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "ip/UdpSocket.h"
#include "logger.hpp"

// 前向声明oscpack类

class OscManager {
public:
    OscManager();
    ~OscManager();

    // 初始化OSC管理器
    bool init(const std::string& address = "127.0.0.1", int port = 8888);

    // 设置OSC前缀
    void setLocationPrefix(const std::string& prefix);

    // 发送模型输出
    bool sendModelOutput(const std::vector<float>& output, const std::vector<std::string>& blend_shapes);

    // 关闭连接
    void close();

private:
    std::string address_;
    int port_;
    std::string location_prefix_;
    float multiplier_;
    std::unique_ptr<UdpTransmitSocket> socket_;
    std::mutex mutex_;
};


#endif //OSC_HPP
