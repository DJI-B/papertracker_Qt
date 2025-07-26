//
// Created by JellyfishKnight on 25-3-11.
//

// You may need to build the project (run Qt uic code generator) to get "ui_paper_tracker_main_window.h" resolved

#include "main_window.h"

#include <eye_tracker_window.hpp>
#include <face_tracker_window.hpp>
#include <QFile>
#include <QTimer>
#include <QWidgetAction>

#include "BubbleTipWidget.h"
#include "translator_manager.h"
PaperTrackerMainWindow::PaperTrackerMainWindow(QWidget *parent) :
    QWidget(parent) {
    initUI();
    initLayout();
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    // 修改 initUI() 中的窗口尺寸设置
    setFixedWidth(600); // 保留固定宽度
    setMinimumHeight(0); // 允许窗口高度自由调整
    retranslateUi();
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
                clientStatus = ClientStatus::UpdateAvailable;
                latestVersionTag = remote_version.version.tag;
                updateStatusLabel();

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
                clientStatus = ClientStatus::UpToDate;
                updateStatusLabel();
            }
        }
        else if (!remote_opt.has_value())
        {
            clientStatus = ClientStatus::ServerUnreachable;
            updateStatusLabel();
        }
        else if (!curr_opt.has_value())
        {
            clientStatus = ClientStatus::VersionUnknown;
            updateStatusLabel();
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
        window->show();

        /*// 创建气泡提示并居中显示在面捕窗口上
        BubbleTipWidget *bubble = new BubbleTipWidget(window); // 设置父窗口为面捕窗口

        // 设置标题和内容
        bubble->setTitle("提示信息");
        bubble->setText("这是一个悬浮提示气泡，可以显示重要信息。"
                       "支持多种关闭方式：\n"
                       "1. 点击右上角关闭按钮\n"
                       "2. 设置自动消失时间\n"
                       "3. 启用hover时隐藏");

        // 在面捕窗口中央显示气泡
        QRect windowGeometry = window->geometry();
        int x = windowGeometry.x() + (windowGeometry.width() - bubble->width()) / 2;
        int y = windowGeometry.y() + (windowGeometry.height() - bubble->height()) / 2;
        bubble->setPosition(x, y);

        bubble->showBubble();*/

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
        QMessageBox::critical(this, Translator::tr("错误"), Translator::tr("无法获取当前客户端版本信息"));
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

void PaperTrackerMainWindow::initUI()
{
    // 创建菜单栏
    menuBar = new QMenuBar(this);
    menuBar->setFixedHeight(30); // 增加高度适配大字体
    menuBar->setStyleSheet(
        "QMenuBar { font-size: 14pt; }"                // 设置字体大小
        "QMenuBar::item { padding: 0 12px; }"           // 增加菜单项水平间距
        "QMenu { font-size: 14pt; padding: 4px 0; }"    // 同步调整下拉菜单字体
    );


    settingsMenu = menuBar->addMenu(Translator::tr("设置"));
    settingsMenu->addAction(Translator::tr("基本设置"), this, &PaperTrackerMainWindow::onSettingsButtonClicked);

    helpMenu = menuBar->addMenu(Translator::tr("帮助"));
    helpMenu->addAction(Translator::tr("关于"), this, &PaperTrackerMainWindow::onAboutClicked);

    // 创建更新按钮
    ClientUpdateButton = new QPushButton(Translator::tr("检查更新"));

    // 创建重启按钮
    RestartVRCFTButton = new QPushButton(Translator::tr("重启VRCFT"));


    FaceTrackerButton = new QPushButton(Translator::tr("面捕界面"), this);
    EyeTrackerButton = new QPushButton(Translator::tr("眼追界面"), this);

    // 创建标签
    QFile Logo = QFile(":/resources/resources/logo.png");
    Logo.open(QIODevice::ReadOnly);
    QPixmap pixmap;
    pixmap.loadFromData(Logo.readAll());
    LOGOLabel = new QLabel(this);
    LOGOLabel->setAlignment(Qt::AlignCenter);
    LOGOLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    LOGOLabel->setScaledContents(true);
    auto final_map = pixmap.scaled(QSize(585,230), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    LOGOLabel->setPixmap(final_map);
    LOGOLabel->update();

    ClientStatusLabel = new QLabel(Translator::tr("无法连接到服务器，请检查网络"), this);
    ClientStatusLabel_2 = new QLabel(Translator::tr("Based on Project Babble: https://babble.diy/"), this);

    // 说明书
    FaceTrackerInstructionLabel = new QLabel(this);
    EyeTrackerInstructionLabel = new QLabel(this);

    FaceTrackerInstructionLabel->setWordWrap(true);
    EyeTrackerInstructionLabel->setWordWrap(true);
    FaceTrackerInstructionLabel->setOpenExternalLinks(true);  // 允许打开外部链接
    EyeTrackerInstructionLabel->setOpenExternalLinks(true);   // 允许打开外部链接

    // 设置统一样式表
    QString labelStyle = R"(
        QLabel {
            font-size: 10pt;
            font-weight: bold;
            padding: 5px;
        }
    )";

    FaceTrackerInstructionLabel->setStyleSheet(labelStyle);
    EyeTrackerInstructionLabel->setStyleSheet(labelStyle);

    FaceTrackerInstructionLabel->setText(QString(R"(<a href="%1" style="color: #FFFFFF; text-decoration: underline;">%2</a>)")
    .arg("https://fcnk6r4c64fa.feishu.cn/wiki/LZdrwWWozi7zffkLt5pc81WanAd")
    .arg(Translator::tr("点击查看面捕说明书")));

    EyeTrackerInstructionLabel->setText(QString(R"(<a href="%1" style="color: #FFFFFF; text-decoration: underline;">%2</a>)")
    .arg("https://fcnk6r4c64fa.feishu.cn/wiki/Dg4qwI3mDiJ3fHk5iZtc2z6Rn47")
    .arg(Translator::tr("点击查看眼追说明书")));
}

void PaperTrackerMainWindow::initLayout()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 16);
    mainLayout->setSpacing(0);

    // 菜单栏
    mainLayout->addWidget(menuBar);

    // LOGO 区域
    mainLayout->addWidget(LOGOLabel);
    mainLayout->addSpacing(8);
    LOGOLabel->setFixedHeight(230);

    // 创建按钮容器
    QWidget* toolBarContainer = new QWidget(this);
    QHBoxLayout* toolBarLayout = new QHBoxLayout(toolBarContainer);
    toolBarLayout->setContentsMargins(10, 0, 10, 0);
    toolBarLayout->setSpacing(20); // 按钮组之间留白

    // 面捕按钮组（垂直布局）
    QVBoxLayout* faceGroup = new QVBoxLayout();
    faceGroup->setSpacing(4);
    FaceTrackerButton->setFixedSize(200, 80);
    faceGroup->addWidget(FaceTrackerButton, 0, Qt::AlignHCenter); // 按钮水平居中

    FaceTrackerInstructionLabel->setWordWrap(true);
    FaceTrackerInstructionLabel->setAlignment(Qt::AlignCenter);  // 文字居中
    FaceTrackerInstructionLabel->setFixedWidth(200);             // 保持宽度一致
    faceGroup->addWidget(FaceTrackerInstructionLabel);

    // 眼追按钮组（垂直布局）
    QVBoxLayout* eyeGroup = new QVBoxLayout();
    eyeGroup->setSpacing(4 );
    EyeTrackerButton->setFixedSize(200, 80);
    eyeGroup->addWidget(EyeTrackerButton, 0, Qt::AlignHCenter);

    EyeTrackerInstructionLabel->setWordWrap(true);
    EyeTrackerInstructionLabel->setAlignment(Qt::AlignCenter);
    EyeTrackerInstructionLabel->setFixedWidth(200);
    eyeGroup->addWidget(EyeTrackerInstructionLabel);

    // 将两个按钮组加入水平布局
    toolBarLayout->addLayout(faceGroup);
    toolBarLayout->addLayout(eyeGroup);

    // 添加按钮组容器到主布局
    mainLayout->addWidget(toolBarContainer);
    mainLayout->addSpacing(8);

    // 新增按钮容器
    QWidget* actionContainer = new QWidget(this);
    QHBoxLayout* actionLayout = new QHBoxLayout(actionContainer);
    actionLayout->setContentsMargins(10, 0, 10, 0);
    actionLayout->setSpacing(20);

    // 设置按钮尺寸（与现有按钮保持比例）
    ClientUpdateButton->setFixedSize(200, 40);
    RestartVRCFTButton->setFixedSize(200, 40);

    // 添加到布局
    actionLayout->addWidget(ClientUpdateButton);
    actionLayout->addWidget(RestartVRCFTButton);

    // 将新容器添加到主布局中
    mainLayout->addWidget(actionContainer);
    mainLayout->addSpacing(8);  // 保持与后续元素的间距

    // 状态标签
    QVBoxLayout* statusLayout = new QVBoxLayout();
    ClientStatusLabel->setFixedHeight(20);
    ClientStatusLabel_2->setFixedHeight(20);
    statusLayout->addWidget(ClientStatusLabel, 0, Qt::AlignCenter);
    statusLayout->addWidget(ClientStatusLabel_2, 0, Qt::AlignCenter);
    mainLayout->addLayout(statusLayout);

    adjustSize();
}

void PaperTrackerMainWindow::connect_callbacks()
{
    connect(FaceTrackerButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onFaceTrackerButtonClicked);
    connect(EyeTrackerButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onEyeTrackerButtonClicked);
    connect(ClientUpdateButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onUpdateButtonClicked);
    connect(RestartVRCFTButton, &QPushButton::clicked, this, &PaperTrackerMainWindow::onRestartVRCFTButtonClicked);
}

void PaperTrackerMainWindow::retranslateUi()
{
    updateStatusLabel();

    FaceTrackerButton->setText(Translator::tr("面捕界面"));
    EyeTrackerButton->setText(Translator::tr("眼追界面"));
    ClientUpdateButton->setText(Translator::tr("客户端更新检查"));
    RestartVRCFTButton->setText(Translator::tr("重启VRCFT"));

    // 新增菜单栏翻译
    settingsMenu->setTitle(Translator::tr("设置"));
    helpMenu->setTitle(Translator::tr("帮助"));
    settingsMenu->actions()[0]->setText(Translator::tr("基本设置"));
    helpMenu->actions()[0]->setText(Translator::tr("关于"));


    FaceTrackerInstructionLabel->setText(QString(R"(<a href="%1" style="color: #FFFFFF; text-decoration: underline;">%2</a>)")
    .arg("https://fcnk6r4c64fa.feishu.cn/wiki/LZdrwWWozi7zffkLt5pc81WanAd")
    .arg(Translator::tr("点击查看面捕说明书")));

    EyeTrackerInstructionLabel->setText(QString(R"(<a href="%1" style="color: #FFFFFF; text-decoration: underline;">%2</a>)")
    .arg("https://fcnk6r4c64fa.feishu.cn/wiki/Dg4qwI3mDiJ3fHk5iZtc2z6Rn47")
    .arg(Translator::tr("点击查看眼追说明书")));

    adjustSize();
}

void PaperTrackerMainWindow::updateStatusLabel()
{
    switch (clientStatus) {
        case ClientStatus::UpdateAvailable:
            ClientStatusLabel->setText(Translator::tr("有新版本可用: ") + latestVersionTag);
            break;

        case ClientStatus::UpToDate:
            ClientStatusLabel->setText(Translator::tr("当前客户端版本为最新版本"));
            break;

        case ClientStatus::ServerUnreachable:
            ClientStatusLabel->setText(Translator::tr("无法连接到服务器，请检查网络"));
            break;

        case ClientStatus::VersionUnknown:
            ClientStatusLabel->setText(Translator::tr("无法获取到当前客户端版本"));
            break;

        default:
            ClientStatusLabel->setText(Translator::tr("未知状态"));
    }
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
                QMessageBox::critical(this, Translator::tr("错误"), Translator::tr("无法重启VRCFT: ") + vrcftProcess->errorString());
            } else {
                LOG_INFO("VRCFT重启成功");
                QMessageBox::information(this, Translator::tr("成功"), Translator::tr("VRCFT已成功重启"));
            }
        } else {
            LOG_ERROR("未找到VRCFT可执行文件，路径: {}", vrcftPath.toStdString());
            QMessageBox::critical(this, Translator::tr("错误"), Translator::tr("未找到VRCFT可执行文件，路径: ") + vrcftPath);
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
                QMessageBox::critical(this, Translator::tr("错误"), Translator::tr("无法启动VRCFT: ") + vrcftProcess->errorString());
            } else {
                LOG_INFO("VRCFT启动成功");
                QMessageBox::information(this, Translator::tr("成功"), Translator::tr("VRCFT已成功启动"));
            }
        } else {
            LOG_ERROR("未找到VRCFT可执行文件，路径: {}", vrcftPath.toStdString());
            QMessageBox::critical(this, Translator::tr("错误"), Translator::tr("未找到VRCFT可执行文件，路径: ") + vrcftPath);
        }
    }
}

void PaperTrackerMainWindow::onSettingsButtonClicked()
{
    LanguageThemeDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // 语言或主题已更改，更新界面
        retranslateUi();
    }
}

void PaperTrackerMainWindow::onAboutClicked()
{
    auto versionStr = Translator::tr("未知版本");
    auto currentVersionOpt = updater->getCurrentVersion();
    if (currentVersionOpt.has_value()) {
        versionStr =  QString::fromStdString(updater->getCurrentVersion().value().version.tag.toStdString());
    }

    QMessageBox::about(this, Translator::tr("关于"), Translator::tr("PaperTracker %1").arg(versionStr));
}