/*
* PaperTracker - 面部追踪应用程序
 * Copyright (C) 2025 PAPER TRACKER
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file contains code from projectbabble:
 * Copyright 2023 Sameer Suri
 * Licensed under the Apache License, Version 2.0
 */
#include "image_downloader.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <QUrl>
#include <QMutexLocker>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
ESP32VideoStream::ESP32VideoStream(QObject *parent)
    : QObject(parent), isRunning(false), webSocket(nullptr), heartbeatTimer(nullptr)
{
}

ESP32VideoStream::~ESP32VideoStream() {
    if (isRunning) {
        stop();
    }

    // 确保 mdnsLookup 被正确清理
    if (mdnsLookup) {
        mdnsLookup->deleteLater();
        mdnsLookup = nullptr;
    }
}

// 修改 init 方法
// 修改 init 方法
bool ESP32VideoStream::init(const std::string& url, int deviceType) {
    if (url.empty()) {
        if (this->currentStreamUrl.empty()) {
            LOG_INFO("无法初始化WebSocket：URL为空");
            return false;  // 不允许初始化空URL
        }
        return !currentStreamUrl.empty();  // 如果已有URL则保持不变
    }

    // 清空连接 URL 列表
    connection_urls.clear();
    connection_attempts = 0;

    // 保存原始 URL，但不要立即连接
    currentStreamUrl = url;

    // 检查是否是 mDNS 地址
    QString qUrl = QString::fromStdString(url);
    if (qUrl.contains(".local", Qt::CaseInsensitive)) {
        LOG_INFO("检测到 mDNS 地址: {}", url);

        // 提取主机名部分
        QUrl parsedUrl(qUrl);
        QString hostname = parsedUrl.host();

        // 设置 mDNS 查询
        setupMdnsLookup(hostname);
    }

    // 转换 HTTP URL 为 WebSocket URL
    if (url.substr(0, 5) != "ws://" && url.substr(0, 6) != "wss://") {
        LOG_DEBUG("转换HTTP URL为WebSocket URL: {}", url);
        std::string wsUrl;

        if (url.substr(0, 7) == "http://") {
            wsUrl = "ws://" + url.substr(7) + "/ws";
        } else if (url.substr(0, 8) == "https://") {
            wsUrl = "wss://" + url.substr(8) + "/ws";
        } else {
            // 不是标准URL，假设是主机地址，添加ws://
            wsUrl = "ws://" + url;
            // 确保URL有正确的端口和路径
            if (wsUrl.find(':') == std::string::npos) {
                // 如果没有端口，添加默认端口81
                wsUrl += ":80";
            }
            if (wsUrl.find("/ws") == std::string::npos) {
                // 如果没有/ws路径，添加它
                wsUrl += "/ws";
            }
        }

        // 添加转换后的 URL
        if (!connection_urls.contains(QString::fromStdString(wsUrl))) {
            connection_urls.append(QString::fromStdString(wsUrl));
        }
    } else {
        // 已经是 WebSocket URL，直接添加
        if (!connection_urls.contains(QString::fromStdString(url))) {
            connection_urls.append(QString::fromStdString(url));
        }
    }

    // 根据设备类型添加固定的 mDNS 地址
    switch (deviceType) {
        case 1: // 面捕设备
            LOG_INFO("添加面捕设备 mDNS 地址: ws://paper1.local:80/ws");
            if (!connection_urls.contains("ws://paper1.local:80/ws")) {
                connection_urls.append("ws://paper1.local:80/ws");
            }
            break;
        case 2: // 左眼设备
            LOG_INFO("添加左眼设备 mDNS 地址: ws://paper2.local:80/ws");
            if (!connection_urls.contains("ws://paper2.local:80/ws")) {
                connection_urls.append("ws://paper2.local:80/ws");
            }
            break;
        case 3: // 右眼设备
            LOG_INFO("添加右眼设备 mDNS 地址: ws://paper3.local:80/ws");
            if (!connection_urls.contains("ws://paper3.local:80/ws")) {
                connection_urls.append("ws://paper3.local:80/ws");
            }
            break;
        default:
            // 如果deviceType为0或其他值，根据URL关键字推断设备类型
            if (url.find("face") != std::string::npos || url.find("paper1") != std::string::npos) {
                // 面捕设备
                if (!connection_urls.contains("ws://paper1.local:80/ws")) {
                    connection_urls.append("ws://paper1.local:80/ws");
                }
            } else if (url.find("left") != std::string::npos || url.find("paper2") != std::string::npos) {
                // 左眼设备
                if (!connection_urls.contains("ws://paper2.local:80/ws")) {
                    connection_urls.append("ws://paper2.local:80/ws");
                }
            } else if (url.find("right") != std::string::npos || url.find("paper3") != std::string::npos) {
                // 右眼设备
                if (!connection_urls.contains("ws://paper3.local:80/ws")) {
                    connection_urls.append("ws://paper3.local:80/ws");
                }
            } else {
                // 未知设备类型，不添加任何 mDNS 地址
                LOG_WARN("未知设备类型，不添加默认 mDNS 地址");
            }
            break;
    }

    LOG_DEBUG("初始化WebSocket视频流，URL列表: {}",
              connection_urls.join(", ").toStdString());

    return true;
}
// 新增 mDNS 查询的方法
void ESP32VideoStream::setupMdnsLookup(const QString& hostname) {
    if (mdnsLookup) {
        mdnsLookup->deleteLater();
    }

    mdnsLookup = new QDnsLookup(this);
    mdnsLookup->setType(QDnsLookup::A);
    mdnsLookup->setName(hostname);

    // 连接查询结果信号
    connect(mdnsLookup, &QDnsLookup::finished, this, [this, hostname]() {
        if (mdnsLookup->error() == QDnsLookup::NoError) {
            const auto records = mdnsLookup->hostAddressRecords();
            if (!records.isEmpty()) {
                // 获取 IP 地址
                const QHostAddress& address = records.first().value();
                QString ipAddress = address.toString();

                LOG_INFO("mDNS 解析成功: {} -> {}",
                         hostname.toStdString(),
                         ipAddress.toStdString());

                // 构建 WebSocket URL
                QString wsUrl = QString("ws://%1:80/ws").arg(ipAddress);

                // 添加到连接列表的最前面，优先尝试
                if (!connection_urls.contains(wsUrl)) {
                    connection_urls.prepend(wsUrl);
                    LOG_INFO("添加 mDNS 解析的 URL: {}", wsUrl.toStdString());
                }

                // 如果尚未开始连接，开始尝试连接
                if (connection_attempts == 0 && !isRunning) {
                    tryConnectToNextAddress();
                }
            } else {
                LOG_WARN("mDNS 解析未找到 IP 地址: {}", hostname.toStdString());
            }
        } else {
            LOG_WARN("mDNS 解析错误: {}: {}",
                     hostname.toStdString(),
                     mdnsLookup->errorString().toStdString());
        }

        // 即使 mDNS 解析失败，也开始尝试连接
        if (connection_attempts == 0 && !isRunning) {
            QTimer::singleShot(500, this, &ESP32VideoStream::tryConnectToNextAddress);
        }
    });

    // 开始查询
    mdnsLookup->lookup();

    LOG_INFO("开始 mDNS 解析: {}", hostname.toStdString());
}

