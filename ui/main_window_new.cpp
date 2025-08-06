//
// Created by colorful on 25-7-28.
//
#include "include/main_window_new.h"
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QFrame>
#include <QProgressBar>
#include <QListWidgetItem>
#include <QDateTime>
#include <QTimer>
#include <QLineEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QMap>
#include <QProgressDialog>
#include <memory>
#include <any>
#include "serial.hpp"

#define FACE_VERSION 1
#define LEFT_VERSION 2
#define RIGHT_VERSION 3

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName("MainPage");
    // 设置窗口为无边框
    setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    // 设置窗口属性以支持透明背景
    setAttribute(Qt::WA_TranslucentBackground);

    m_serialManager = std::make_shared<SerialPortManager>(this);
     m_serialManager->registerCallback(
            PACKET_DEVICE_STATUS,
            [this](const std::string& ip, int brightness, int power, int version) {
                // 根据 version 判断设备类型和名称
                QString deviceType;
                QString deviceId = QTime::currentTime().toString("hhmmss");
                QString deviceName;
                switch (version) {
                    case FACE_VERSION:
                        deviceType = "Face Tracker";
                        deviceName = QString("Face Tracker #%1").arg(deviceId);
                        break;
                    case LEFT_VERSION:
                        deviceType = "Eye Tracker";
                        deviceName = QString("Left Eye #%1").arg(deviceId);
                        break;
                    case RIGHT_VERSION:
                        deviceType = "Eye Tracker";
                        deviceName = QString("Right Eye #%1").arg(deviceId);
                        break;
                    default:
                        deviceType = "Unknown Device";
                        deviceName = QString("Device #%1").arg(deviceId);
                }
                // 主线程更新 UI
                QMetaObject::invokeMethod(this, [=]() {
                    onDeviceConnected(deviceName, deviceType);
                    updateDeviceStatus(deviceName, QString::fromStdString(ip), power);
                    QMessageBox::information(this, tr("设备发现"),
                        tr("发现设备: %1\nIP: %2\n电量: %3%\n已添加到列表")
                        .arg(deviceName)
                        .arg(QString::fromStdString(ip))
                        .arg(power));
                }, Qt::QueuedConnection);
            }
        );
        m_serialManager->registerCallback(
        PACKET_WIFI_ERROR,
        [this](int version) {
            // 根据 version 判断设备类型
            QString deviceType;
            QString deviceId = QTime::currentTime().toString("hhmmss");
            QString deviceName;
            switch (version) {
                case FACE_VERSION:
                    deviceType = "Face Tracker";
                    deviceName = QString("Face Tracker #%1").arg(deviceId);
                    break;
                case LEFT_VERSION:
                    deviceType = "Eye Tracker";
                    deviceName = QString("Left Eye #%1").arg(deviceId);
                    break;
                case RIGHT_VERSION:
                    deviceType = "Eye Tracker";
                    deviceName = QString("Right Eye #%1").arg(deviceId);
                    break;
                default:
                    deviceType = "Unknown Device";
                    deviceName = QString("Device #%1").arg(deviceId);
            }

            // 主线程处理WiFi配置需求
            QMetaObject::invokeMethod(this, [=]() {
                onDeviceNeedsWifiConfig(deviceName, deviceType);
            }, Qt::QueuedConnection);
        }
    );
    m_serialManager->registerRawDataCallback([this](const std::string& data) {
            LOG_INFO("串口原始数据: {}", data)
    });
    setupUI();
}

MainWindow::~MainWindow()
{
    // 析构函数，Qt对象会自动清理子对象
}

void MainWindow::setupUI() {
    setFixedSize(1300, 700);
    // 创建自定义标题栏
    setupTitleBar();
    // 创建阴影框架
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setColor(QColor(127, 127, 127, 127));
    shadowEffect->setBlurRadius(20);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);

    auto frame = new QFrame(this);
    frame->setGeometry(10, 10, width() - 20, height() - 20);
    frame->setGraphicsEffect(shadowEffect);
    frame->setObjectName("centralWidget");

    // 创建主布局（不包含标题栏）
    mainLayout = new QHBoxLayout(frame);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->setMenuBar(titleBarWidget);

    // 创建侧边栏
    sidebarWidget = new QWidget();
    sidebarWidget->setObjectName("sidebarWidget");
    sidebarWidget->setFixedWidth(200);

    sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setAlignment(Qt::AlignTop);
    sidebarLayout->setSpacing(0);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);

    // 创建主内容区域
    contentStack = new QStackedWidget();
    contentStack->setObjectName("contentStack");

    // 添加到主布局
    mainLayout->addWidget(sidebarWidget);
    mainLayout->addWidget(contentStack);

    // 创建内容页面
    createContentPages();

    // 创建侧边栏项 - 新增设备选项
    createSidebarItem(":/resources/resources/images/vr-cardboard-solid-full.png", "Add New Device");

    // 添加分隔线
    createSidebarSeparator();

    // 添加弹性空间，将设备列表推到底部
    sidebarLayout->addStretch();

    // 添加一些示例设备用于测试
    onDeviceConnected("Face Tracker #001", "Face Tracker");
    onDeviceConnected("Eye Tracker #002", "Eye Tracker");
}

// 在WiFi配置成功后，添加设备到侧边栏的功能
void MainWindow::onWiFiConfigurationSuccess(const QString &deviceType, const QString &wifiName) {
    // 当WiFi配置成功时，生成设备名称并添加到侧边栏
    QString deviceId = QDateTime::currentDateTime().toString("hhmmss");
    QString deviceName = QString("%1 #%2").arg(deviceType, deviceId);

    // 添加设备到侧边栏
    onDeviceConnected(deviceName, deviceType);

    // 自动切换到新设备的配置页面
    if (deviceContentPages.contains(deviceName)) {
        contentStack->setCurrentWidget(deviceContentPages[deviceName]);

        // 选中新添加的设备tab
        if (deviceTabs.contains(deviceName)) {
            setSelectedItem(deviceTabs[deviceName]);
        }
    }
}

void MainWindow::setupTitleBar() {
    // 创建标题栏
    titleBarWidget = new QWidget();
    titleBarWidget->setObjectName("titleBarWidget");
    titleBarWidget->setFixedHeight(40);
    // 移除直接样式设置，使用QSS

    // 创建标题栏布局
    titleBarLayout = new QHBoxLayout(titleBarWidget);
    titleBarLayout->setContentsMargins(0, 0, 0, 0);
    titleBarLayout->setSpacing(0);

    // 创建标题栏左侧区域（与侧边栏同色）
    titleLeftArea = new QWidget();
    titleLeftArea->setObjectName("titleLeftArea");
    titleLeftArea->setFixedWidth(200);
    // 移除直接样式设置，使用QSS
    titleLeftArea->setCursor(Qt::ArrowCursor);

    // 创建标题文本
    titleLabel = new QLabel("PaperTracker");
    titleLabel->setObjectName("titleLabel");
    // 移除直接样式设置，使用QSS

    // 左侧区域布局
    QHBoxLayout *leftLayout = new QHBoxLayout(titleLeftArea);
    leftLayout->setContentsMargins(20, 0, 0, 0);
    leftLayout->addWidget(titleLabel);
    leftLayout->addStretch();

    // 创建标题栏右侧区域
    titleRightArea = new QWidget();
    titleRightArea->setObjectName("titleRightArea");
    // 移除直接样式设置，使用QSS

    // 右侧区域布局
    titleRightLayout = new QHBoxLayout(titleRightArea);
    titleRightLayout->setContentsMargins(0, 0, 0, 0);
    titleRightLayout->setSpacing(0);

    // 创建按钮
    minimizeButton = new QPushButton();
    minimizeButton->setObjectName("minimizeButton");
    barsButton = new QPushButton();
    barsButton->setObjectName("barsButton");
    bellButton = new QPushButton();
    bellButton->setObjectName("bellButton");
    closeButton = new QPushButton();
    closeButton->setObjectName("closeButton");

    // 设置按钮大小
    QSize buttonSize(40, 40);
    minimizeButton->setFixedSize(buttonSize);
    barsButton->setFixedSize(buttonSize);
    bellButton->setFixedSize(buttonSize);
    closeButton->setFixedSize(buttonSize);

    // 设置按钮图标
    minimizeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    minimizeButton->setIconSize(QSize(12, 12));

    // 加载自定义图标
    QPixmap barsPixmap(":/resources/resources/images/bars-solid-full.png");
    if (!barsPixmap.isNull()) {
        barsButton->setIcon(QIcon(barsPixmap));
        barsButton->setIconSize(QSize(16, 16));
    }

    QPixmap bellPixmap(":/resources/resources/images/bell-regular-full.png");
    if (!bellPixmap.isNull()) {
        bellButton->setIcon(QIcon(bellPixmap));
        bellButton->setIconSize(QSize(16, 16));
    }

    closeButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    closeButton->setIconSize(QSize(12, 12));

    // 连接按钮信号
    connect(minimizeButton, &QPushButton::clicked, this, &MainWindow::onMinimizeButtonClicked);
    connect(barsButton, &QPushButton::clicked, this, &MainWindow::onBarsButtonClicked);
    connect(bellButton, &QPushButton::clicked, this, &MainWindow::onBellButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::onCloseButtonClicked);

    // 添加按钮到右侧布局
    titleRightLayout->addStretch();
    titleRightLayout->addWidget(barsButton);
    titleRightLayout->addWidget(bellButton);
    titleRightLayout->addSpacing(4);
    titleRightLayout->addWidget(minimizeButton);
    titleRightLayout->addWidget(closeButton);

    // 添加到标题栏布局
    titleBarLayout->addWidget(titleLeftArea);
    titleBarLayout->addWidget(titleRightArea);

    // 为标题栏安装事件过滤器来处理拖动
    titleBarWidget->installEventFilter(this);
    titleLeftArea->installEventFilter(this);
    titleRightArea->installEventFilter(this);
    titleLabel->installEventFilter(this);
}

