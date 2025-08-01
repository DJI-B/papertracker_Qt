//
// Created by JellyfishKnight on 25-3-10.
//
#include "updater.hpp"
#include <QTimer>
#include <QApplication>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include "logger.hpp"
#include "translator_manager.h"
#include <QJsonArray>

std::optional<Version> Updater::getClientVersionSync(QWidget* window) {
    LOG_INFO("正在获取远程版本信息...");

    // 根据当前语言选择不同的版本信息URL
    auto currentLang = TranslatorManager::instance().getCurrentLanguage();
    QString versionUrl = "";
    if (currentLang == "zh_CN")
    {
        versionUrl = "http://47.116.163.1:8443/packages/PaperTracker_Setup/versions";
    }
    else
    {
        versionUrl = "http://en.download.papertracker.top/packages/PaperTracker_Setup/versions";
    }

    QNetworkAccessManager manager;
    QUrl URL = QUrl(versionUrl);
    QNetworkRequest request(URL);
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR("获取版本信息失败: {}", reply->errorString().toStdString());
        if (window) {
            QMessageBox::critical(window, Translator::tr("错误"), Translator::tr("获取版本信息失败: ") + reply->errorString());
        }
        reply->deleteLater();
        return std::nullopt;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    LOG_INFO("收到服务器响应: {}", QString(responseData).toStdString());

    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    // 解析数组格式的版本信息
    if (!doc.isArray()) {
        LOG_ERROR("版本信息格式错误: 不是有效的JSON数组");
        if (window) {
            QMessageBox::critical(window, Translator::tr("错误"), Translator::tr("版本信息格式错误"));
        }
        return std::nullopt;
    }

    QJsonArray versionsArray = doc.array();

    // 查找最新的版本（is_latest为true的版本）
    QJsonObject latestVersionObj;
    bool foundLatest = false;

    for (const auto& value : versionsArray) {
        if (value.isObject()) {
            QJsonObject versionObj = value.toObject();
            if (versionObj.contains("is_latest") && versionObj["is_latest"].toBool()) {
                latestVersionObj = versionObj;
                foundLatest = true;
                break;
            }
        }
    }

    // 如果没有找到标记为最新的版本，则使用第一个版本
    if (!foundLatest && !versionsArray.isEmpty()) {
        QJsonValue firstValue = versionsArray.first();
        if (firstValue.isObject()) {
            latestVersionObj = firstValue.toObject();
            foundLatest = true;
        }
    }

    if (!foundLatest) {
        LOG_ERROR("版本信息格式错误: 未找到有效的版本信息");
        if (window) {
            QMessageBox::critical(window, Translator::tr("错误"), Translator::tr("版本信息格式错误: 未找到有效的版本信息"));
        }
        return std::nullopt;
    }

    // 提取版本信息
    ClientVersion cv;
    cv.tag = latestVersionObj["version"].toString();
    cv.description = latestVersionObj["description"].toString();
    cv.firmware = ""; // 新的API格式中没有firmware字段

    LOG_INFO("成功获取远程版本: {}", cv.tag.toStdString());
    return Version{cv};
}


