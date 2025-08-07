#include "main_window_refactored.h"
#include "components/title_bar_widget.h"
#include "components/sidebar_widget.h"
#include "components/wifi_setup_widget.h"
#include "components/guide_widget.h"
#include "components/face_config_widget.h"
#include "managers/device_manager.h"
#include "serial.hpp"

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTime>
#include <QProgressDialog>
#include <QMetaObject>
#include <QFile>

// 设备版本定义
#define FACE_VERSION 1
#define LEFT_VERSION 2
#define RIGHT_VERSION 3

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName("MainPage");
    setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 加载样式表
    loadStyleSheet();

    setupSerialManager();
    setupUI();
}

MainWindow::~MainWindow()
{
    // Qt对象会自动清理子对象
}

void MainWindow::loadStyleSheet()
{
    QFile file(":/resources/resources/styles/light.qss");
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        this->setStyleSheet(styleSheet);
        file.close();
    }
}

void MainWindow::setupSerialManager()
{
    m_serialManager = std::make_shared<SerialPortManager>(this);
    m_serialManager->init();
    // 注册设备状态回调
    m_serialManager->registerCallback(
        PACKET_DEVICE_STATUS,
        [this](const std::string& ip, int brightness, int power, int version) {
            QString deviceType;
            std::string portName = m_serialManager->getCurrentPortName();
            QString deviceName = QString("设备 (%1)").arg(QString::fromStdString(portName));

            switch (version) {
                case FACE_VERSION:
                    deviceType = "Face Tracker";
                    break;
                case LEFT_VERSION:
                    deviceType = "Eye Tracker";
                    break;
                case RIGHT_VERSION:
                    deviceType = "Eye Tracker";
                    break;
                default:
                    deviceType = "Unknown Device";
            }
            
            // 主线程更新 UI
            QMetaObject::invokeMethod(this, [=]() {
                if (m_deviceManager && m_deviceManager->hasDevice(deviceName)) {
                    // 设备已存在，更新设备类型和状态
                    m_deviceManager->updateDeviceType(deviceName, deviceType);
                    m_deviceManager->updateDeviceStatus(deviceName, QString::fromStdString(ip), power);
                    // 更新侧边栏中的设备类型显示
                    m_sidebar->updateDeviceType(deviceName, deviceType);
                } else {
                    // 设备不存在，直接添加
                    onDeviceConnected(deviceName, deviceType);
                    if (m_deviceManager) {
                        m_deviceManager->updateDeviceStatus(deviceName, QString::fromStdString(ip), power);
                    }
                }

            }, Qt::QueuedConnection);
        }
    );

    // 注册WiFi错误回调
    m_serialManager->registerCallback(
        PACKET_WIFI_ERROR,
        [this](const std::string& wifiName, const std::string& wifiPassword) {
            QString qWifiName = QString::fromStdString(wifiName);
            QString qWifiPassword = QString::fromStdString(wifiPassword);
            if (qWifiName.isEmpty())
            {
                LOG_INFO("(网络连接中): 当前WIFI为 {}, 密码为 {}, 如果长时间连接失败，请检查是否有误", wifiName, wifiPassword);
            }
            
            // 获取当前串口作为设备标识
            std::string portName = m_serialManager->getCurrentPortName();
            QString deviceName = QString("设备 (%1)").arg(QString::fromStdString(portName));
            QString deviceType = "WiFi配置中"; // 临时设备类型
            
            QMetaObject::invokeMethod(this, [=]() {
                // 检查设备是否已经添加过
                if (!m_deviceManager || !m_deviceManager->hasDevice(deviceName)) {
                    // 添加设备到侧边栏和设备管理器，但标记为WiFi配置状态
                    onDeviceConnected(deviceName, deviceType);
                }
                // 显示WiFi配置请求
                onWifiConfigRequest(qWifiName, qWifiPassword);
            }, Qt::QueuedConnection);
        }
    );

    // 创建设备管理器
    m_deviceManager = new DeviceManager(m_serialManager, this);
    // 连接设备管理器信号
    connect(m_deviceManager, &DeviceManager::deviceAdded,
            this, &MainWindow::onDeviceConnected);
    connect(m_deviceManager, &DeviceManager::deviceRemoved,
            this, &MainWindow::onDeviceDisconnected);
}