void MainWindow::setSelectedItem(QWidget *selectedItem) {
    // 取消之前选中项的选中状态
    for (QWidget *item : sidebarItems) {
        if (item->property("selected").toBool()) {
            item->setProperty("selected", false);
            item->setStyleSheet(""); // 清除样式
        }
    }

    // 设置新的选中项
    selectedItem->setProperty("selected", true);
    selectedItem->setStyleSheet(
        "QWidget#SiderBarItem {"
        "    background-color: #e7ebf0;"
        "}"
        "QLabel#sidebarIconLabel {"
        "    background-color: #e7ebf0;"
        "}"
        "QLabel#sidebarTextLabel {"
        "    background-color: #e7ebf0;"
        "    color: #0070f9;"
        "}"
    );
}
void MainWindow::mousePressEvent(QMouseEvent *event) {
    // 移除主窗口的拖动处理，改为在 eventFilter 中处理
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    // 移除主窗口的拖动处理，改为在 eventFilter 中处理
    QMainWindow::mouseMoveEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // 检查是否是标题栏相关的拖动事件
    bool isTitleBarArea = (obj == titleBarWidget || obj == titleLeftArea ||
                          obj == titleRightArea || obj == titleLabel);

    if (isTitleBarArea) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                dragPosition = mouseEvent->globalPos() - frameGeometry().topLeft();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                move(mouseEvent->globalPos() - dragPosition);
                return true;
            }
        }
    }

    // 处理开始页面按钮的 hover 效果
    QPushButton *button = qobject_cast<QPushButton*>(obj);
    if (button && (button == faceTrackerButton || button == eyeTrackerButton)) {
        if (event->type() == QEvent::Enter) {
            // 鼠标进入时的效果
            QString enhancedHoverStyle =
                "QPushButton {"
                "    background-color: #f8f9ff;"
                "    border: 2px solid #0070f9;"
                "    border-radius: 12px;"
                "    padding: 18px 20px 22px 20px;" // 模拟上移效果
                "}";

            button->setStyleSheet(enhancedHoverStyle);

            // 直接操作成员变量更改文字颜色
            if (button == faceTrackerButton) {
                faceText->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #0070f9; }");
                faceDesc->setStyleSheet("QLabel { font-size: 12px; color: #0070f9; }");
            } else if (button == eyeTrackerButton) {
                eyeText->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #0070f9; }");
                eyeDesc->setStyleSheet("QLabel { font-size: 12px; color: #0070f9; }");
            }
            return true;
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开时恢复原样
            QString normalStyle =
                "QPushButton {"
                "    background-color: white;"
                "    border: 2px solid #e0e0e0;"
                "    border-radius: 12px;"
                "    padding: 20px;"
                "}";
            button->setStyleSheet(normalStyle);

            // 直接操作成员变量重置文字样式
            if (button == faceTrackerButton) {
                faceText->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #333; }");
                faceDesc->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
            } else if (button == eyeTrackerButton) {
                eyeText->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #333; }");
                eyeDesc->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
            }
            return true;
        }
    }

    // 查找包含在sidebarItems中的对象
    QWidget *itemWidget = nullptr;
    for (QWidget *widget : sidebarItems) {
        if (widget == obj || widget->isAncestorOf(qobject_cast<QWidget*>(obj))) {
            itemWidget = widget;
            break;
        }
    }

    if (itemWidget) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                setSelectedItem(itemWidget);
                QString itemText = itemWidget->property("itemText").toString();
                onSidebarItemClicked(itemText);
                return true;
            }
        } else if (event->type() == QEvent::Enter) {
            if (!itemWidget->property("selected").toBool()) {
                itemWidget->setStyleSheet(
                    "QWidget#SiderBarItem {"
                    "    background-color: #f0f0f0;"
                    "}"
                    "QLabel#sidebarIconLabel {"
                    "    background-color: #f0f0f0;"
                    "}"
                    "QLabel#sidebarTextLabel {"
                    "    background-color: #f0f0f0;"
                    "}"
                );
            }
            return true;
        } else if (event->type() == QEvent::Leave) {
            if (!itemWidget->property("selected").toBool()) {
                itemWidget->setStyleSheet("");
            }
            return true;
        }
    }

    // 处理菜单项的 hover 效果
    if (customMenu && customMenu->isVisible()) {
        QWidget *menuItem = qobject_cast<QWidget*>(obj);
        if (menuItem && menuItem->objectName() == "MenuItem") {
            if (event->type() == QEvent::Enter) {
                menuItem->setStyleSheet(
                    "QWidget#MenuItem {"
                    "    background-color: #f0f0f0;"
                    "    border-radius: 4px;"
                    "}"
                );
                return true;
            } else if (event->type() == QEvent::Leave) {
                menuItem->setStyleSheet(
                    "QWidget#MenuItem {"
                    "    background-color: transparent;"
                    "    border-radius: 4px;"
                    "}"
                );
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onMinimizeButtonClicked() {
    showMinimized();
}

void MainWindow::onCloseButtonClicked() {
    close();
}

void MainWindow::onBellButtonClicked() {
    // TODO: 实现bell按钮功能
}

// CustomMenu 实现
CustomMenu::CustomMenu(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    setObjectName("CustomMenu");

    menuLayout = new QVBoxLayout(this);
    menuLayout->setContentsMargins(0, 8, 0, 8);
    menuLayout->setSpacing(0);
}

void CustomMenu::addItem(QWidget *item)
{
    menuLayout->addWidget(item);
}

void CustomMenu::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制白色背景和圆角
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.fillPath(path, QColor(255, 255, 255));

    // 绘制边框（可选）
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.drawPath(path);
}

void CustomMenu::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    emit menuHidden();
}

void MainWindow::createCustomMenu()
{
    customMenu = new CustomMenu(this);

    // 连接菜单隐藏信号到销毁方法
    connect(customMenu, &CustomMenu::menuHidden, this, &MainWindow::destroyCustomMenu);

    // 添加菜单项
    addMenuItem(":/resources/resources/images/bars-solid-full.png", "Settings", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/bell-regular-full.png", "Notifications", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/face-smile-regular-full.png", "Profile", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/vr-cardboard-solid-full.png", "Help", ":/resources/resources/images/chevron-down-solid-full.png");
}

void MainWindow::addMenuItem(const QString &iconPath, const QString &text, const QString &rightIconPath)
{
    // 创建菜单项容器
    QWidget *menuItem = new QWidget();
    menuItem->setObjectName("MenuItem");
    menuItem->setFixedHeight(48);
    menuItem->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *itemLayout = new QHBoxLayout(menuItem);
    itemLayout->setContentsMargins(16, 12, 16, 12);
    itemLayout->setSpacing(12);

    // 左侧图标
    QLabel *iconLabel = new QLabel();
    iconLabel->setObjectName("MenuItemIcon");
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        iconLabel->setFixedSize(20, 20);
    }

    // 文字内容
    QLabel *textLabel = new QLabel(text);
    textLabel->setObjectName("MenuItemText");
    textLabel->setStyleSheet("QLabel#MenuItemText { color: #333333; font-size: 14px; font-weight: 400; }");

    // 右侧图标
    QLabel *rightIconLabel = new QLabel();
    rightIconLabel->setObjectName("MenuItemRightIcon");
    if (!rightIconPath.isEmpty()) {
        QPixmap rightIconPixmap(rightIconPath);
        if (!rightIconPixmap.isNull()) {
            rightIconLabel->setPixmap(rightIconPixmap.scaled(14, 14, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            rightIconLabel->setFixedSize(14, 14);
        }
    }

    // 添加到布局
    itemLayout->addWidget(iconLabel);
    itemLayout->addWidget(textLabel);
    itemLayout->addStretch();
    itemLayout->addWidget(rightIconLabel);

    // 设置初始样式
    menuItem->setStyleSheet(
        "QWidget#MenuItem {"
        "    background-color: transparent;"
        "    border-radius: 4px;"
        "}"
    );

    // 添加hover效果 - 为菜单项及其子控件都安装事件过滤器
    menuItem->installEventFilter(this);
    iconLabel->installEventFilter(this);
    textLabel->installEventFilter(this);
    rightIconLabel->installEventFilter(this);

    // 添加到菜单
    customMenu->addItem(menuItem);
}

void MainWindow::resetMenuItemsStyle()
{
    if (!customMenu) return;

    // 遍历菜单中的所有子控件，重置菜单项样式
    QList<QWidget*> menuItems = customMenu->findChildren<QWidget*>("MenuItem");
    for (QWidget *item : menuItems) {
        item->setStyleSheet(
            "QWidget#MenuItem {"
            "    background-color: transparent;"
            "    border-radius: 4px;"
            "}"
        );
    }
}

void MainWindow::destroyCustomMenu()
{
    if (customMenu) {
        customMenu->deleteLater();
        customMenu = nullptr;
    }
}

// 修改 onBarsButtonClicked 方法
void MainWindow::onBarsButtonClicked() {
    // 每次都重新创建菜单
    if (customMenu) {
        customMenu->deleteLater();
        customMenu = nullptr;
    }

    createCustomMenu();

    // 计算菜单显示位置 - 菜单右上角在按钮左下角一点点
    QPoint buttonGlobalPos = barsButton->mapToGlobal(QPoint(0, 0));
    QSize menuSize(200, 220); // 菜单大小
    customMenu->setFixedSize(menuSize);

    // 计算位置：菜单右上角对齐到按钮左下角稍微偏移
    QPoint menuPos = QPoint(
        buttonGlobalPos.x() - menuSize.width() + 8, // 右对齐到按钮左侧，稍微偏移8px
        buttonGlobalPos.y() + barsButton->height() + 2 // 下方稍微偏移2px
    );

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(customMenu);
    shadowEffect->setColor(QColor(0, 0, 0, 30));
    shadowEffect->setBlurRadius(12);
    shadowEffect->setOffset(0, 4);
    customMenu->setGraphicsEffect(shadowEffect);

    // 显示菜单
    customMenu->move(menuPos);
    customMenu->show();
}

void MainWindow::createContentPages() {
    // 创建默认内容
    createDefaultContent();

        // 创建 Face Tracker 引导界面
    faceGuideWidget = new GuideWidget("Face Tracker Setup", 4, this);

    // Face Tracker 第一步：WiFi 设置
    QWidget *faceWifiContent = createWifiSetupStep("Face Tracker");
    faceGuideWidget->setStepContent(1, faceWifiContent);


    QWidget *faceStep2Content = createFaceConfigStep("Face Tracker");
    faceGuideWidget->setStepContent(2, faceStep2Content);

    // 为 Face Tracker 设置其他步骤内容
    for (int i = 3; i <= 4; ++i) {
        QWidget *stepContent = new QWidget();
        stepContent->setObjectName("StepContent");
        QVBoxLayout *stepLayout = new QVBoxLayout(stepContent);
        stepLayout->setAlignment(Qt::AlignCenter);

        QLabel *stepLabel = new QLabel(QString("Face Tracker - Step %1").arg(i));
        stepLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #333; margin: 20px; }");
        stepLabel->setAlignment(Qt::AlignCenter);

        QLabel *descLabel = new QLabel(QString("This is the content for step %1 of Face Tracker setup.\nFollow the instructions to complete this step.").arg(i));
        descLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin: 10px; }");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);

        stepLayout->addWidget(stepLabel);
        stepLayout->addWidget(descLabel);
        stepLayout->addStretch();

        faceGuideWidget->setStepContent(i, stepContent);
    }

    // 创建 Eye Tracker 引导界面
    eyeGuideWidget = new GuideWidget("Eye Tracker Setup", 5, this);

    // Eye Tracker 第一步：WiFi 设置
    QWidget *eyeWifiContent = createWifiSetupStep("Eye Tracker");
    eyeGuideWidget->setStepContent(1, eyeWifiContent);

    // 为 Eye Tracker 设置其他步骤内容
    for (int i = 2; i <= 5; ++i) {
        QWidget *stepContent = new QWidget();
        stepContent->setObjectName("StepContent");
        QVBoxLayout *stepLayout = new QVBoxLayout(stepContent);
        stepLayout->setAlignment(Qt::AlignCenter);

        QLabel *stepLabel = new QLabel(QString("Eye Tracker - Step %1").arg(i));
        stepLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #333; margin: 20px; }");
        stepLabel->setAlignment(Qt::AlignCenter);

        QLabel *descLabel = new QLabel(QString("This is the content for step %1 of Eye Tracker setup.\nConfigure your eye tracking settings here.").arg(i));
        descLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; margin: 10px; }");
        descLabel->setAlignment(Qt::AlignCenter);
        descLabel->setWordWrap(true);

        stepLayout->addWidget(stepLabel);
        stepLayout->addWidget(descLabel);
        stepLayout->addStretch();

        eyeGuideWidget->setStepContent(i, stepContent);
    }

    // 添加到内容栈
    contentStack->addWidget(defaultContentWidget);
    contentStack->addWidget(faceGuideWidget);
    contentStack->addWidget(eyeGuideWidget);

    // 默认显示默认内容
    contentStack->setCurrentWidget(defaultContentWidget);
}

void MainWindow::createDefaultContent() {
    // 创建默认内容容器
    defaultContentWidget = new QWidget();
    defaultContentWidget->setObjectName("defaultContent");

    QVBoxLayout *layout = new QVBoxLayout(defaultContentWidget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(30);
    layout->setContentsMargins(50, 50, 50, 50);

    // 欢迎标题
    QLabel *welcomeLabel = new QLabel("Welcome to PaperTracker");
    welcomeLabel->setObjectName("welcomeLabel");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setStyleSheet(
        "QLabel#welcomeLabel {"
        "    font-size: 32px;"
        "    font-weight: bold;"
        "    color: #333;"
        "    margin-bottom: 10px;"
        "}"
    );
    layout->addWidget(welcomeLabel);

    // 描述文本
    QLabel *descLabel = new QLabel("Get started by scanning for devices or adding a new device manually");
    descLabel->setObjectName("descLabel");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(
        "QLabel#descLabel {"
        "    font-size: 16px;"
        "    color: #666;"
        "    margin-bottom: 20px;"
        "}"
    );
    layout->addWidget(descLabel);

    // 扫描设备按钮
    QPushButton *scanButton = new QPushButton("🔍 Scan for Devices");
    scanButton->setObjectName("scanButton");
    scanButton->setFixedHeight(50);
    scanButton->setStyleSheet(
        "QPushButton#scanButton {"
        "    background-color: #28a745;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton#scanButton:hover {"
        "    background-color: #218838;"
        "}"
        "QPushButton#scanButton:pressed {"
        "    background-color: #1e7e34;"
        "}"
    );
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanDevicesButtonClicked);
    layout->addWidget(scanButton);

    // 添加间距
    layout->addSpacing(20);

    // 设备类型选择容器
    QWidget *buttonContainer = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setSpacing(20);

    // Face Tracker 按钮
    QPushButton *faceButton = new QPushButton();
    faceButton->setObjectName("faceButton");
    faceButton->setFixedSize(200, 120);
    faceButton->setText("Face Tracker");
    faceButton->setStyleSheet(
        "QPushButton#faceButton {"
        "    background-color: #007bff;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 10px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton#faceButton:hover {"
        "    background-color: #0056b3;"
        "}"
    );
    connect(faceButton, &QPushButton::clicked, [this]() {
        startDeviceSetupFlow("Face Tracker");
    });
    buttonLayout->addWidget(faceButton);

    // Eye Tracker 按钮
    QPushButton *eyeButton = new QPushButton();
    eyeButton->setObjectName("eyeButton");
    eyeButton->setFixedSize(200, 120);
    eyeButton->setText("Eye Tracker");
    eyeButton->setStyleSheet(
        "QPushButton#eyeButton {"
        "    background-color: #6c757d;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 10px;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton#eyeButton:hover {"
        "    background-color: #545b62;"
        "}"
    );
    connect(eyeButton, &QPushButton::clicked, [this]() {
        startDeviceSetupFlow("Eye Tracker");
    });
    buttonLayout->addWidget(eyeButton);

    layout->addWidget(buttonContainer);
    layout->addStretch();

    // 添加到内容栈
    contentStack->addWidget(defaultContentWidget);
}

void MainWindow::createSidebarItem(const QString &iconPath, const QString &text) {
    // 创建侧边栏项容器
    QWidget *itemWidget = new QWidget();
    itemWidget->setObjectName("SiderBarItem");
    itemWidget->setProperty("selected", false);
    itemWidget->setProperty("itemText", text); // 添加文本属性用于识别
    QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
    itemLayout->setContentsMargins(10, 15, 10, 15);
    itemLayout->setSpacing(10);

    // 图标标签
    QLabel *iconLabel = new QLabel();
    iconLabel->setObjectName("sidebarIconLabel");
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        iconLabel->setFixedSize(24, 24);
    }

    // 文本标签
    QLabel *textLabel = new QLabel(text);
    textLabel->setObjectName("sidebarTextLabel");

    // 添加到布局
    itemLayout->addWidget(iconLabel);
    itemLayout->addWidget(textLabel);
    itemLayout->addStretch();

    // 添加到侧边栏布局
    sidebarLayout->addWidget(itemWidget);

    // 保存引用以便后续操作
    sidebarItems.append(itemWidget);

    // 连接点击事件 - 修改为处理页面切换
    connect(itemWidget, &QWidget::customContextMenuRequested, this, [=]() {
        setSelectedItem(itemWidget);
        onSidebarItemClicked(text);
    });

    // 安装事件过滤器处理hover效果和点击事件
    itemWidget->installEventFilter(this);
    iconLabel->installEventFilter(this);
    textLabel->installEventFilter(this);
}