// 同步读取本地版本信息
std::optional<Version> Updater::getCurrentVersion() {
    //LOG_INFO("正在读取本地版本信息...");
    QFile file("version.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_ERROR("无法打开文件: version.json");
        return std::nullopt;
    }
    QByteArray fileData = file.readAll();
    file.close();

    //LOG_INFO("本地版本文件内容: {}", QString(fileData).toStdString());

    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (!doc.isObject()) {
        LOG_ERROR("本地版本JSON格式错误");
        return std::nullopt;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("version") || !obj["version"].isObject()) {
        LOG_ERROR("本地版本信息格式错误: 缺少version字段或格式不正确");
        return std::nullopt;
    }

    QJsonObject versionObj = obj["version"].toObject();
    ClientVersion cv;
    cv.tag = versionObj["tag"].toString();
    cv.description = versionObj["description"].toString();
    cv.firmware = versionObj["firmware"].toString();

    LOG_INFO("成功读取本地版本: {}", cv.tag.toStdString());
    return Version{cv};
}
// 同步下载 zip 更新包，并返回下载是否成功
bool Updater::downloadZipSync(QWidget* window, const QString &zipFilePath, const QString &downloadUrl) {
    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(downloadUrl)};
    QNetworkReply* reply = manager.get(request);

    QProgressDialog progress("正在下载更新包，如果速度过慢请到群文件下载，请稍候...", "取消", 0, 100, window);
    progress.setWindowTitle("下载更新");
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    connect(reply, &QNetworkReply::downloadProgress, [&](qint64 bytesReceived, qint64 bytesTotal) {
        if (bytesTotal > 0) {
            progress.setMaximum(bytesTotal);
            progress.setValue(bytesReceived);
        }
        QCoreApplication::processEvents();
    });
    // 如果用户取消，则中止下载
    connect(&progress, &QProgressDialog::canceled, [&]() {
        reply->abort();
    });

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(window, "错误", "下载更新包失败，请到群文件直接下载: " + reply->errorString());
        reply->deleteLater();
        return false;
    }

    QFile file(zipFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(window, "错误", "无法创建文件: " + zipFilePath);
        reply->deleteLater();
        return false;
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();
    return true;
}

// 解压 zip 文件到目标目录（使用 QuaZip 或其他方式实现）
// 下面给出使用 QuaZip 的示例伪代码，实际使用时需要确保引入并配置好 QuaZip 库
// 添加运行安装程序的函数实现
bool Updater::runInstaller(const QString &installerPath, QWidget* parent) {
    QMessageBox confirmBox;
    confirmBox.setWindowTitle("确认安装更新");
    confirmBox.setText("已下载安装程序，准备进行安装。");
    confirmBox.setInformativeText("点击确定开始安装，这将关闭当前应用程序。");
    confirmBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    confirmBox.setDefaultButton(QMessageBox::Ok);

    if (confirmBox.exec() == QMessageBox::Cancel) {
        return false;
    }

    // 使用QProcess启动安装程序
    QProcess::startDetached(installerPath);

    // 安装程序启动后退出当前应用
    QApplication::quit();
    return true;
}
// 更新程序：下载 zip 包，解压覆盖文件，关闭当前程序并重启应用
// 修改更新函数实现
bool Updater::updateApplicationSync(QWidget* window, const Version &remoteVersion) {
    // 假设服务器提供的安装包 URL 为：<下载地址>/<版本tag>.exe
    auto currentLang = TranslatorManager::instance().getCurrentLanguage();
    QString downloadUrl = "";
    if (currentLang == "zh_CN")
    {
        downloadUrl = "http://47.116.163.1:8443/downloads/PaperTracker_Setup/latest";
    }
    else
    {
        downloadUrl = "http://en.download.papertracker.top/downloads/PaperTracker_Setup/latest";
    }
    // 保存安装程序到临时目录，避免权限问题
    QString tempDir = QDir::tempPath();
    QString installerPath = QDir(tempDir).filePath("PaperTracker_" + remoteVersion.version.tag + "_Setup.exe");

    // 同步下载安装程序
    if (!downloadZipSync(window, installerPath, downloadUrl)) {
        return false;
    }

    QMessageBox::information(window, "提示", "安装包下载成功，点击确定开始安装");

    // 运行安装程序
    return runInstaller(installerPath, window);
}
// 1. 首先在 updater.hpp 中添加版本比较函数声明
#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <iostream>
#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QProgressDialog>
#include <QProcess>
#include <QEventLoop>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>
#include <optional>

// 定义版本相关结构体
struct ClientVersion {
    QString tag;
    QString description;
    QString firmware;
};

struct Version {
    ClientVersion version;
};

class Updater : public QObject {
public:
    explicit Updater(QObject *parent = nullptr) : QObject(parent) {}