// 尝试连接到下一个地址
void ESP32VideoStream::tryConnectToNextAddress() {
    if (connection_attempts >= connection_urls.size()) {
        LOG_DEBUG("无法通过WIFI链接到捕捉设备");
        connection_attempts=0;
        return;
    }

    QString urlToTry = connection_urls.at(connection_attempts);
    connection_attempts++;

    LOG_DEBUG("尝试连接到 URL ({}/{}): {}",
             connection_attempts,
             connection_urls.size(),
             urlToTry.toStdString());

    // 创建 WebSocket 连接
    if (webSocket) {
        disconnect(webSocket, nullptr, this, nullptr);
        webSocket->deleteLater();
    }

    webSocket = new QWebSocket();
    webSocket->setProxy(QNetworkProxy::NoProxy);

    // 连接信号
    connect(webSocket, &QWebSocket::connected, this, &ESP32VideoStream::onConnected);
    connect(webSocket, &QWebSocket::disconnected, this, &ESP32VideoStream::onDisconnected);
    connect(webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &ESP32VideoStream::onError);
    connect(webSocket, &QWebSocket::binaryMessageReceived,
            this, &ESP32VideoStream::onBinaryMessageReceived);
    connect(webSocket, &QWebSocket::textMessageReceived,
            this, &ESP32VideoStream::onTextMessageReceived);

    // 打开连接
    webSocket->open(QUrl(urlToTry));

    // 设置连接超时
    QTimer::singleShot(3000, this, [this, urlToTry]() {
        if (!isRunning && webSocket && webSocket->state() != QAbstractSocket::ConnectedState) {
            LOG_DEBUG("连接到 {} 超时，尝试下一个地址", urlToTry.toStdString());
            tryConnectToNextAddress();
        }
    });
}