void MainWindow::onSidebarItemClicked(const QString &itemText) {
    if (itemText == "Add New Device") {
        // 显示设备选择页面
        contentStack->setCurrentWidget(defaultContentWidget);
    } else if (deviceContentPages.contains(itemText)) {
        // 处理已连接设备的点击 - 显示该设备的配置页面
        contentStack->setCurrentWidget(deviceContentPages[itemText]);
    } else {
        // 默认显示主页面
        contentStack->setCurrentWidget(defaultContentWidget);
    }
}

// GuideWidget 实现
GuideWidget::GuideWidget(const QString &title, int totalSteps, QWidget *parent)
    : QWidget(parent)
    , guideTitle(title)
    , totalSteps(totalSteps)
    , currentStep(1)
{
    setupUI();
}

void GuideWidget::setupUI() {
    setObjectName("GuideWidget");

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 15, 20, 20);  // 减小顶部和左右边距
    mainLayout->setSpacing(10);  // 减小整体间距

    // 标题
    titleLabel = new QLabel(guideTitle);
    titleLabel->setObjectName("GuideTitle");
    titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #333; margin: 5px 0; }");  // 减小字体和边距
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setMaximumHeight(35);  // 限制标题高度

    // 进度条
    progressBar = new QProgressBar();
    progressBar->setObjectName("GuideProgressBar");
    progressBar->setRange(0, totalSteps);
    progressBar->setValue(currentStep);
    progressBar->setTextVisible(false); // 隐藏进度条数字
    progressBar->setFixedHeight(16);  // 减小进度条高度
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #ddd;"  // 减小边框
        "    border-radius: 8px;"
        "    background-color: #f0f0f0;"
        "    height: 16px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #0070f9;"
        "    border-radius: 7px;"
        "}"
    );

    // 内容区域 - 这里是关键，给它更大的空间权重
    contentStack = new QStackedWidget();
    contentStack->setObjectName("GuideContentStack");
    contentStack->setContentsMargins(0, 5, 0, 5);  // 减小内容区域边距

    // 按钮区域
    buttonArea = new QWidget();
    buttonArea->setObjectName("GuideButtonArea");
    buttonArea->setFixedHeight(50);  // 固定按钮区域高度
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonArea);
    buttonLayout->setContentsMargins(0, 10, 0, 0);  // 只保留顶部间距
    buttonLayout->setSpacing(10);

    prevButton = new QPushButton("Previous");
    prevButton->setObjectName("GuidePrevButton");
    prevButton->setFixedHeight(36);  // 固定按钮高度

    nextButton = new QPushButton("Next");
    nextButton->setObjectName("GuideNextButton");
    nextButton->setFixedHeight(36);  // 固定按钮高度

    buttonLayout->addStretch();
    buttonLayout->addWidget(prevButton);
    buttonLayout->addWidget(nextButton);

    // 连接信号
    connect(prevButton, &QPushButton::clicked, this, &GuideWidget::onPrevButtonClicked);
    connect(nextButton, &QPushButton::clicked, this, &GuideWidget::onNextButtonClicked);

    // 添加到主布局 - 关键是给内容区域设置拉伸因子
    mainLayout->addWidget(titleLabel, 0);      // 标题不拉伸
    mainLayout->addWidget(progressBar, 0);     // 进度条不拉伸
    mainLayout->addWidget(contentStack, 1);    // 内容区域占据剩余所有空间
    mainLayout->addWidget(buttonArea, 0);      // 按钮区域不拉伸

    updateButtons();
}