void MainWindow::setupUI()
{
    setFixedSize(1300, 700);

    // 创建阴影框架
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setColor(QColor(127, 127, 127, 127));
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);

    m_centralFrame = new QFrame(this);
    m_centralFrame->setGeometry(10, 10, width() - 20, height() - 20);
    m_centralFrame->setGraphicsEffect(shadowEffect);
    m_centralFrame->setObjectName("centralWidget");
    m_centralFrame->setStyleSheet(
        "QFrame#centralWidget {"
        "    background-color: #ffffff;"
        "    border-radius: 12px;"
        "}"
    );

    // 创建标题栏
    m_titleBar = new TitleBarWidget(m_centralFrame);
    m_titleBar->setTitle("PaperTracker");
    
    // 连接标题栏信号
    connect(m_titleBar, &TitleBarWidget::minimizeRequested, this, &MainWindow::onMinimizeRequested);
    connect(m_titleBar, &TitleBarWidget::closeRequested, this, &MainWindow::onCloseRequested);
    connect(m_titleBar, &TitleBarWidget::menuRequested, this, &MainWindow::onMenuRequested);
    connect(m_titleBar, &TitleBarWidget::notificationRequested, this, &MainWindow::onNotificationRequested);

    // 为标题栏安装事件过滤器处理拖动
    m_titleBar->installEventFilter(this);

    // 创建主布局（不包含标题栏）
    m_mainLayout = new QHBoxLayout(m_centralFrame);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setMenuBar(m_titleBar);

    // 创建侧边栏
    m_sidebar = new SidebarWidget();

    // 连接侧边栏信号
    connect(m_sidebar, &SidebarWidget::itemClicked, this, &MainWindow::onSidebarItemClicked);
    connect(m_sidebar, &SidebarWidget::deviceTabClicked, this, &MainWindow::onDeviceTabClicked);

    // 创建主内容区域
    m_contentStack = new QStackedWidget(m_centralFrame);
    m_contentStack->setObjectName("contentStack");

    // 添加到主布局
    m_mainLayout->addWidget(m_sidebar);
    m_mainLayout->addWidget(m_contentStack);

    // 设置侧边栏拉伸因子，确保内容区域占据剩余空间
    m_mainLayout->setStretchFactor(m_sidebar, 0);  // 侧边栏固定宽度
    m_mainLayout->setStretchFactor(m_contentStack, 1);  // 内容区域可伸缩

    // 创建内容页面
    createContentPages();
}