void ESP32VideoStream::checkHeartBeat()
{
    // 检查URL是否为空
    if (currentStreamUrl.empty()) {
        // URL为空，停止心跳检查
        if (heartbeatTimer->isActive()) {
            heartbeatTimer->stop();
        }
        return;
    }

    if (image_not_receive_count++ > 50)
    {
        image_not_receive_count = 0;
        isRunning = false;
        stop();
        start();
    }
}

bool ESP32VideoStream::start() {
    // 检查URL是否为空
    if (connection_urls.isEmpty() && currentStreamUrl.empty()) {
        LOG_INFO("无法启动WebSocket连接：URL为空");
        return false;
    }

    webSocket = new QWebSocket();
    // 禁用代理设置，避免代理相关错误
    webSocket->setProxy(QNetworkProxy::NoProxy);

    connect(webSocket, &QWebSocket::connected,
        this, &ESP32VideoStream::onConnected);
    connect(webSocket, &QWebSocket::disconnected,
            this, &ESP32VideoStream::onDisconnected);
    connect(webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &ESP32VideoStream::onError);
    connect(webSocket, &QWebSocket::binaryMessageReceived,
            this, &ESP32VideoStream::onBinaryMessageReceived);
    // 添加文本消息处理的连接
    connect(webSocket, &QWebSocket::textMessageReceived,
            this, &ESP32VideoStream::onTextMessageReceived);

    if (!heartbeatTimer)
    {
        heartbeatTimer = new QTimer();
        connect(heartbeatTimer, &QTimer::timeout, this, &ESP32VideoStream::checkHeartBeat);
    }


    if (isRunning) {
        LOG_WARN("视频流已经在运行中");
        return false;
    }

    // 开始连接尝试
    connection_attempts = 0;
    tryConnectToNextAddress();

    if (!heartbeatTimer) {
        heartbeatTimer = new QTimer();
        connect(heartbeatTimer, &QTimer::timeout, this, &ESP32VideoStream::checkHeartBeat);
    }

    if (!heartbeatTimer->isActive()) {
        heartbeatTimer->start(50);
    }

    return true;
}

void ESP32VideoStream::stop() {
    LOG_DEBUG("停止WebSocket视频流");
    isRunning = false;

    // 停止 mDNS 查询
    if (mdnsLookup) {
        mdnsLookup->abort();
    }

    // 关闭WebSocket
    if (webSocket) {
        if (webSocket->state() != QAbstractSocket::UnconnectedState) {
            QMetaObject::invokeMethod(webSocket, "close", Qt::QueuedConnection);
        }
        webSocket->deleteLater();
        webSocket = nullptr;
    }

    // 清空图像队列
    QMutexLocker locker(&mutex);
    while (!image_buffer_queue.empty()) {
        image_buffer_queue.pop();
    }
}

cv::Mat ESP32VideoStream::getLatestFrame() const
{
    QMutexLocker locker(&mutex);
    if (image_buffer_queue.empty()) {
        return {};
    }
    return image_buffer_queue.front().clone();
}

// 修改 onConnected 方法
void ESP32VideoStream::onConnected() {
    LOG_INFO("成功连接到 WebSocket: {}", webSocket->requestUrl().toString().toStdString());
    isRunning = true;
    image_not_receive_count = 0;

    // 保存成功连接的 URL
    currentStreamUrl = webSocket->requestUrl().toString().toStdString();
}

void ESP32VideoStream::onDisconnected()
{
    LOG_DEBUG("WebSocket连接已关闭");
    isRunning = false;
}

void ESP32VideoStream::onError(QAbstractSocket::SocketError error) {
    QString errorString = webSocket->errorString();
    int errorCode = static_cast<int>(error);
    LOG_DEBUG("WebSocket错误: {}-{} (URL: {})",
             errorCode,
             errorString.toStdString(),
             webSocket->requestUrl().toString().toStdString());

    // 如果尚未成功运行，尝试下一个 URL
    if (!isRunning) {
        LOG_INFO("连接失败，尝试下一个地址");
        tryConnectToNextAddress();
    } else {
        LOG_ERROR("无线连接失败，请确保设备已经开机且连接上WIFI并且和电脑处于一个路由器下");
        isRunning = false;
    }
}