void GuideWidget::setCurrentStep(int step) {
    if (step >= 1 && step <= totalSteps) {
        currentStep = step;
        progressBar->setValue(currentStep);
        contentStack->setCurrentIndex(currentStep - 1);
        updateButtons();
    }
}

void GuideWidget::setStepContent(int step, QWidget *content) {
    if (step >= 1 && step <= totalSteps) {
        if (contentStack->count() < step) {
            // 补齐空的页面
            while (contentStack->count() < step - 1) {
                contentStack->addWidget(new QWidget());
            }
            contentStack->addWidget(content);
        } else {
            contentStack->removeWidget(contentStack->widget(step - 1));
            contentStack->insertWidget(step - 1, content);
        }
    }
}

void GuideWidget::onNextButtonClicked() {
    if (currentStep < totalSteps) {
        setCurrentStep(currentStep + 1);
    }
}

void GuideWidget::onPrevButtonClicked() {
    if (currentStep > 1) {
        setCurrentStep(currentStep - 1);
    }
}

void GuideWidget::updateButtons() {
    prevButton->setEnabled(currentStep > 1);
    if (currentStep == totalSteps) {
        nextButton->setText("Finish");
    } else {
        nextButton->setText("Next");
    }
}

QWidget* MainWindow::createWifiSetupStep(const QString &deviceType) {
     WiFiSetupWidget *wifiWidget = new WiFiSetupWidget(deviceType, m_serialManager, this);

    // 连接配置成功信号
    connect(wifiWidget, &WiFiSetupWidget::configurationSuccess,
            this, &MainWindow::onWiFiConfigurationSuccess);

    return wifiWidget;
}

QWidget* MainWindow::createFaceConfigStep(const QString &deviceType) {
    FaceConfigWidget *faceWidget = new FaceConfigWidget(deviceType, this);

    return faceWidget;
}

// 设备管理方法
void MainWindow::addDeviceTab(const QString &deviceName, const QString &deviceType) {
    // 如果设备已存在，不重复添加
    if (deviceTabs.contains(deviceName)) {
        return;
    }

    // 确定设备图标
    QString iconPath;
    if (deviceType.contains("Face", Qt::CaseInsensitive)) {
        iconPath = ":/resources/resources/images/vr-cardboard-solid-full.png";
    } else if (deviceType.contains("Eye", Qt::CaseInsensitive)) {
        iconPath = ":/resources/resources/images/face-smile-regular-full.png";
    } else {
        iconPath = ":/resources/resources/images/vr-cardboard-solid-full.png"; // 默认图标
    }

    // 创建设备tab
    QWidget *deviceTab = new QWidget();
    deviceTab->setObjectName("SiderBarItem");
    deviceTab->setProperty("selected", false);
    deviceTab->setProperty("itemText", deviceName);

    QHBoxLayout *itemLayout = new QHBoxLayout(deviceTab);
    itemLayout->setContentsMargins(10, 15, 10, 15);
    itemLayout->setSpacing(10);

    // 图标标签
    QLabel *iconLabel = new QLabel();
    iconLabel->setObjectName("sidebarIconLabel");
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        iconLabel->setFixedSize(24, 24);
    }

    // 文本标签
    QLabel *textLabel = new QLabel(deviceName);
    textLabel->setObjectName("sidebarTextLabel");

    // 状态指示器（小绿点）
    QLabel *statusDot = new QLabel();
    statusDot->setObjectName("deviceStatusDot");
    statusDot->setFixedSize(8, 8);
    statusDot->setStyleSheet(
        "QLabel#deviceStatusDot {"
        "    background-color: #4CAF50;"
        "    border-radius: 4px;"
        "}"
    );

    // 添加到布局
    itemLayout->addWidget(iconLabel);
    itemLayout->addWidget(textLabel);
    itemLayout->addStretch();
    itemLayout->addWidget(statusDot);

    // 插入到分隔线之前
    int separatorIndex = -1;
    for (int i = 0; i < sidebarLayout->count(); ++i) {
        QFrame *frame = qobject_cast<QFrame*>(sidebarLayout->itemAt(i)->widget());
        if (frame && frame->objectName() == "SidebarSeparator") {
            separatorIndex = i;
            break;
        }
    }

    if (separatorIndex != -1) {
        sidebarLayout->insertWidget(separatorIndex, deviceTab);
    } else {
        sidebarLayout->addWidget(deviceTab);
    }

    // 保存引用
    sidebarItems.append(deviceTab);
    deviceTabs[deviceName] = deviceTab;

    // 安装事件过滤器
    deviceTab->installEventFilter(this);
    iconLabel->installEventFilter(this);
    textLabel->installEventFilter(this);
    statusDot->installEventFilter(this);

    // 创建对应的内容页面（这里可以根据设备类型创建不同的配置页面）
    QWidget *deviceContentPage = createDeviceContentPage(deviceName, deviceType);
    contentStack->addWidget(deviceContentPage);
    deviceContentPages[deviceName] = deviceContentPage;
}

void MainWindow::removeDeviceTab(const QString &deviceName) {
    if (!deviceTabs.contains(deviceName)) {
        return;
    }

    // 移除侧边栏tab
    QWidget *tab = deviceTabs[deviceName];
    sidebarItems.removeAll(tab);
    sidebarLayout->removeWidget(tab);
    tab->deleteLater();
    deviceTabs.remove(deviceName);

    // 移除内容页面
    if (deviceContentPages.contains(deviceName)) {
        QWidget *contentPage = deviceContentPages[deviceName];
        contentStack->removeWidget(contentPage);
        contentPage->deleteLater();
        deviceContentPages.remove(deviceName);
    }
}

void MainWindow::clearAllDeviceTabs() {
    QStringList deviceNames = deviceTabs.keys();
    for (const QString &deviceName : deviceNames) {
        removeDeviceTab(deviceName);
    }
}

void MainWindow::startDeviceSetupFlow(const QString &deviceType) {
    // 根据设备类型选择相应的引导界面
    if (deviceType == "Face Tracker") {
        contentStack->setCurrentWidget(faceGuideWidget);
        faceGuideWidget->setCurrentStep(1); // 从第一步开始：WiFi设置
    } else if (deviceType == "Eye Tracker") {
        contentStack->setCurrentWidget(eyeGuideWidget);
        eyeGuideWidget->setCurrentStep(1); // 从第一步开始：WiFi设置
    }
}

void MainWindow::onDeviceConnected(const QString &deviceName, const QString &deviceType) {
    addDeviceTab(deviceName, deviceType);
}

void MainWindow::onDeviceDisconnected(const QString &deviceName) {
    removeDeviceTab(deviceName);
}

QWidget* MainWindow::createDeviceContentPage(const QString &deviceName, const QString &deviceType) {
    QWidget *contentPage = new QWidget();
    contentPage->setObjectName("deviceContentPage");

    QVBoxLayout *layout = new QVBoxLayout(contentPage);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // 设备标题
    QLabel *titleLabel = new QLabel(deviceName);
    titleLabel->setObjectName("deviceTitleLabel");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");
    layout->addWidget(titleLabel);

    // 设备类型
    QLabel *typeLabel = new QLabel(QString("Type: %1").arg(deviceType));
    typeLabel->setObjectName("deviceTypeLabel");
    typeLabel->setStyleSheet("font-size: 14px; color: #666;");
    layout->addWidget(typeLabel);

    // 连接状态
    QLabel *statusLabel = new QLabel("Status: Searching...");
    statusLabel->setObjectName("deviceStatusLabel");
    statusLabel->setStyleSheet("font-size: 14px; color: #orange; font-weight: bold;");
    layout->addWidget(statusLabel);

    // IP地址显示
    QLabel *ipLabel = new QLabel("IP Address: Unknown");
    ipLabel->setObjectName("deviceIPLabel");
    ipLabel->setStyleSheet("font-size: 14px; color: #666;");
    layout->addWidget(ipLabel);

    // 电量显示
    QLabel *batteryLabel = new QLabel("Battery: Unknown");
    batteryLabel->setObjectName("deviceBatteryLabel");
    batteryLabel->setStyleSheet("font-size: 14px; color: #666;");
    layout->addWidget(batteryLabel);

    // 分隔线
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // 根据设备类型创建特定的配置界面
    QWidget *configWidget = nullptr;
    if (deviceType == "Face Tracker") {
        // 修正构造函数调用：只传入deviceType，不传入deviceName
        configWidget = new FaceConfigWidget(deviceType, this);
    } else if (deviceType == "Eye Tracker") {
        // 创建简化的眼动仪配置界面
        configWidget = createEyeTrackerConfig(deviceName, deviceType);
    } else {
        // 通用设备配置界面
        configWidget = createGenericDeviceConfig(deviceName, deviceType);
    }

    layout->addWidget(configWidget);
    layout->addStretch();

    return contentPage;
}

