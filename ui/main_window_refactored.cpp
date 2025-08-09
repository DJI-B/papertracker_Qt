#include "main_window_refactored.h"
#include "components/title_bar_widget.h"
#include "components/sidebar_widget.h"
#include "components/wifi_setup_widget.h"
#include "components/guide_widget.h"
#include "components/face_config_widget.h"
#include "managers/device_manager.h"
#include "serial.hpp"
#include "roi_event.hpp"

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
#include <QScrollArea>

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
                    m_currentEyeVersion = LEFT_VERSION;
                    break;
                case RIGHT_VERSION:
                    deviceType = "Eye Tracker";
                    m_currentEyeVersion = RIGHT_VERSION;
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
                    
                    // 检查是否是等待WiFi配置完成的设备
                    if (m_pendingWifiDevices.contains(deviceName)) {
                        // 设备WiFi连接成功，切换到相应的配置页面
                        QString originalDeviceType = m_pendingWifiDevices.value(deviceName);
                        m_pendingWifiDevices.remove(deviceName);
                        
                        // 根据设备类型跳转到相应的配置页面
                        if (deviceType == "Face Tracker") {
                            m_contentStack->setCurrentWidget(m_faceGuideWidget);
                        } else if (deviceType == "Eye Tracker") {
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
        stepLayout->setContentsMargins(20, 20, 20, 20);
        stepLayout->setSpacing(15);
        
        if (i == 1) {
            // 第一步：ROI选择
            // 创建滚动区域
            m_eyeGuideScrollArea = new QScrollArea(stepContent);
            m_eyeGuideScrollArea->setWidgetResizable(true);
            m_eyeGuideScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_eyeGuideScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            m_eyeGuideScrollArea->setFrameStyle(QFrame::NoFrame);
            
            // 创建滚动内容区域
            QWidget *scrollContent = new QWidget();
            QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
            scrollLayout->setContentsMargins(20, 20, 20, 20);
            scrollLayout->setSpacing(15);
            
            QLabel *stepLabel = new QLabel("步骤 1：眼球区域选择 (ROI)");
            stepLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #333; margin: 10px 0; }");
            stepLabel->setAlignment(Qt::AlignCenter);
            
            QLabel *descLabel = new QLabel("请在下方的图像预览区域中，用鼠标框选出眼球的区域。\n这将帮助系统更准确地追踪您的眼部运动。");
            descLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin: 10px 0; line-height: 1.5; }");
            descLabel->setAlignment(Qt::AlignCenter);
            descLabel->setWordWrap(true);
            
            // 创建图像预览区域
            QWidget *previewArea = new QWidget();
            QVBoxLayout *previewLayout = new QVBoxLayout(previewArea);
            previewLayout->setContentsMargins(0, 10, 0, 10);
            previewLayout->setSpacing(15);
            previewLayout->setAlignment(Qt::AlignCenter);
            
            // 眼部预览标题（动态显示左眼或右眼）
            QLabel *eyeTitle = new QLabel("眼部预览");
            eyeTitle->setObjectName("eyePreviewTitle");
            eyeTitle->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #444; }");
            eyeTitle->setAlignment(Qt::AlignCenter);
            
            // 眼部预览图像
            m_eyePreviewLabel = new QLabel();
            m_eyePreviewLabel->setFixedSize(280, 280);
            m_eyePreviewLabel->setAlignment(Qt::AlignCenter);
            m_eyePreviewLabel->setStyleSheet(
                "QLabel {"
                "    border: 2px solid #0070f9;"
                "    border-radius: 8px;"
                "    background-color: #f8f9fa;"
                "    color: #666;"
                "    font-size: 12px;"
                "}"
            );
            m_eyePreviewLabel->setText("等待设备连接...\n\n点击并拖拽鼠标\n框选眼球区域");
            
            previewLayout->addWidget(eyeTitle);
            previewLayout->addWidget(m_eyePreviewLabel, 0, Qt::AlignCenter);
            
            // 设备信息显示
            QLabel *deviceInfoLabel = new QLabel("设备类型：等待检测...");
            deviceInfoLabel->setObjectName("deviceInfoLabel");
            deviceInfoLabel->setStyleSheet("QLabel { font-size: 13px; color: #888; font-style: italic; }");
            deviceInfoLabel->setAlignment(Qt::AlignCenter);
            previewLayout->addWidget(deviceInfoLabel);
            
            // 操作提示
            QLabel *instructionLabel = new QLabel(
                "📝 操作说明：\n"
                "• 设备连接后，将显示实时图像\n"
                "• 用鼠标在图像上点击并拖拽，框选出眼球区域\n"
                "• 尽量框选完整的眼球，避免包含过多背景\n"
                "• 框选完成后，点击\"下一步\"继续"
            );
            instructionLabel->setStyleSheet(
                "QLabel {"
                "    background-color: #e8f4fd;"
                "    border: 1px solid #b8daff;"
                "    border-radius: 6px;"
                "    padding: 15px;"
                "    font-size: 13px;"
                "    color: #0c5460;"
                "    line-height: 1.4;"
                "}"
            );
            instructionLabel->setWordWrap(true);
            
            scrollLayout->addWidget(stepLabel);
            scrollLayout->addWidget(descLabel);
            scrollLayout->addWidget(previewArea, 1);
            scrollLayout->addWidget(instructionLabel);
            scrollLayout->addStretch();
            
            // 设置滚动区域内容
            m_eyeGuideScrollArea->setWidget(scrollContent);
            
            // 将滚动区域添加到步骤布局
            stepLayout->addWidget(m_eyeGuideScrollArea);
            
            // 为图像预览标签添加ROI事件过滤器
            setupROIEventFilters();
            
        } else {
            // 其他步骤保持原样
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
        }
        
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
    QLabel *welcomeLabel = new QLabel("PaperTracker 主界面");
    welcomeLabel->setObjectName("welcomeLabel");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(welcomeLabel);

    // 描述文本
    QLabel *descLabel = new QLabel("欢迎使用您的专业追踪设备管理中心");
    descLabel->setObjectName("descLabel");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    // 添加说明文本
    QLabel *instructionLabel = new QLabel(
        "这里是您的设备管理主界面\n\n"
        "🔌 请通过USB连接您的追踪设备\n\n"
        "📱 设备连接后将自动识别并出现在左侧边栏的\"已连接设备\"区域\n\n"
        "⚙️ 点击左侧设备标签页即可进入相应的配置界面\n\n"
        "🌐 支持面部追踪和眼部追踪设备的WiFi配置和参数调整\n\n"
        "🏠 随时点击\"主界面\"返回此欢迎页面"
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
    if (itemText == "主界面") {
        m_contentStack->setCurrentWidget(m_defaultContentWidget);
        
        // 取消所有设备标签的选中状态
        if (m_sidebar) {
            m_sidebar->clearDeviceSelection();
        }
    }
}

void MainWindow::onDeviceTabClicked(const QString &deviceName)
{
    if (m_deviceManager) {
        QString deviceType = m_deviceManager->getDeviceType(deviceName);
        
        // 检查是否是等待WiFi配置完成的设备
        if (m_pendingWifiDevices.contains(deviceName)) {
            // 设备正在等待WiFi连接，保持在WiFi配置页面
            m_contentStack->setCurrentWidget(m_wifiConfigWidget);
            return;
        }
        
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
    // 获取当前正在配置的设备名称
    std::string portName = m_serialManager->getCurrentPortName();
    QString deviceName = QString("设备 (%1)").arg(QString::fromStdString(portName));
    
    // 将设备标记为等待WiFi连接完成
    m_pendingWifiDevices.insert(deviceName, deviceType);
    
    QMessageBox::information(this, tr("WiFi配置完成"),
        tr("WiFi配置已发送成功。\n网络: %1\n\n设备正在连接WiFi网络，请等待设备连接完成后自动跳转到配置页面。")
        .arg(wifiName));
    
    // 不再切换页面，保持在WiFi配置页面等待设备连接
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
    // if (m_contentStack->currentWidget() == m_wifiConfigWidget) {
    //     m_wifiConfigWidget->updateWifiInfo(wifiName, wifiPassword);
    //     return;
    // }

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
    // m_wifiConfigWidget->updateWifiInfo(wifiName, wifiPassword);
    m_contentStack->setCurrentWidget(m_wifiConfigWidget);
}

void MainWindow::setupROIEventFilters()
{
    if (m_eyePreviewLabel) {
        auto roiFilter = new ROIEventFilter([this](QRect rect, bool isEnd, int tag) {
            onEyeROIChanged(rect, isEnd);
        }, m_eyePreviewLabel, 0);
        m_eyePreviewLabel->installEventFilter(roiFilter);
    }
    
    // 更新设备信息显示
    updateEyeDeviceInfo();
}

void MainWindow::updateEyeDeviceInfo()
{
    if (m_eyeGuideScrollArea) {
        QLabel *eyeTitle = m_eyeGuideScrollArea->findChild<QLabel*>("eyePreviewTitle");
        QLabel *deviceInfo = m_eyeGuideScrollArea->findChild<QLabel*>("deviceInfoLabel");
        
        if (eyeTitle && deviceInfo) {
            QString eyeType = "眼部";
            QString deviceTypeText = "设备类型：";
            
            if (m_currentEyeVersion == LEFT_VERSION) {
                eyeType = "左眼";
                deviceTypeText += "左眼追踪器";
            } else if (m_currentEyeVersion == RIGHT_VERSION) {
                eyeType = "右眼";
                deviceTypeText += "右眼追踪器";
            } else {
                deviceTypeText += "等待检测...";
            }
            
            eyeTitle->setText(eyeType + "预览");
            deviceInfo->setText(deviceTypeText);
        }
    }
}

void MainWindow::onEyeROIChanged(QRect rect, bool isEnd)
{
    // 规范化矩形坐标
    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height = rect.height();
    
    // 规范化宽度和高度为正值
    if (width < 0) {
        x += width;
        width = -width;
    }
    if (height < 0) {
        y += height;
        height = -height;
    }
    
    // 裁剪坐标到图像边界内
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    
    // 确保ROI不超出图像边界
    if (x + width > 280) {
        width = 280 - x;
    }
    if (y + height > 280) {
        height = 280 - y;
    }
    
    // 确保最终的宽度和高度为正值
    width = qMax(0, width);
    height = qMax(0, height);
    
    m_eyeRoiRect = QRect(x, y, width, height);
    
    if (isEnd) {
        QString eyeType = "Eye";
        if (m_currentEyeVersion == LEFT_VERSION) {
            eyeType = "Left eye";
        } else if (m_currentEyeVersion == RIGHT_VERSION) {
            eyeType = "Right eye";
        }
        qDebug() << eyeType << "ROI selected:" << m_eyeRoiRect;
    }
}