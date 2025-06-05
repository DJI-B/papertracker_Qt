//
// Created by JellyfishKnight on 25-3-10.
//
#include "updater.hpp"
#include <QTimer>
#include <QApplication>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include "logger.hpp"
std::optional<Version> Updater::getClientVersionSync(QWidget* window) {
    LOG_INFO("正在获取远程版本信息...");
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("http://47.116.163.1/version.json"));
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR("获取版本信息失败: {}", reply->errorString().toStdString());
        if (window) {
            QMessageBox::critical(window, "错误", "获取版本信息失败: " + reply->errorString());
        }
        reply->deleteLater();
        return std::nullopt;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    //LOG_INFO("收到服务器响应: {}", QString(responseData).toStdString());

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        LOG_ERROR("版本信息格式错误: 不是有效的JSON对象");
        if (window) {
            QMessageBox::critical(window, "错误", "版本信息格式错误");
        }
        return std::nullopt;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("version") || !obj["version"].isObject()) {
        LOG_ERROR("版本信息格式错误: 缺少版本字段或格式不正确");
        return std::nullopt;
    }

    QJsonObject versionObj = obj["version"].toObject();
    ClientVersion cv;
    cv.tag = versionObj["tag"].toString();
    cv.description = versionObj["description"].toString();
    cv.firmware = versionObj["firmware"].toString();

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
    QString downloadUrl = "http://47.116.163.1:80/downloads/" + remoteVersion.version.tag;

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