// 添加创建眼动仪配置界面的方法
QWidget* MainWindow::createEyeTrackerConfig(const QString &deviceName, const QString &deviceType) {
    QWidget *configWidget = new QWidget();
    configWidget->setObjectName("eyeTrackerConfig");

    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    configLayout->setContentsMargins(0, 10, 0, 10);
    configLayout->setSpacing(15);

    // 眼动仪配置标题
    QLabel *configTitle = new QLabel("Eye Tracker Configuration");
    configTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #333; margin-bottom: 10px;");
    configLayout->addWidget(configTitle);

    // 跟踪模式选择
    QGroupBox *trackingModeGroup = new QGroupBox("Tracking Mode");
    trackingModeGroup->setFixedHeight(80);
    QVBoxLayout *trackingLayout = new QVBoxLayout(trackingModeGroup);

    QComboBox *trackingModeCombo = new QComboBox();
    trackingModeCombo->setFixedHeight(32);
    trackingModeCombo->addItem("Single Eye Tracking");
    trackingModeCombo->addItem("Dual Eye Tracking");
    trackingModeCombo->addItem("Gaze Point Tracking");
    trackingModeCombo->setCurrentIndex(1);

    trackingLayout->addWidget(trackingModeCombo);
    configLayout->addWidget(trackingModeGroup);

    // 采样率设置
    QGroupBox *samplingRateGroup = new QGroupBox("Sampling Rate");
    samplingRateGroup->setFixedHeight(80);
    QVBoxLayout *samplingLayout = new QVBoxLayout(samplingRateGroup);

    QHBoxLayout *samplingControlLayout = new QHBoxLayout();
    QLabel *minLabel = new QLabel("30Hz");
    minLabel->setStyleSheet("font-size: 10px; color: #666;");

    QSlider *samplingSlider = new QSlider(Qt::Horizontal);
    samplingSlider->setRange(30, 120);
    samplingSlider->setValue(60);
    samplingSlider->setFixedHeight(20);

    QLabel *maxLabel = new QLabel("120Hz");
    maxLabel->setStyleSheet("font-size: 10px; color: #666;");

    QLabel *valueLabel = new QLabel("60Hz");
    valueLabel->setStyleSheet("font-size: 11px; color: #333; font-weight: bold;");
    valueLabel->setFixedWidth(40);
    valueLabel->setAlignment(Qt::AlignCenter);

    connect(samplingSlider, &QSlider::valueChanged, [valueLabel](int value) {
        valueLabel->setText(QString("%1Hz").arg(value));
    });

    samplingControlLayout->addWidget(minLabel);
    samplingControlLayout->addWidget(samplingSlider);
    samplingControlLayout->addWidget(maxLabel);
    samplingControlLayout->addWidget(valueLabel);

    samplingLayout->addLayout(samplingControlLayout);
    configLayout->addWidget(samplingRateGroup);

    // 校准控制
    QGroupBox *calibrationGroup = new QGroupBox("Calibration Controls");
    calibrationGroup->setFixedHeight(100);
    QVBoxLayout *calibLayout = new QVBoxLayout(calibrationGroup);

    QHBoxLayout *calibButtonLayout = new QHBoxLayout();

    QPushButton *startCalibBtn = new QPushButton("Start Calibration");
    startCalibBtn->setFixedHeight(32);
    startCalibBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #28a745;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #218838;"
        "}"
    );

    QPushButton *stopCalibBtn = new QPushButton("Stop");
    stopCalibBtn->setFixedHeight(32);
    stopCalibBtn->setEnabled(false);
    stopCalibBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #dc3545;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c82333;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #cccccc;"
        "    color: #888;"
        "}"
    );

    QPushButton *resetCalibBtn = new QPushButton("Reset");
    resetCalibBtn->setFixedHeight(32);
    resetCalibBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #6c757d;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #545b62;"
        "}"
    );

    calibButtonLayout->addWidget(startCalibBtn);
    calibButtonLayout->addWidget(stopCalibBtn);
    calibButtonLayout->addWidget(resetCalibBtn);

    calibLayout->addLayout(calibButtonLayout);
    configLayout->addWidget(calibrationGroup);

    configLayout->addStretch();

    return configWidget;
}

// 添加创建通用设备配置界面的方法
QWidget* MainWindow::createGenericDeviceConfig(const QString &deviceName, const QString &deviceType) {
    QWidget *configWidget = new QWidget();
    configWidget->setObjectName("genericDeviceConfig");

    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    configLayout->setContentsMargins(0, 20, 0, 20);
    configLayout->setAlignment(Qt::AlignCenter);

    QLabel *configLabel = new QLabel("Device configuration options will appear here");
    configLabel->setAlignment(Qt::AlignCenter);
    configLabel->setStyleSheet("color: #999; font-style: italic; font-size: 16px;");
    configLayout->addWidget(configLabel);

    QLabel *deviceInfo = new QLabel(QString("Device: %1\nType: %2").arg(deviceName, deviceType));
    deviceInfo->setAlignment(Qt::AlignCenter);
    deviceInfo->setStyleSheet("color: #666; font-size: 14px; margin-top: 10px;");
    configLayout->addWidget(deviceInfo);

    configLayout->addStretch();

    return configWidget;
}
void MainWindow::createSidebarSeparator() {
    QFrame *separator = new QFrame();
    separator->setObjectName("SidebarSeparator");
    separator->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    separator->setFixedHeight(1);
    sidebarLayout->addWidget(separator);
}

// WiFiSetupWidget 实现
WiFiSetupWidget::WiFiSetupWidget(const QString &deviceType, std::shared_ptr<SerialPortManager> serialManager, QWidget *parent)
    : QWidget(parent), m_serialManager(serialManager), m_deviceType(deviceType)
{
    setupUI();
    retranslateUI();
}

void WiFiSetupWidget::setupUI()
{
    setObjectName("WiFiSetupContent");

    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("WiFiScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    // 创建内容容器
    QWidget *contentContainer = new QWidget();
    contentContainer->setObjectName("WiFiContentContainer");
    contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout *mainLayout = new QVBoxLayout(contentContainer);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    // 标题
    m_titleLabel = new QLabel();
    m_titleLabel->setObjectName("WiFiSetupTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setFixedHeight(32);

    // 描述
    m_descLabel = new QLabel();
    m_descLabel->setObjectName("WiFiSetupDesc");
    m_descLabel->setAlignment(Qt::AlignCenter);
    m_descLabel->setWordWrap(true);
    m_descLabel->setFixedHeight(40);

    // 创建水平布局
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);

    // WiFi 设置表单 (左侧)
    m_wifiGroupBox = new QGroupBox();

    QFormLayout *formLayout = new QFormLayout(m_wifiGroupBox);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(15, 15, 15, 15);

    // WiFi 网络名称输入
    m_wifiNameEdit = new QLineEdit();
    m_wifiNameEdit->setObjectName("WiFiNameEdit");
    m_wifiNameEdit->setFixedHeight(32);

    // WiFi 密码输入
    m_wifiPasswordEdit = new QLineEdit();
    m_wifiPasswordEdit->setObjectName("WiFiPasswordEdit");
    m_wifiPasswordEdit->setEchoMode(QLineEdit::Password);
    m_wifiPasswordEdit->setFixedHeight(32);

    // 显示密码复选框
    m_showPasswordCheckBox = new QCheckBox();
    m_showPasswordCheckBox->setObjectName("ShowPasswordCheckBox");

    // 设备状态指示器
    m_wifiStatusLabel = new QLabel();
    m_wifiStatusLabel->setObjectName("WiFiStatusLabel");
    m_wifiStatusLabel->setWordWrap(true);
    m_wifiStatusLabel->setMaximumHeight(50);

    // 按钮容器
    QWidget *buttonContainer = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    // 清除输入按钮
    m_clearButton = new QPushButton();
    m_clearButton->setObjectName("ClearButton");
    m_clearButton->setFixedHeight(30);

    // 发送配置按钮
    m_sendConfigButton = new QPushButton();
    m_sendConfigButton->setObjectName("SendConfigButton");
    m_sendConfigButton->setFixedHeight(30);

    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_sendConfigButton);

    // WiFi 历史列表 (右侧)
    m_historyGroupBox = new QGroupBox();
    m_historyGroupBox->setMaximumWidth(220);
    m_historyGroupBox->setMinimumWidth(180);

    QVBoxLayout *historyLayout = new QVBoxLayout(m_historyGroupBox);
    historyLayout->setContentsMargins(12, 12, 12, 12);
    historyLayout->setSpacing(8);

    // 创建历史列表
    m_historyListWidget = new QListWidget();
    m_historyListWidget->setObjectName("WiFiHistoryList");
    m_historyListWidget->setFixedHeight(200);

    // 添加示例历史记录
    QListWidgetItem *item1 = new QListWidgetItem();
    item1->setText("📶 Home_WiFi\n🕐 2024-08-04 10:30");
    item1->setData(Qt::UserRole, "Home_WiFi");
    m_historyListWidget->addItem(item1);

    // 清除历史按钮
    m_clearHistoryButton = new QPushButton();
    m_clearHistoryButton->setObjectName("ClearHistoryButton");
    m_clearHistoryButton->setFixedHeight(26);

    historyLayout->addWidget(m_historyListWidget);
    historyLayout->addWidget(m_clearHistoryButton);

    // 创建标签
    m_networkNameLabel = new QLabel();
    m_passwordLabel = new QLabel();
    m_statusLabel = new QLabel();

    // 添加到表单布局
    formLayout->addRow(m_networkNameLabel, m_wifiNameEdit);
    formLayout->addRow(m_passwordLabel, m_wifiPasswordEdit);
    formLayout->addRow("", m_showPasswordCheckBox);
    formLayout->addRow("", buttonContainer);
    formLayout->addRow(m_statusLabel, m_wifiStatusLabel);

    // 设备连接状态提示
    m_deviceStatusLabel = new QLabel();
    m_deviceStatusLabel->setAlignment(Qt::AlignCenter);
    m_deviceStatusLabel->setFixedHeight(18);

    // 添加配置表单和历史列表到水平布局
    contentLayout->addWidget(m_wifiGroupBox, 3);
    contentLayout->addWidget(m_historyGroupBox, 2);

    // 添加到主布局
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(m_descLabel);
    mainLayout->addSpacing(8);
    mainLayout->addLayout(contentLayout);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(m_deviceStatusLabel);

    // 设置滚动区域
    scrollArea->setWidget(contentContainer);

    // 创建最终容器
    QVBoxLayout *finalLayout = new QVBoxLayout(this);
    finalLayout->setContentsMargins(0, 0, 0, 0);
    finalLayout->addWidget(scrollArea);

    // 连接信号
    connect(m_sendConfigButton, &QPushButton::clicked, this, &WiFiSetupWidget::onSendConfigClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &WiFiSetupWidget::onClearClicked);
    connect(m_showPasswordCheckBox, &QCheckBox::toggled, this, &WiFiSetupWidget::onShowPasswordToggled);
    connect(m_historyListWidget, &QListWidget::itemClicked, this, &WiFiSetupWidget::onHistoryItemClicked);
    connect(m_clearHistoryButton, &QPushButton::clicked, this, &WiFiSetupWidget::onClearHistoryClicked);

    // 验证输入
    connect(m_wifiNameEdit, &QLineEdit::textChanged, this, &WiFiSetupWidget::validateInputs);
    connect(m_wifiPasswordEdit, &QLineEdit::textChanged, this, &WiFiSetupWidget::validateInputs);

    // 初始验证
    validateInputs();
}