void MainWindow::createContentPages()
{
    // 创建默认内容
    createDefaultContent();

    // 创建WiFi配置页面
    m_wifiConfigWidget = new WiFiSetupWidget("", m_serialManager);
    m_wifiConfigWidget->setObjectName("contentWidget");
    connect(m_wifiConfigWidget, &WiFiSetupWidget::configurationSuccess,
            this, &MainWindow::onWiFiConfigurationSuccess);

    // 创建引导界面
    m_faceGuideWidget = new GuideWidget("Face Tracker Setup", 3);
    m_faceGuideWidget->setObjectName("contentWidget");
    m_eyeGuideWidget = new GuideWidget("Eye Tracker Setup", 4);
    m_eyeGuideWidget->setObjectName("contentWidget");

    // 为引导界面添加步骤内容
    for (int i = 1; i <= 3; ++i) {
        QWidget *stepContent = new QWidget();
        QVBoxLayout *stepLayout = new QVBoxLayout(stepContent);
        
        QLabel *stepLabel = new QLabel(QString("Face Tracker Setup - Step %1").arg(i));
        stepLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #333; margin: 20px; }");
        stepLabel->setAlignment(Qt::AlignCenter);
        
        QLabel *descLabel = new QLabel(QString("Configure your face tracker settings in step %1").arg(i));
        descLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin: 10px; }");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);
        
        stepLayout->addWidget(stepLabel);
        stepLayout->addWidget(descLabel);
        stepLayout->addStretch();
        
        m_faceGuideWidget->setStepContent(i, stepContent);
    }

    for (int i = 1; i <= 4; ++i) {
        QWidget *stepContent = new QWidget();
        QVBoxLayout *stepLayout = new QVBoxLayout(stepContent);
        
        QLabel *stepLabel = new QLabel(QString("Eye Tracker Setup - Step %1").arg(i));
        stepLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #333; margin: 20px; }");
        stepLabel->setAlignment(Qt::AlignCenter);
        
        QLabel *descLabel = new QLabel(QString("Configure your eye tracker settings in step %1").arg(i));
        descLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin: 10px; }");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);
        
        stepLayout->addWidget(stepLabel);
        stepLayout->addWidget(descLabel);
        stepLayout->addStretch();
        
        m_eyeGuideWidget->setStepContent(i, stepContent);
    }

    // 添加到内容栈
    m_contentStack->addWidget(m_defaultContentWidget);
    m_contentStack->addWidget(m_wifiConfigWidget);
    m_contentStack->addWidget(m_faceGuideWidget);
    m_contentStack->addWidget(m_eyeGuideWidget);

    // 默认显示默认内容
    m_contentStack->setCurrentWidget(m_defaultContentWidget);
}