    // 现有函数声明...
    std::optional<Version> getClientVersionSync(QWidget* window);
    std::optional<Version> getCurrentVersion();
    bool downloadZipSync(QWidget* window, const QString &zipFilePath, const QString &downloadUrl);
    bool runInstaller(const QString &installerPath, QWidget* parent);
    bool updateApplicationSync(QWidget* window, const Version &remoteVersion);

    // 新增版本比较函数
    int compareVersions(const QString& version1, const QString& version2);
    bool isVersionNewer(const QString& currentVersion, const QString& serverVersion);
    void showBetaVersionDialog(QWidget* parent, const QString& currentVersion, const QString& serverVersion);
};

#endif // UPDATER_HPP

// 2. 在 updater.cpp 中实现版本比较功能
// 添加版本比较函数实现
int Updater::compareVersions(const QString& version1, const QString& version2) {
    // 移除版本号中的 'v' 前缀（如果存在）
    QString v1 = version1.startsWith('v') ? version1.mid(1) : version1;
    QString v2 = version2.startsWith('v') ? version2.mid(1) : version2;

    // 分割版本号
    QStringList parts1 = v1.split('.');
    QStringList parts2 = v2.split('.');

    // 确保两个版本号部分数量相同，不足的用0补齐
    int maxParts = qMax(parts1.size(), parts2.size());
    while (parts1.size() < maxParts) parts1.append("0");
    while (parts2.size() < maxParts) parts2.append("0");

    // 逐个比较版本号部分
    for (int i = 0; i < maxParts; ++i) {
        // 提取数字部分进行比较
        QRegularExpression numRegex("(\\d+)");
        QRegularExpressionMatch match1 = numRegex.match(parts1[i]);
        QRegularExpressionMatch match2 = numRegex.match(parts2[i]);

        int num1 = match1.hasMatch() ? match1.captured(1).toInt() : 0;
        int num2 = match2.hasMatch() ? match2.captured(1).toInt() : 0;

        if (num1 < num2) return -1;
        if (num1 > num2) return 1;

        // 如果数字部分相同，再比较字符串部分（用于处理如 "1.0.0-beta" 这样的版本）
        QString suffix1 = parts1[i].mid(match1.capturedLength());
        QString suffix2 = parts2[i].mid(match2.capturedLength());

        if (suffix1 != suffix2) {
            // beta 版本通常小于正式版本
            if (suffix1.contains("beta") && !suffix2.contains("beta")) return 1;
            if (!suffix1.contains("beta") && suffix2.contains("beta")) return -1;
            return suffix1.compare(suffix2);
        }
    }

    return 0; // 版本相同
}

bool Updater::isVersionNewer(const QString& currentVersion, const QString& serverVersion) {
    return compareVersions(currentVersion, serverVersion) > 0;
}

void Updater::showBetaVersionDialog(QWidget* parent, const QString& currentVersion, const QString& serverVersion) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle("测试版本提示");
    msgBox.setIcon(QMessageBox::Information);

    // 设置主要文本
    msgBox.setText("当前运行的是测试版本");

    // 设置详细信息
    QString detailText = QString(
        "当前版本: %1\n"
        "服务器版本: %2\n\n"
        "您正在使用的版本高于服务器上的稳定版本。\n"
        "测试版本可能包含未经完全测试的功能，\n"
        "如遇到问题请及时反馈。\n\n"
        "建议在生产环境中使用稳定版本。"
    ).arg(currentVersion, serverVersion);

    msgBox.setInformativeText(detailText);

    // 添加按钮
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);

    // 设置样式（可选，让对话框更醒目）
    msgBox.setStyleSheet(
        "QMessageBox {"
        "    background-color: #fffbf0;"
        "}"
        "QMessageBox QLabel {"
        "    color: #8b4513;"
        "    font-weight: bold;"
        "}"
    );

    msgBox.exec();
}