void WiFiSetupWidget::retranslateUI()
{
    m_titleLabel->setText(tr("Configure %1 WiFi Settings").arg(m_deviceType));
    m_descLabel->setText(tr("Enter your WiFi network credentials to configure the %1 device.\n"
                           "The settings will be sent to the device via USB connection.").arg(m_deviceType));

    m_wifiGroupBox->setTitle(tr("WiFi Configuration"));
    m_historyGroupBox->setTitle(tr("Connection History"));

    m_networkNameLabel->setText(tr("Network Name:"));
    m_passwordLabel->setText(tr("Password:"));
    m_statusLabel->setText(tr("Status:"));

    m_wifiNameEdit->setPlaceholderText(tr("Enter WiFi network name"));
    m_wifiPasswordEdit->setPlaceholderText(tr("Enter WiFi password"));
    m_showPasswordCheckBox->setText(tr("Show password"));

    m_clearButton->setText(tr("Clear"));
    m_sendConfigButton->setText(tr("Send to Device"));
    m_clearHistoryButton->setText(tr("Clear History"));

    m_wifiStatusLabel->setText(tr("Ready to send WiFi configuration to device"));
    m_deviceStatusLabel->setText(tr("💻 Ensure your device is connected via USB"));
}

void WiFiSetupWidget::onSendConfigClicked()
{
    QString wifiName = m_wifiNameEdit->text().trimmed();
    QString wifiPassword = m_wifiPasswordEdit->text();

    if (wifiName.isEmpty()) {
        m_wifiStatusLabel->setText(tr("❌ Please enter a WiFi network name"));
        m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #d32f2f; }");
        return;
    }

    if (wifiPassword.isEmpty()) {
        m_wifiStatusLabel->setText(tr("❌ Please enter a WiFi password"));
        m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #d32f2f; }");
        return;
    }

    // 显示发送中状态
    m_wifiStatusLabel->setText(tr("📡 Sending WiFi configuration to device..."));
    m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #ff9800; }");

    // 模拟发送过程
    QTimer::singleShot(2000, [this, wifiName]() {
        m_wifiStatusLabel->setText(tr("✅ Configuration sent successfully!\nDevice will connect to: %1").arg(wifiName));
        m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #388e3c; }");

        // 添加到历史记录
        addToHistory(wifiName);

        // 发射成功信号
        emit configurationSuccess(m_deviceType, wifiName);
    });
}

void WiFiSetupWidget::onClearClicked()
{
    m_wifiNameEdit->clear();
    m_wifiPasswordEdit->clear();
    m_showPasswordCheckBox->setChecked(false);

    // 重置状态标签
    m_wifiStatusLabel->setText(tr("Ready to send WiFi configuration to device"));
    m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");

    // 重新验证输入
    validateInputs();
}

void WiFiSetupWidget::onShowPasswordToggled(bool checked)
{
    m_wifiPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void WiFiSetupWidget::onHistoryItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    // 从历史记录中获取WiFi网络名称
    QString wifiName = item->data(Qt::UserRole).toString();
    if (!wifiName.isEmpty()) {
        m_wifiNameEdit->setText(wifiName);
        m_wifiPasswordEdit->setFocus(); // 焦点移到密码输入框

        // 更新状态
        m_wifiStatusLabel->setText(tr("📝 Selected from history: %1").arg(wifiName));
        m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #2196F3; }");
    }
}

void WiFiSetupWidget::onClearHistoryClicked()
{
        m_historyListWidget->clear();

        // 显示清除成功提示
        m_wifiStatusLabel->setText(tr("🗑️ WiFi history cleared successfully"));
        m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #4CAF50; }");

        // 3秒后恢复默认状态
        QTimer::singleShot(3000, [this]() {
            m_wifiStatusLabel->setText(tr("Ready to send WiFi configuration to device"));
            m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
        });
}

void WiFiSetupWidget::validateInputs()
{
    QString wifiName = m_wifiNameEdit->text().trimmed();
    QString wifiPassword = m_wifiPasswordEdit->text();

    // 启用/禁用发送按钮
    bool isValid = !wifiName.isEmpty() && !wifiPassword.isEmpty();
    m_sendConfigButton->setEnabled(isValid);

    // 更新发送按钮样式
    if (isValid) {
        m_sendConfigButton->setStyleSheet(
            "QPushButton#SendConfigButton {"
            "    background-color: #0070f9;"
            "    border: none;"
            "    border-radius: 6px;"
            "    padding: 8px 16px;"
            "    font-size: 13px;"
            "    color: white;"
            "    font-weight: bold;"
            "}"
            "QPushButton#SendConfigButton:hover {"
            "    background-color: #005acc;"
            "}"
        );
    } else {
        m_sendConfigButton->setStyleSheet(
            "QPushButton#SendConfigButton {"
            "    background-color: #cccccc;"
            "    border: none;"
            "    border-radius: 6px;"
            "    padding: 8px 16px;"
            "    font-size: 13px;"
            "    color: #888;"
            "    font-weight: bold;"
            "}"
        );
    }

    // 更新清除按钮状态
    bool hasContent = !wifiName.isEmpty() || !wifiPassword.isEmpty();
    m_clearButton->setEnabled(hasContent);

    if (hasContent) {
        m_clearButton->setStyleSheet(
            "QPushButton#ClearButton {"
            "    background-color: #f0f0f0;"
            "    border: 1px solid #ddd;"
            "    border-radius: 6px;"
            "    padding: 8px 16px;"
            "    font-size: 13px;"
            "    font-weight: 500;"
            "    color: #333;"
            "}"
            "QPushButton#ClearButton:hover {"
            "    background-color: #e0e0e0;"
            "}"
        );
    } else {
        m_clearButton->setStyleSheet(
            "QPushButton#ClearButton {"
            "    background-color: #f8f8f8;"
            "    border: 1px solid #eee;"
            "    border-radius: 6px;"
            "    padding: 8px 16px;"
            "    font-size: 13px;"
            "    font-weight: 500;"
            "    color: #999;"
            "}"
        );
    }
}

void WiFiSetupWidget::addToHistory(const QString &wifiName)
{
    // 检查是否已经存在于历史记录中
    if (isWifiNameInHistory(wifiName)) {
        // 如果已存在，移除旧记录并添加到顶部
        for (int i = 0; i < m_historyListWidget->count(); ++i) {
            QListWidgetItem *item = m_historyListWidget->item(i);
            if (item && item->data(Qt::UserRole).toString() == wifiName) {
                delete m_historyListWidget->takeItem(i);
                break;
            }
        }
    }

    // 创建新的历史记录项
    QListWidgetItem *newItem = new QListWidgetItem();
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
    newItem->setText(QString("📶 %1\n🕐 %2").arg(wifiName, currentTime));
    newItem->setData(Qt::UserRole, wifiName);

    // 添加到列表顶部
    m_historyListWidget->insertItem(0, newItem);

    // 限制历史记录数量（例如最多保存10条）
    const int maxHistoryItems = 10;
    while (m_historyListWidget->count() > maxHistoryItems) {
        delete m_historyListWidget->takeItem(m_historyListWidget->count() - 1);
    }

    // 高亮新添加的项目（可选）
    m_historyListWidget->setCurrentItem(newItem);

    // 显示添加成功的提示
    QTimer::singleShot(1000, [this, wifiName]() {
        m_wifiStatusLabel->setText(tr("📋 Added '%1' to connection history").arg(wifiName));
        m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #2196F3; }");

        // 3秒后恢复默认状态
        QTimer::singleShot(3000, [this]() {
            m_wifiStatusLabel->setText(tr("Ready to send WiFi configuration to device"));
            m_wifiStatusLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
        });
    });
}

bool WiFiSetupWidget::isWifiNameInHistory(const QString &wifiName)
{
    for (int i = 0; i < m_historyListWidget->count(); ++i) {
        QListWidgetItem *item = m_historyListWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == wifiName) {
            return true;
        }
    }
    return false;
}

FaceConfigWidget::FaceConfigWidget(const QString &deviceType, QWidget *parent)
    : QWidget(parent), m_deviceType(deviceType)
{
    setupUI();
    retranslateUI();
}

