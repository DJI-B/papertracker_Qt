//
// Created by JellyfishKnight on 25-3-11.
//

// You may need to build the project (run Qt uic code generator) to get "ui_paper_tracker_main_window.h" resolved

#include "main_window.hpp"

#include <eye_tracker_window.hpp>
#include <face_tracker_window.hpp>
#include <QFile>
#include <QTimer>

#include "ui_main_window.h"


PaperTrackerMainWindow::PaperTrackerMainWindow(QWidget *parent) :
    QWidget(parent) {
    ui.setupUi(this);
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    ui.FaceTrackerInstructionLabel->setText("<a href='https://fcnk6r4c64fa.feishu.cn/wiki/LZdrwWWozi7zffkLt5pc81WanAd'>点击查看面捕说明书</a>");
    ui.EyeTrackerInstructionLabel->setText("<a href='https://fcnk6r4c64fa.feishu.cn/wiki/Dg4qwI3mDiJ3fHk5iZtc2z6Rn47'>点击查看眼追说明书</a>");
    setFixedSize(585, 459);
    // set logo
    QFile Logo = QFile("./resources/logo.png");
    Logo.open(QIODevice::ReadOnly);
    QPixmap pixmap;
    pixmap.loadFromData(Logo.readAll());
    auto final_map = pixmap.scaled(ui.LOGOLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    Logo.close();
    ui.LOGOLabel->setAlignment(Qt::AlignCenter);
    ui.LOGOLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui.LOGOLabel->setScaledContents(true);
    ui.LOGOLabel->setPixmap(final_map);
    ui.LOGOLabel->update();

    updater = std::make_shared<Updater>();
    // 启动VRCFT应用程序
    vrcftProcess = new QProcess(this);
    // 获取应用程序路径
    QString appDir = QCoreApplication::applicationDirPath();
    QString vrcftPath = appDir + "/VRCFaceTracking/VRCFaceTracking.exe";

    // 检查VRCFT可执行文件是否存在
    QFileInfo fileInfo(vrcftPath);
    if (fileInfo.exists()) {
        LOG_INFO("正在启动VRCFT...");
        vrcftProcess->start(vrcftPath);

        // 检查是否成功启动
        if (!vrcftProcess->waitForStarted(3000)) {
            LOG_ERROR("无法启动VRCFT，错误: {}", vrcftProcess->errorString().toStdString());
        } else {
            LOG_INFO("VRCFT启动成功");
        }
    } else {
        LOG_ERROR("未找到VRCFT可执行文件，路径: {}", vrcftPath.toStdString());
    }
    connect_callbacks();

    // 检查客户端版本
    QTimer::singleShot(1000, this, [this] ()
    {
        LOG_INFO("开始检查客户端版本...");
        auto remote_opt = updater->getClientVersionSync(nullptr);
        auto curr_opt = updater->getCurrentVersion();

        if (remote_opt.has_value() && curr_opt.has_value())
        {
            const auto& remote_version = remote_opt.value();
            const auto& curr_version = curr_opt.value();

            LOG_INFO("本地版本: {}, 远程版本: {}",
                    curr_version.version.tag.toStdString(),
                    remote_version.version.tag.toStdString());

            if (curr_version.version.tag != remote_version.version.tag)
            {
                ui.ClientStatusLabel->setText(tr("有新版本可用: ") + remote_version.version.tag);

                // 可选：自动弹出更新提示
                QTimer::singleShot(2000, this, [this, remote_version]() {
                    QMessageBox::StandardButton reply = QMessageBox::question(
                        this, "发现新版本",
                        "检测到新版本 " + remote_version.version.tag +
                        "\n\n是否现在更新？\n\n更新说明：\n" + remote_version.version.description,
                        QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes) {
                        onUpdateButtonClicked();
                    }
                });
            }
            else
            {
                ui.ClientStatusLabel->setText(tr("当前客户端版本为最新版本"));
            }
        }
        else if (!remote_opt.has_value())
        {
            ui.ClientStatusLabel->setText(tr("无法连接到服务器，请检查网络"));
        }
        else if (!curr_opt.has_value())
        {
            ui.ClientStatusLabel->setText(tr("无法获取到当前客户端版本"));
        }
    });
}

PaperTrackerMainWindow::~PaperTrackerMainWindow() {
    // 关闭VRCFT进程
    if (vrcftProcess) {
        if (vrcftProcess->state() == QProcess::Running) {
            LOG_INFO("正在关闭VRCFT...");
            vrcftProcess->terminate();
            if (!vrcftProcess->waitForFinished(3000)) {
                vrcftProcess->kill();
            }
        }
        delete vrcftProcess;
    }
}
void PaperTrackerMainWindow::onFaceTrackerButtonClicked()
{
    try
    {
        auto window = new PaperFaceTrackerWindow();
        window->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
        window->setWindowModality(Qt::NonModal);     // 设置为非模态
        window->setWindowIcon(this->windowIcon());
       // window->setParent(this, Qt::Window);
        window->show();
    } catch (std::exception& e)
    {
        QMessageBox::critical(this, tr("错误"), e.what());
    }
}

void PaperTrackerMainWindow::onEyeTrackerButtonClicked()
{
    try
    {
        auto window = new PaperEyeTrackerWindow();
        window->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
        window->setWindowModality(Qt::NonModal);     // 设置为非模态
        window->setWindowIcon(this->windowIcon());
       // window->setParent(this, Qt::Window);
        window->show();
    } catch (std::exception& e)
    {
        QMessageBox::critical(this, tr("错误"), e.what());
    }
}

void PaperTrackerMainWindow::onUpdateButtonClicked()
{
    // 1. 同步获取远程版本信息
    auto remoteVersionOpt = updater->getClientVersionSync(this);
    if (!remoteVersionOpt.has_value()) {
        return;
    }
    // 2. 获取本地版本信息
    auto currentVersionOpt = updater->getCurrentVersion();
    if (!currentVersionOpt.has_value()) {
        QMessageBox::critical(this, tr("错误"), tr("无法获取当前客户端版本信息"));
        return;
    }
    // 3. 如果版本不同，则执行更新
    if (remoteVersionOpt.value().version.tag != currentVersionOpt.value().version.tag) {
        auto reply = QMessageBox::question(this, tr("版本检查"),
            tr(("当前客户端版本不是最新版本是否更新？\n版本更新信息如下：\n" +
            remoteVersionOpt.value().version.description).toUtf8().constData()),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            if (!updater->updateApplicationSync(this, remoteVersionOpt.value())) {
                // 更新过程中有错误，提示信息已在内部处理
                return;
            }
            // 若更新成功，updateApplicationSync 内部会重启程序并退出当前程序
        }
    } else {
        QMessageBox::information(this, tr("版本检查"), tr("当前客户端版本已是最新版本"));
    }
}

void PaperTrackerMainWindow::connect_callbacks()
{
    connect(ui.FaceTrackerButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onFaceTrackerButtonClicked);
    connect(ui.EyeTrackerButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onEyeTrackerButtonClicked);
    connect(ui.ClientUpdateButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onUpdateButtonClicked);
    connect(ui.RestartVRCFTButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onRestartVRCFTButtonClicked);
}
void PaperTrackerMainWindow::onRestartVRCFTButtonClicked()
{
    // 检查VRCFT进程是否在运行
    if (vrcftProcess && vrcftProcess->state() == QProcess::Running) {
        LOG_INFO("正在重启VRCFT...");

        // 关闭当前进程
        vrcftProcess->terminate();
        if (!vrcftProcess->waitForFinished(3000)) {
            LOG_INFO("VRCFT未正常退出，强制结束进程");
            vrcftProcess->kill();
            vrcftProcess->waitForFinished(1000);
        }

        // 获取应用程序路径
        QString appDir = QCoreApplication::applicationDirPath();
        QString vrcftPath = appDir + "/VRCFaceTracking/VRCFaceTracking.exe";

        // 检查VRCFT可执行文件是否存在
        QFileInfo fileInfo(vrcftPath);
        if (fileInfo.exists()) {
            // 重新启动VRCFT
            vrcftProcess->start(vrcftPath);

            // 检查是否成功启动
            if (!vrcftProcess->waitForStarted(3000)) {
                LOG_ERROR("无法重启VRCFT，错误: {}", vrcftProcess->errorString().toStdString());
                QMessageBox::critical(this, tr("错误"), tr("无法重启VRCFT: ") + vrcftProcess->errorString());
            } else {
                LOG_INFO("VRCFT重启成功");
                QMessageBox::information(this, tr("成功"), tr("VRCFT已成功重启"));
            }
        } else {
            LOG_ERROR("未找到VRCFT可执行文件，路径: {}", vrcftPath.toStdString());
            QMessageBox::critical(this, tr("错误"), tr("未找到VRCFT可执行文件，路径: ") + vrcftPath);
        }
    } else {
        // VRCFT没有运行，直接启动
        QString appDir = QCoreApplication::applicationDirPath();
        QString vrcftPath = appDir + "/VRCFaceTracking/VRCFaceTracking.exe";

        // 检查VRCFT可执行文件是否存在
        QFileInfo fileInfo(vrcftPath);
        if (fileInfo.exists()) {
            LOG_INFO("正在启动VRCFT...");
            vrcftProcess->start(vrcftPath);

            // 检查是否成功启动
            if (!vrcftProcess->waitForStarted(3000)) {
                LOG_ERROR("无法启动VRCFT，错误: {}", vrcftProcess->errorString().toStdString());
                QMessageBox::critical(this, tr("错误"), tr("无法启动VRCFT: ") + vrcftProcess->errorString());
            } else {
                LOG_INFO("VRCFT启动成功");
                QMessageBox::information(this, tr("成功"), tr("VRCFT已成功启动"));
            }
        } else {
            LOG_ERROR("未找到VRCFT可执行文件，路径: {}", vrcftPath.toStdString());
            QMessageBox::critical(this, tr("错误"), tr("未找到VRCFT可执行文件，路径: ") + vrcftPath);
        }
    }
}