void MainWindow::createDefaultContent()
{
    m_defaultContentWidget = new QWidget();
    m_defaultContentWidget->setObjectName("contentWidget");

    QVBoxLayout *layout = new QVBoxLayout(m_defaultContentWidget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(30);
    layout->setContentsMargins(50, 50, 50, 50);

    // 欢迎标题
    QLabel *welcomeLabel = new QLabel("Welcome to PaperTracker");
    welcomeLabel->setObjectName("welcomeLabel");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(welcomeLabel);

    // 描述文本
    QLabel *descLabel = new QLabel("您的追踪设备管理中心");
    descLabel->setObjectName("descLabel");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    // 添加说明文本
    QLabel *instructionLabel = new QLabel(
        "🔌 请通过USB连接您的追踪设备\n\n"
        "📱 设备连接后将自动识别并出现在左侧边栏中\n\n"
        "⚙️ 点击左侧设备标签页即可进入相应的配置界面\n\n"
        "🌐 支持面部追踪和眼部追踪设备的自动配置"
    );
    instructionLabel->setObjectName("instructionLabel");
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setWordWrap(true);
    layout->addWidget(instructionLabel);

    layout->addStretch();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 处理标题栏拖动
    if (obj == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_dragPosition = mouseEvent->globalPos() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                move(mouseEvent->globalPos() - m_dragPosition);
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    QMainWindow::mouseMoveEvent(event);
}

// 标题栏事件处理
void MainWindow::onMinimizeRequested()
{
    showMinimized();
}

void MainWindow::onCloseRequested()
{
    close();
}

void MainWindow::onMenuRequested()
{
    // 菜单显示逻辑已在TitleBarWidget中处理
}

void MainWindow::onNotificationRequested()
{
    // TODO: 实现通知功能
}

// 侧边栏事件处理
void MainWindow::onSidebarItemClicked(const QString &itemText)
{
    if (itemText == "Add Device") {
        m_contentStack->setCurrentWidget(m_defaultContentWidget);
    }
}

void MainWindow::onDeviceTabClicked(const QString &deviceName)
{
    if (m_deviceManager) {
        QString deviceType = m_deviceManager->getDeviceType(deviceName);
        
        // 如果是WiFi配置中的设备，跳转到WiFi配置页面
        if (deviceType == "WiFi配置中") {
            m_contentStack->setCurrentWidget(m_wifiConfigWidget);
            return;
        }
        
        // 根据设备类型跳转到相应的配置页面
        if (deviceType == "Face Tracker") {
            // 跳转到面部追踪配置页面
            m_contentStack->setCurrentWidget(m_faceGuideWidget);
        } else if (deviceType == "Eye Tracker") {
            // 跳转到眼部追踪配置页面
            m_contentStack->setCurrentWidget(m_eyeGuideWidget);
        } else {
            // 其他设备类型，使用设备管理器的配置页面
            QWidget *devicePage = m_deviceManager->getDeviceConfigPage(deviceName);
            if (devicePage) {
                // 如果设备页面不在栈中，添加它
                if (m_contentStack->indexOf(devicePage) == -1) {
                    m_contentStack->addWidget(devicePage);
                }
                m_contentStack->setCurrentWidget(devicePage);
            }
        }
    }
}

// 设备管理事件处理
void MainWindow::onDeviceConnected(const QString &deviceName, const QString &deviceType)
{
    // 添加设备到侧边栏
    m_sidebar->addDeviceTab(deviceName, deviceType);
    
    // 添加设备到管理器
    if (m_deviceManager) {
        m_deviceManager->addDevice(deviceName, deviceType);
    }
}

void MainWindow::onDeviceDisconnected(const QString &deviceName)
{
    // 从侧边栏移除设备
    m_sidebar->removeDeviceTab(deviceName);
    
    // 从管理器移除设备
    if (m_deviceManager) {
        m_deviceManager->removeDevice(deviceName);
    }
}

// WiFi配置事件处理
void MainWindow::onWiFiConfigurationSuccess(const QString &deviceType, const QString &wifiName)
{
    QMessageBox::information(this, tr("WiFi配置完成"),
        tr("WiFi配置已完成。\n网络: %1\n\n设备将自动连接到WiFi网络，连接成功后会自动出现在设备列表中。")
        .arg(wifiName));
    
    m_contentStack->setCurrentWidget(m_defaultContentWidget);
}

void MainWindow::onWifiConfigRequest(const QString &wifiName, const QString &wifiPassword)
{
    static QString lastWifiName;
    static QDateTime lastRequestTime;
    QDateTime currentTime = QDateTime::currentDateTime();

    // 防止重复处理
    if (wifiName == lastWifiName) {
        return;
    }

    lastWifiName = wifiName;
    lastRequestTime = currentTime;

    // 检查是否已经在WiFi配置页面
    if (m_contentStack->currentWidget() == m_wifiConfigWidget) {
        m_wifiConfigWidget->updateWifiInfo(wifiName, wifiPassword);
        return;
    }

    showWifiConfigPrompt(wifiName, wifiPassword);
}

void MainWindow::showWifiConfigPrompt(const QString &wifiName, const QString &wifiPassword)
{
    QString promptMessage;
    if (!wifiName.isEmpty()) {
        promptMessage = tr("检测到设备WiFi配置请求！\n"
                          "设备尝试连接: %1\n"
                          "该设备需要配置WiFi网络。\n"
                          "是否立即进行WiFi配置？").arg(wifiName);
    } else {
        promptMessage = tr("检测到设备WiFi配置请求！\n"
                          "该设备需要配置WiFi网络。\n"
                          "是否立即进行WiFi配置？");
    }

    int result = QMessageBox::question(this, tr("发现设备"),
        promptMessage,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        showWifiConfigPage(wifiName, wifiPassword);
    }
}

void MainWindow::showWifiConfigPage(const QString &wifiName, const QString &wifiPassword)
{
    m_wifiConfigWidget->updateWifiInfo(wifiName, wifiPassword);
    m_contentStack->setCurrentWidget(m_wifiConfigWidget);
}