void FaceConfigWidget::setupUI()
{
    setObjectName("FaceConfigContent");

    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("WiFiScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    // 创建内容容器
    QWidget *contentContainer = new QWidget();
    contentContainer->setObjectName("FaceConfigContentContainer");
    contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout *mainLayout = new QVBoxLayout(contentContainer);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    // 标题
    m_titleLabel = new QLabel();
    m_titleLabel->setObjectName("WiFiSetupTitle");  // 替代 FaceConfigTitle
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setFixedHeight(32);

    // 描述
    m_descLabel = new QLabel();
    m_descLabel->setObjectName("WiFiSetupDesc");    // 替代 FaceConfigDesc
    m_descLabel->setAlignment(Qt::AlignCenter);
    m_descLabel->setWordWrap(true);
    m_descLabel->setFixedHeight(40);

    // 创建主要内容布局
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // 左侧：图像预览区域
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(10);

    // 图像预览标签
    m_previewLabel = new QLabel();
    m_previewLabel->setObjectName("FacePreviewLabel");
    m_previewLabel->setFixedSize(280, 280);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(
        "QLabel#FacePreviewLabel {"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 8px;"
        "    background-color: #f8f8f8;"
        "    color: #666;"
        "    font-size: 14px;"
        "}"
    );
    m_previewLabel->setText("Preview Image\n(280 x 280)");

    // IP显示控件
    QGroupBox *ipGroupBox = new QGroupBox();
    ipGroupBox->setObjectName("IPGroupBox");
    ipGroupBox->setTitle("Device Information");
    ipGroupBox->setFixedHeight(60);

    QHBoxLayout *ipLayout = new QHBoxLayout(ipGroupBox);
    ipLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *ipLabel = new QLabel("IP Address:");
    ipLabel->setObjectName("IPLabel");
    ipLabel->setStyleSheet("QLabel { font-size: 12px; color: #333; }");

    m_ipDisplayLabel = new QLabel("192.168.1.100");
    m_ipDisplayLabel->setObjectName("IPDisplayLabel");
    m_ipDisplayLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #0070f9;"
        "    font-weight: bold;"
        "    background-color: #f0f8ff;"
        "    padding: 4px 8px;"
        "    border-radius: 4px;"
        "    border: 1px solid #cce7ff;"
        "}"
    );

    ipLayout->addWidget(ipLabel);
    ipLayout->addWidget(m_ipDisplayLabel);
    ipLayout->addStretch();

    leftLayout->addWidget(m_previewLabel);
    leftLayout->addWidget(ipGroupBox);
    leftLayout->addStretch();

    // 右侧：控制面板
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    // 亮度调整控件
    QGroupBox *brightnessGroupBox = new QGroupBox();
    brightnessGroupBox->setObjectName("BrightnessGroupBox");
    brightnessGroupBox->setTitle("Brightness Control");
    brightnessGroupBox->setFixedHeight(80);

    QVBoxLayout *brightnessLayout = new QVBoxLayout(brightnessGroupBox);
    brightnessLayout->setContentsMargins(10, 10, 10, 10);
    brightnessLayout->setSpacing(8);

    QHBoxLayout *brightnessControlLayout = new QHBoxLayout();

    QLabel *brightnessMinLabel = new QLabel("Dark");
    brightnessMinLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setObjectName("BrightnessSlider");
    m_brightnessSlider->setRange(0, 100);
    m_brightnessSlider->setValue(50);
    m_brightnessSlider->setFixedHeight(20);

    QLabel *brightnessMaxLabel = new QLabel("Bright");
    brightnessMaxLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_brightnessValueLabel = new QLabel("50%");
    m_brightnessValueLabel->setObjectName("BrightnessValueLabel");
    m_brightnessValueLabel->setStyleSheet("QLabel { font-size: 11px; color: #333; font-weight: bold; }");
    m_brightnessValueLabel->setFixedWidth(35);
    m_brightnessValueLabel->setAlignment(Qt::AlignCenter);

    // 连接亮度滑块信号
    connect(m_brightnessSlider, &QSlider::valueChanged, this, [this](int value) {
        m_brightnessValueLabel->setText(QString("%1%").arg(value));
        // TODO: 在这里可以添加实际的亮度控制逻辑
        // onBrightnessChanged(value);
    });

    brightnessControlLayout->addWidget(brightnessMinLabel);
    brightnessControlLayout->addWidget(m_brightnessSlider);
    brightnessControlLayout->addWidget(brightnessMaxLabel);
    brightnessControlLayout->addWidget(m_brightnessValueLabel);

    brightnessLayout->addLayout(brightnessControlLayout);

    // 旋转角度调整控件
    QGroupBox *rotationGroupBox = new QGroupBox();
    rotationGroupBox->setObjectName("RotationGroupBox");
    rotationGroupBox->setTitle("Rotation Control");
    rotationGroupBox->setFixedHeight(80);

    QVBoxLayout *rotationLayout = new QVBoxLayout(rotationGroupBox);
    rotationLayout->setContentsMargins(10, 10, 10, 10);
    rotationLayout->setSpacing(8);

    QHBoxLayout *rotationControlLayout = new QHBoxLayout();

    QLabel *rotationMinLabel = new QLabel("0°");
    rotationMinLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_rotationSlider = new QSlider(Qt::Horizontal);
    m_rotationSlider->setObjectName("RotationSlider");
    m_rotationSlider->setRange(0, 360);
    m_rotationSlider->setValue(0);
    m_rotationSlider->setFixedHeight(20);

    QLabel *rotationMaxLabel = new QLabel("360°");
    rotationMaxLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_rotationValueLabel = new QLabel("0°");
    m_rotationValueLabel->setObjectName("RotationValueLabel");
    m_rotationValueLabel->setStyleSheet("QLabel { font-size: 11px; color: #333; font-weight: bold; }");
    m_rotationValueLabel->setFixedWidth(35);
    m_rotationValueLabel->setAlignment(Qt::AlignCenter);

    // 连接旋转滑块信号
    connect(m_rotationSlider, &QSlider::valueChanged, this, [this](int value) {
        m_rotationValueLabel->setText(QString("%1°").arg(value));
        // TODO: 在这里可以添加实际的旋转控制逻辑
        // onRotationChanged(value);
    });

    rotationControlLayout->addWidget(rotationMinLabel);
    rotationControlLayout->addWidget(m_rotationSlider);
    rotationControlLayout->addWidget(rotationMaxLabel);
    rotationControlLayout->addWidget(m_rotationValueLabel);

    rotationLayout->addLayout(rotationControlLayout);

    // 性能模式选择控件
    QGroupBox *performanceGroupBox = new QGroupBox();
    performanceGroupBox->setObjectName("PerformanceGroupBox");
    performanceGroupBox->setTitle("Performance Mode");
    performanceGroupBox->setFixedHeight(65);

    QVBoxLayout *performanceLayout = new QVBoxLayout(performanceGroupBox);
    performanceLayout->setContentsMargins(10, 10, 10, 10);

    m_performanceModeComboBox = new QComboBox();
    m_performanceModeComboBox->setObjectName("PerformanceModeComboBox");
    m_performanceModeComboBox->setFixedHeight(30);
    m_performanceModeComboBox->addItem("Normal Mode");
    m_performanceModeComboBox->addItem("Power Saving Mode");
    m_performanceModeComboBox->addItem("Performance Mode");
    m_performanceModeComboBox->setCurrentIndex(0);

    performanceLayout->addWidget(m_performanceModeComboBox);

    // 校准按钮组
    QGroupBox *calibrationGroupBox = new QGroupBox();
    calibrationGroupBox->setObjectName("CalibrationGroupBox");
    calibrationGroupBox->setTitle("Calibration Controls");
    calibrationGroupBox->setFixedHeight(120);

    QVBoxLayout *calibrationLayout = new QVBoxLayout(calibrationGroupBox);
    calibrationLayout->setContentsMargins(10, 10, 10, 10);
    calibrationLayout->setSpacing(8);

    // 校准模式选择
    QHBoxLayout *calibrationModeLayout = new QHBoxLayout();
    QLabel *calibrationModeLabel = new QLabel("Mode:");
    calibrationModeLabel->setStyleSheet("QLabel { font-size: 12px; color: #333; }");
    calibrationModeLabel->setFixedWidth(40);

    m_calibrationModeComboBox = new QComboBox();
    m_calibrationModeComboBox->setObjectName("CalibrationModeComboBox");
    m_calibrationModeComboBox->setFixedHeight(28);
    m_calibrationModeComboBox->addItem("Quick Calibration");
    m_calibrationModeComboBox->addItem("Standard Calibration");
    m_calibrationModeComboBox->addItem("Precision Calibration");
    m_calibrationModeComboBox->setCurrentIndex(1);

    calibrationModeLayout->addWidget(calibrationModeLabel);
    calibrationModeLayout->addWidget(m_calibrationModeComboBox);

    QHBoxLayout *calibrationButtonLayout = new QHBoxLayout();
    calibrationButtonLayout->setSpacing(8);

    m_startCalibrationButton = new QPushButton();
    m_startCalibrationButton->setObjectName("StartCalibrationButton");
    m_startCalibrationButton->setFixedHeight(32);
    m_startCalibrationButton->setText("Start");
    // 移除 setStyleSheet，使用 QSS 中的样式

    m_stopCalibrationButton = new QPushButton();
    m_stopCalibrationButton->setObjectName("StopCalibrationButton");
    m_stopCalibrationButton->setFixedHeight(32);
    m_stopCalibrationButton->setText("Stop");
    m_stopCalibrationButton->setEnabled(false);
    // 移除 setStyleSheet，使用 QSS 中的样式

    m_resetCalibrationButton = new QPushButton();
    m_resetCalibrationButton->setObjectName("ResetCalibrationButton");
    m_resetCalibrationButton->setFixedHeight(32);
    m_resetCalibrationButton->setText("Reset");
    // 移除 setStyleSheet，使用 QSS 中的样式

    calibrationButtonLayout->addWidget(m_startCalibrationButton);
    calibrationButtonLayout->addWidget(m_stopCalibrationButton);
    calibrationButtonLayout->addWidget(m_resetCalibrationButton);

    calibrationLayout->addLayout(calibrationModeLayout);
    calibrationLayout->addLayout(calibrationButtonLayout);

    // 添加所有控件到右侧布局
    rightLayout->addWidget(brightnessGroupBox);
    rightLayout->addWidget(rotationGroupBox);
    rightLayout->addWidget(performanceGroupBox);
    rightLayout->addWidget(calibrationGroupBox);
    rightLayout->addStretch();

    // 添加左右布局到内容布局
    contentLayout->addLayout(leftLayout, 1);
    contentLayout->addLayout(rightLayout, 1);

    // 添加到主布局
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(m_descLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(contentLayout);

    // 设置滚动区域
    scrollArea->setWidget(contentContainer);

    // 创建最终容器
    QVBoxLayout *finalLayout = new QVBoxLayout(this);
    finalLayout->setContentsMargins(0, 0, 0, 0);
    finalLayout->addWidget(scrollArea);
}

void FaceConfigWidget::retranslateUI()
{
    m_titleLabel->setText(tr("Configure %1 Settings").arg(m_deviceType));
    m_descLabel->setText(tr("Adjust camera settings and calibrate your %1 device.\n"
                           "Monitor the live preview and fine-tune parameters for optimal performance.").arg(m_deviceType));
}

void MainWindow::scanForDevices() {
    try {
        // 初始化并打开串口
        m_serialManager->init();
        if (m_serialManager->status() == SerialStatus::OPENED) {
            // 启动心跳，保持连接
            m_serialManager->start_heartbeat_timer();

            // 显示搜索状态
            QTimer::singleShot(5000, [this]() {
                // 5秒后如果没有收到任何设备响应，显示未找到设备的消息
                if (deviceTabs.isEmpty()) {
                    showNoDeviceFoundMessage();
                }
            });
        } else {
            showNoDeviceFoundMessage();
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("错误"),
            tr("设备搜索过程中发生错误: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::critical(this, tr("错误"),
            tr("设备搜索过程中发生未知错误"));
    }
}

void MainWindow::updateDeviceStatus(const QString &deviceName, const QString &ipAddress, int batteryLevel) {
    // 更新设备内容页面中的状态显示
    if (deviceContentPages.contains(deviceName)) {
        QWidget *contentPage = deviceContentPages[deviceName];

        // 查找并更新IP标签
        QLabel *ipLabel = contentPage->findChild<QLabel*>("deviceIPLabel");
        if (ipLabel) {
            ipLabel->setText(QString("IP Address: %1").arg(ipAddress));
        }

        // 查找并更新电量标签
        QLabel *batteryLabel = contentPage->findChild<QLabel*>("deviceBatteryLabel");
        if (batteryLabel) {
            batteryLabel->setText(QString("Battery: %1%").arg(batteryLevel));
        }

        // 更新连接状态
        QLabel *statusLabel = contentPage->findChild<QLabel*>("deviceStatusLabel");
        if (statusLabel) {
            statusLabel->setText("Status: Connected");
            statusLabel->setStyleSheet("color: green; font-weight: bold;");
        }
    }
}

void MainWindow::showNoDeviceFoundMessage() {
    // 在界面上显示未找到设备的提示
    QMessageBox::information(this, tr("设备搜索"),
        tr("未发现连接的设备。\n请确保设备已通过USB连接并正确安装驱动程序。"));
}

void MainWindow::onScanDevicesButtonClicked() {
    // 使用指针而不是局部变量
    QProgressDialog *progress = new QProgressDialog(tr("正在搜索设备..."), tr("取消"), 0, 0, this);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progress->show();

    // 在后台线程中执行设备搜索
    QTimer::singleShot(100, [this, progress]() {
        scanForDevices();
        progress->close();
        progress->deleteLater();  // 安全删除
    });
}


void MainWindow::onDeviceNeedsWifiConfig(const QString &deviceName, const QString &deviceType) {
    // 显示WiFi配置需求对话框
    int result = QMessageBox::question(this, tr("设备配网"),
        tr("发现设备: %1\n该设备尚未连接WiFi，需要进行配网。\n是否立即开始WiFi配置？")
        .arg(deviceName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        // 自动启动WiFi配置流程
        startDeviceSetupFlow(deviceType);

        // 临时添加设备到侧边栏（显示为"配置中"状态）
        addDeviceTabWithConfigStatus(deviceName, deviceType, "配置中");
    } else {
        // 用户选择不配置，但仍可以添加到设备列表（显示为"未配置"状态）
        addDeviceTabWithConfigStatus(deviceName, deviceType, "未配置");
    }
}

void MainWindow::addDeviceTabWithConfigStatus(const QString &deviceName, const QString &deviceType, const QString &status) {
    // 如果设备已存在，先移除
    if (deviceTabs.contains(deviceName)) {
        removeDeviceTab(deviceName);
    }

    // 确定设备图标
    QString iconPath;
    if (deviceType.contains("Face", Qt::CaseInsensitive)) {
        iconPath = ":/resources/resources/images/vr-cardboard-solid-full.png";
    } else if (deviceType.contains("Eye", Qt::CaseInsensitive)) {
        iconPath = ":/resources/resources/images/face-smile-regular-full.png";
    } else {
        iconPath = ":/resources/resources/images/vr-cardboard-solid-full.png";
    }

    // 创建设备tab
    QWidget *deviceTab = new QWidget();
    deviceTab->setObjectName("SiderBarItem");
    deviceTab->setProperty("selected", false);
    deviceTab->setProperty("itemText", deviceName);

    QHBoxLayout *itemLayout = new QHBoxLayout(deviceTab);
    itemLayout->setContentsMargins(10, 15, 10, 15);
    itemLayout->setSpacing(10);

    // 图标标签
    QLabel *iconLabel = new QLabel();
    iconLabel->setObjectName("sidebarIconLabel");
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        iconLabel->setFixedSize(24, 24);
    }

    // 文本标签
    QLabel *textLabel = new QLabel(deviceName);
    textLabel->setObjectName("sidebarTextLabel");

    // 状态指示器
    QLabel *statusDot = new QLabel();
    statusDot->setObjectName("deviceStatusDot");
    statusDot->setFixedSize(8, 8);

    // 根据状态设置不同颜色
    QString statusColor;
    if (status == "配置中") {
        statusColor = "#FF9800"; // 橙色表示配置中
    } else if (status == "未配置") {
        statusColor = "#F44336"; // 红色表示未配置
    } else {
        statusColor = "#4CAF50"; // 绿色表示已连接
    }

    statusDot->setStyleSheet(QString(
        "QLabel#deviceStatusDot {"
        "    background-color: %1;"
        "    border-radius: 4px;"
        "}"
    ).arg(statusColor));

    // 添加到布局
    itemLayout->addWidget(iconLabel);
    itemLayout->addWidget(textLabel);
    itemLayout->addStretch();
    itemLayout->addWidget(statusDot);

    // 插入到分隔线之前
    int separatorIndex = -1;
    for (int i = 0; i < sidebarLayout->count(); ++i) {
        QFrame *frame = qobject_cast<QFrame*>(sidebarLayout->itemAt(i)->widget());
        if (frame && frame->objectName() == "SidebarSeparator") {
            separatorIndex = i;
            break;
        }
    }

    if (separatorIndex != -1) {
        sidebarLayout->insertWidget(separatorIndex, deviceTab);
    } else {
        sidebarLayout->addWidget(deviceTab);
    }

    // 保存引用
    sidebarItems.append(deviceTab);
    deviceTabs[deviceName] = deviceTab;

    // 安装事件过滤器
    deviceTab->installEventFilter(this);
    iconLabel->installEventFilter(this);
    textLabel->installEventFilter(this);
    statusDot->installEventFilter(this);

    // 创建对应的内容页面
    QWidget *deviceContentPage = createDeviceContentPageWithStatus(deviceName, deviceType, status);
    contentStack->addWidget(deviceContentPage);
    deviceContentPages[deviceName] = deviceContentPage;
}

QWidget* MainWindow::createDeviceContentPageWithStatus(const QString &deviceName, const QString &deviceType, const QString &status) {
    QWidget *contentPage = new QWidget();
    contentPage->setObjectName("deviceContentPage");

    QVBoxLayout *layout = new QVBoxLayout(contentPage);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    // 设备标题
    QLabel *titleLabel = new QLabel(deviceName);
    titleLabel->setObjectName("deviceTitleLabel");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");
    layout->addWidget(titleLabel);

    // 设备类型
    QLabel *typeLabel = new QLabel(QString("Type: %1").arg(deviceType));
    typeLabel->setObjectName("deviceTypeLabel");
    typeLabel->setStyleSheet("font-size: 14px; color: #666;");
    layout->addWidget(typeLabel);

    // 连接状态
    QLabel *statusLabel = new QLabel(QString("Status: %1").arg(status));
    statusLabel->setObjectName("deviceStatusLabel");

    QString statusColor;
    if (status == "配置中") {
        statusColor = "#FF9800";
    } else if (status == "未配置") {
        statusColor = "#F44336";
    } else {
        statusColor = "#4CAF50";
    }

    statusLabel->setStyleSheet(QString("font-size: 14px; color: %1; font-weight: bold;").arg(statusColor));
    layout->addWidget(statusLabel);

    // 根据状态显示不同内容
    if (status == "未配置") {
        // 显示配网按钮
        QPushButton *configWifiBtn = new QPushButton("开始WiFi配置");
        configWifiBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #2196F3;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 8px;"
            "    padding: 12px 24px;"
            "    font-size: 14px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #1976D2;"
            "}"
        );

        connect(configWifiBtn, &QPushButton::clicked, [this, deviceType]() {
            startDeviceSetupFlow(deviceType);
        });

        layout->addWidget(configWifiBtn);
        layout->addStretch();
    } else if (status == "配置中") {
        // 显示配置进度
        QLabel *configLabel = new QLabel("正在进行WiFi配置，请按照引导完成设置...");
        configLabel->setStyleSheet("font-size: 14px; color: #FF9800; font-style: italic;");
        layout->addWidget(configLabel);
        layout->addStretch();
    } else {
        // 正常的设备配置界面
        QLabel *ipLabel = new QLabel("IP Address: Unknown");
        ipLabel->setObjectName("deviceIPLabel");
        ipLabel->setStyleSheet("font-size: 14px; color: #666;");
        layout->addWidget(ipLabel);

        QLabel *batteryLabel = new QLabel("Battery: Unknown");
        batteryLabel->setObjectName("deviceBatteryLabel");
        batteryLabel->setStyleSheet("font-size: 14px; color: #666;");
        layout->addWidget(batteryLabel);

        // 分隔线
        QFrame *line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);

        // 设备配置界面
        QWidget *configWidget = nullptr;
        if (deviceType == "Face Tracker") {
            configWidget = new FaceConfigWidget(deviceType, this);
        } else if (deviceType == "Eye Tracker") {
            configWidget = createEyeTrackerConfig(deviceName, deviceType);
        } else {
            configWidget = createGenericDeviceConfig(deviceName, deviceType);
        }

        layout->addWidget(configWidget);
        layout->addStretch();
    }

    return contentPage;
}