// 在 image_downloader.cpp 中添加新方法
void ESP32VideoStream::onTextMessageReceived(const QString &message) {
    try {
        // 尝试解析JSON
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("battery")) {
                battery_percentage = static_cast<float>(obj["battery"].toDouble());
                LOG_DEBUG("收到电池电量: {}%", battery_percentage);
            }
            if (obj.contains("brightness")) {
                brightness_value = obj["brightness"].toInt();
                LOG_DEBUG("收到亮度值: {}", brightness_value);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("处理文本消息出错: {}", e.what());
    }
}
void ESP32VideoStream::onBinaryMessageReceived(const QByteArray &message)
{
    isRunning = true;
    image_not_receive_count = 0;
    try {
        // 打印接收到的数据长度以进行调试
        //LOG_DEBUG("接收到WebSocket数据: " + std::to_string(message.size()) + " 字节");

        // 帧率计算
        static std::deque<std::chrono::steady_clock::time_point> frame_timestamps;
        static auto last_fps_log_time = std::chrono::steady_clock::now();

        auto current_time = std::chrono::steady_clock::now();
        frame_timestamps.push_back(current_time);

        // 保持窗口大小，移除过旧的时间戳
        while (frame_timestamps.size() > 30) {
            frame_timestamps.pop_front();
        }

        // 每隔1秒记录一次帧率
        if (frame_timestamps.size() >= 2 &&
            std::chrono::duration_cast<std::chrono::seconds>(current_time - last_fps_log_time).count() >= 1) {

            // 计算帧率
            float duration = std::chrono::duration<float>(
                frame_timestamps.back() - frame_timestamps.front()).count();
            float fps = (frame_timestamps.size() - 1) / duration;

            LOG_DEBUG("当前WebSocket帧率: {} FPS", fps);
            last_fps_log_time = current_time;
        }

        // 检查数据是否足够长
        if (message.size() < 10) {
            LOG_WARN("接收到的数据太短，不可能是有效的图像");
            return;
        }

        // 直接使用OpenCV解码数据 - 模仿HTML测试页面的处理方式
        std::vector<uchar> buffer(message.begin(), message.end());
        cv::Mat rawFrame = cv::imdecode(buffer, cv::IMREAD_COLOR);

        if (!rawFrame.empty()) {
            // LOG_DEBUG("成功解码图像，尺寸: " + std::to_string(rawFrame.cols) + "x" + std::to_string(rawFrame.rows));

            QMutexLocker locker(&mutex);
            if (image_buffer_queue.empty()) {
                image_buffer_queue.push(std::move(rawFrame));
            } else {
                image_buffer_queue.pop();
                image_buffer_queue.push(std::move(rawFrame));
            }
        } else {
            // 如果OpenCV解码失败，尝试Qt的方法
            QImage image;
            if (image.loadFromData(message, "JPEG")) {
                LOG_DEBUG("Qt成功解码JPEG图像");
                cv::Mat frame = QImageToCvMat(image);

                if (!frame.empty()) {
                    QMutexLocker locker(&mutex);
                    image_not_receive_count = 0;
                    if (image_buffer_queue.empty()) {
                        image_buffer_queue.push(std::move(frame));
                    } else {
                        image_buffer_queue.pop();
                        image_buffer_queue.push(std::move(frame));
                    }
                }
            } else {
                // 如果Qt也失败，记录数据头部信息
                LOG_WARN("无法解码接收到的图像数据");
                //LOG_DEBUG("数据前16字节: " + bytesToHexString(message.left(16)));

                // 尝试检测数据类型
                if (message.size() > 4) {
                    uint32_t magic = 0;
                    memcpy(&magic, message.data(), 4);
                    LOG_DEBUG("数据前4字节魔数: 0x{}", QString::number(magic, 16).toStdString());
                }
            }
        }

    } catch (const std::exception& e) {
        LOG_ERROR("处理WebSocket消息时出错: {}", e.what());
    }
}

cv::Mat ESP32VideoStream::QImageToCvMat(const QImage &image) const
{
    switch (image.format()) {
    case QImage::Format_RGB888: {
        cv::Mat mat(image.height(), image.width(), CV_8UC3,
                   const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::Mat result;
        cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
        return result;
    }
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied: {
        cv::Mat mat(image.height(), image.width(), CV_8UC4,
                   const_cast<uchar*>(image.bits()), image.bytesPerLine());
        cv::Mat result;
        cv::cvtColor(mat, result, cv::COLOR_RGBA2BGR);
        return result;
    }
    default: {
        // 对于其他格式，转换为RGB888再处理
        QImage converted = image.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat(converted.height(), converted.width(), CV_8UC3,
                   const_cast<uchar*>(converted.bits()), converted.bytesPerLine());
        cv::Mat result;
        cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
        return result;
    }
    }
}