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
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName("MainPage");
    // 设置窗口为无边框
    setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    // 设置窗口属性以支持透明背景
    setAttribute(Qt::WA_TranslucentBackground);

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

    // 创建侧边栏项
    createSidebarItem(":/resources/resources/images/vr-cardboard-solid-full.png", "Face Tracker");
    createSidebarItem(":/resources/resources/images/face-smile-regular-full.png", "Eye Tracker");
    
    // 添加弹性空间，将底部区域推到底部
    sidebarLayout->addStretch();

    // 添加分隔线
    createSidebarSeparator();

    // 创建设备状态区域（底部）
    createDeviceStatusSection();
    
    // 添加分隔线
    createSidebarSeparator();
    
    // 创建已连接设备列表（底部）
    createConnectedDevicesSection();
}

void MainWindow::updateDeviceCount() {
    int count = connectedDevicesList->count();
    QLabel *deviceCount = sidebarWidget->findChild<QLabel*>("DeviceCount");
    if (deviceCount) {
        deviceCount->setText(QString("(%1)").arg(count));
    }
}

void MainWindow::checkUSBConnection() {
    // 模拟USB设备检测逻辑
    // 在实际应用中，这里应该实现真实的USB设备检测
    static bool simulatedConnection = false;
    static int checkCount = 0;
    
    checkCount++;
    
    // 模拟连接状态变化
    if (checkCount % 4 == 0) {
        simulatedConnection = !simulatedConnection;
        updateUSBStatus(simulatedConnection);
        
        // 当USB状态改变时，也更新设备列表
        if (simulatedConnection) {
            // 模拟新设备连接
            if (connectedDevicesList->count() < 3) {
                addConnectedDevice(QString("New Device #%1").arg(QDateTime::currentDateTime().toString("mmss")), "Connected", true);
                updateDeviceCount();
            }
        }
    }
}

// 在WiFi配置成功后，添加设备到列表的功能
void MainWindow::onWiFiConfigurationSuccess(const QString &deviceType, const QString &wifiName) {
    // 当WiFi配置成功时，添加设备到连接列表
    QString deviceId = QDateTime::currentDateTime().toString("hhmmss");
    QString deviceName = QString("%1 #%2").arg(deviceType, deviceId);
    
    addConnectedDevice(deviceName, "Connected via WiFi", true);
    updateDeviceCount();
    
    // 更新USB状态为已连接
    updateUSBStatus(true);
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
    defaultContentWidget = new QWidget();
    defaultContentWidget->setObjectName("DefaultContent");
    
    QVBoxLayout *layout = new QVBoxLayout(defaultContentWidget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(30);
    
    // 创建欢迎标签
    welcomeLabel = new QLabel("Welcome to PaperTracker");
    welcomeLabel->setObjectName("WelcomeLabel");
    welcomeLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #333; margin: 20px; }");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    
    // 创建描述标签
    descLabel = new QLabel("Choose a tracker to get started with your setup.");
    descLabel->setObjectName("DescLabel");
    descLabel->setStyleSheet("QLabel { font-size: 16px; color: #666; margin: 10px; }");
    descLabel->setAlignment(Qt::AlignCenter);
    
    // 创建按钮容器
    buttonContainer = new QWidget();
    buttonContainer->setObjectName("ButtonContainer");
    buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setSpacing(30);
    buttonLayout->setAlignment(Qt::AlignCenter);
    
    // Face Tracker 按钮
    faceTrackerButton = new QPushButton();
    faceTrackerButton->setObjectName("FaceTrackerButton");
    faceTrackerButton->setFixedSize(200, 120);
    faceTrackerButton->setCursor(Qt::PointingHandCursor);
    
    // 创建 Face Tracker 按钮内容
    faceButtonWidget = new QWidget();
    faceButtonWidget->setObjectName("FaceButtonWidget");
    QVBoxLayout *faceButtonLayout = new QVBoxLayout(faceButtonWidget);
    faceButtonLayout->setAlignment(Qt::AlignCenter);
    faceButtonLayout->setSpacing(10);
    
    faceIcon = new QLabel();
    faceIcon->setObjectName("FaceTrackerIcon");
    QPixmap facePixmap(":/resources/resources/images/vr-cardboard-solid-full.png");
    if (!facePixmap.isNull()) {
        faceIcon->setPixmap(facePixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        faceIcon->setAlignment(Qt::AlignCenter);
    }
    
    faceText = new QLabel("Face Tracker");
    faceText->setObjectName("FaceTrackerText");
    faceText->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #333; }");
    faceText->setAlignment(Qt::AlignCenter);
    
    faceDesc = new QLabel("Setup face tracking");
    faceDesc->setObjectName("FaceTrackerDesc");
    faceDesc->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
    faceDesc->setAlignment(Qt::AlignCenter);
    
    faceButtonLayout->addWidget(faceIcon);
    faceButtonLayout->addWidget(faceText);
    faceButtonLayout->addWidget(faceDesc);
    
    // Eye Tracker 按钮
    eyeTrackerButton = new QPushButton();
    eyeTrackerButton->setObjectName("EyeTrackerButton");
    eyeTrackerButton->setFixedSize(200, 120);
    eyeTrackerButton->setCursor(Qt::PointingHandCursor);
    
    // 创建 Eye Tracker 按钮内容
    eyeButtonWidget = new QWidget();
    eyeButtonWidget->setObjectName("EyeButtonWidget");
    QVBoxLayout *eyeButtonLayout = new QVBoxLayout(eyeButtonWidget);
    eyeButtonLayout->setAlignment(Qt::AlignCenter);
    eyeButtonLayout->setSpacing(10);
    
    eyeIcon = new QLabel();
    eyeIcon->setObjectName("EyeTrackerIcon");
    QPixmap eyePixmap(":/resources/resources/images/face-smile-regular-full.png");
    if (!eyePixmap.isNull()) {
        eyeIcon->setPixmap(eyePixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        eyeIcon->setAlignment(Qt::AlignCenter);
    }
    
    eyeText = new QLabel("Eye Tracker");
    eyeText->setObjectName("EyeTrackerText");
    eyeText->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #333; }");
    eyeText->setAlignment(Qt::AlignCenter);
    
    eyeDesc = new QLabel("Setup eye tracking");
    eyeDesc->setObjectName("EyeTrackerDesc");
    eyeDesc->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
    eyeDesc->setAlignment(Qt::AlignCenter);
    
    eyeButtonLayout->addWidget(eyeIcon);
    eyeButtonLayout->addWidget(eyeText);
    eyeButtonLayout->addWidget(eyeDesc);
    
    // 设置按钮样式
    QString buttonStyle = 
        "QPushButton {"
        "    background-color: white;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 12px;"
        "    padding: 20px;"
        "}"
        "QPushButton:hover {"
        "    border-color: #0070f9;"
        "    background-color: #f8f9ff;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #e7ebf0;"
        "}";
    
    faceTrackerButton->setStyleSheet(buttonStyle);
    eyeTrackerButton->setStyleSheet(buttonStyle);
    
    // 将内容添加到按钮
    QVBoxLayout *faceLayout = new QVBoxLayout(faceTrackerButton);
    faceLayout->addWidget(faceButtonWidget);
    faceLayout->setContentsMargins(0, 0, 0, 0);
    
    QVBoxLayout *eyeLayout = new QVBoxLayout(eyeTrackerButton);
    eyeLayout->addWidget(eyeButtonWidget);
    eyeLayout->setContentsMargins(0, 0, 0, 0);
    
    // 为按钮安装事件过滤器以实现更丰富的 hover 效果
    faceTrackerButton->installEventFilter(this);
    eyeTrackerButton->installEventFilter(this);
    
    // 连接按钮点击事件
    connect(faceTrackerButton, &QPushButton::clicked, this, [this]() {
        // 设置侧边栏选中状态
        for (QWidget *item : sidebarItems) {
            if (item->property("itemText").toString() == "Face Tracker") {
                setSelectedItem(item);
                break;
            }
        }
        onSidebarItemClicked("Face Tracker");
    });
    
    connect(eyeTrackerButton, &QPushButton::clicked, this, [this]() {
        // 设置侧边栏选中状态
        for (QWidget *item : sidebarItems) {
            if (item->property("itemText").toString() == "Eye Tracker") {
                setSelectedItem(item);
                break;
            }
        }
        onSidebarItemClicked("Eye Tracker");
    });
    
    // 添加按钮到布局
    buttonLayout->addWidget(faceTrackerButton);
    buttonLayout->addWidget(eyeTrackerButton);
    
    // 添加所有元素到主布局
    layout->addWidget(welcomeLabel);
    layout->addWidget(descLabel);
    layout->addSpacing(20);
    layout->addWidget(buttonContainer);
    layout->addStretch();
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
    if (itemText == "Face Tracker") {
        contentStack->setCurrentWidget(faceGuideWidget);
        faceGuideWidget->setCurrentStep(1); // 重置到第一步
    } else if (itemText == "Eye Tracker") {
        contentStack->setCurrentWidget(eyeGuideWidget);
        eyeGuideWidget->setCurrentStep(1); // 重置到第一步
    } else {
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
     WiFiSetupWidget *wifiWidget = new WiFiSetupWidget(deviceType, this);
    
    // 连接配置成功信号
    connect(wifiWidget, &WiFiSetupWidget::configurationSuccess, 
            this, &MainWindow::onWiFiConfigurationSuccess);
    
    return wifiWidget;
}

QWidget* MainWindow::createFaceConfigStep(const QString &deviceType) {
    FaceConfigWidget *faceWidget = new FaceConfigWidget(deviceType, this);

    return faceWidget;
}

void MainWindow::createDeviceStatusSection() {
    // USB连接状态区域
    usbStatusWidget = new QWidget();
    usbStatusWidget->setObjectName("USBStatusWidget");
    usbStatusWidget->setFixedHeight(75);
    
    QVBoxLayout *usbLayout = new QVBoxLayout(usbStatusWidget);
    usbLayout->setContentsMargins(15, 10, 15, 10);
    usbLayout->setSpacing(6);
    
    // 标题
    QLabel *usbTitle = new QLabel("USB Connection");
    usbTitle->setObjectName("USBSectionTitle");
    
    // 状态容器
    QWidget *statusContainer = new QWidget();
    statusContainer->setObjectName("USBStatusContainer");
    QHBoxLayout *statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(10);
    
    // 状态图标
    usbStatusIcon = new QLabel();
    usbStatusIcon->setObjectName("USBStatusIcon");
    usbStatusIcon->setFixedSize(18, 18);
    updateUSBStatus(false); // 初始状态为未连接
    
    // 状态文本
    usbStatusLabel = new QLabel("No device connected");
    usbStatusLabel->setObjectName("USBStatusLabel");
    
    statusLayout->addWidget(usbStatusIcon);
    statusLayout->addWidget(usbStatusLabel);
    statusLayout->addStretch();
    
    usbLayout->addWidget(usbTitle);
    usbLayout->addWidget(statusContainer);
    
    sidebarLayout->addWidget(usbStatusWidget);
    
    // 模拟USB状态检测
    QTimer *usbCheckTimer = new QTimer(this);
    connect(usbCheckTimer, &QTimer::timeout, this, &MainWindow::checkUSBConnection);
    usbCheckTimer->start(3000);
}

void MainWindow::createConnectedDevicesSection() {
    // 已连接设备列表区域
    deviceListWidget = new QWidget();
    deviceListWidget->setObjectName("DeviceListWidget");
    
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceListWidget);
    deviceLayout->setContentsMargins(15, 10, 15, 12);
    deviceLayout->setSpacing(0);
    
    // 标题容器
    QWidget *titleContainer = new QWidget();
    titleContainer->setObjectName("DeviceTitleContainer");
    titleContainer->setFixedHeight(24);  // 设置固定高度，确保完整显示
    titleContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);  // 确保高度固定
    
    QHBoxLayout *titleLayout = new QHBoxLayout(titleContainer);
    titleLayout->setContentsMargins(0, 2, 0, 2);  // 调整内边距，给文字留出适当空间
    titleLayout->setSpacing(6);
    
    QLabel *deviceTitle = new QLabel("Connected Devices");
    deviceTitle->setObjectName("DeviceSectionTitle");
    
    // 设备数量标签
    QLabel *deviceCount = new QLabel("(0)");
    deviceCount->setObjectName("DeviceCount");
    
    titleLayout->addWidget(deviceTitle);
    titleLayout->addWidget(deviceCount);
    titleLayout->addStretch();
    
    // 设备列表
    connectedDevicesList = new QListWidget();
    connectedDevicesList->setObjectName("ConnectedDevicesList");
    connectedDevicesList->setMaximumHeight(140);
    connectedDevicesList->setMinimumHeight(100);
    
    // 添加示例设备
    addConnectedDevice("Face Tracker #001", "Connected", true);
    addConnectedDevice("Eye Tracker #002", "Connecting...", false);
    
    // 更新设备数量
    updateDeviceCount();
    
    deviceLayout->addWidget(titleContainer);
    deviceLayout->addWidget(connectedDevicesList);
    
    sidebarLayout->addWidget(deviceListWidget);
}

void MainWindow::addConnectedDevice(const QString &deviceName, const QString &status, bool isConnected) {
    QListWidgetItem *item = new QListWidgetItem();
    
    // 创建自定义widget作为列表项
    QWidget *deviceWidget = new QWidget();
    deviceWidget->setObjectName("DeviceItem");
    
    QVBoxLayout *itemLayout = new QVBoxLayout(deviceWidget);
    itemLayout->setContentsMargins(4, 4, 4, 4);
    itemLayout->setSpacing(3);
    
    // 设备名称
    QLabel *nameLabel = new QLabel(deviceName);
    nameLabel->setObjectName("DeviceNameLabel");
    
    // 状态容器
    QWidget *statusContainer = new QWidget();
    statusContainer->setObjectName("DeviceStatusContainer");
    QHBoxLayout *statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(5);
    
    // 状态图标 - 保留动态样式（颜色根据连接状态变化）
    QLabel *statusIcon = new QLabel();
    statusIcon->setFixedSize(10, 10);
    statusIcon->setStyleSheet(QString(
        "QLabel {"
        "    background-color: %1;"
        "    border-radius: 5px;"
        "}"
    ).arg(isConnected ? "#4CAF50" : "#FF9800"));
    
    // 状态文本 - 保留动态样式（颜色根据连接状态变化）
    QLabel *statusLabel = new QLabel(status);
    statusLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-size: 10px;"
        "    color: %1;"
        "    margin: 0;"
        "}"
    ).arg(isConnected ? "#4CAF50" : "#FF9800"));
    
    statusLayout->addWidget(statusIcon);
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    itemLayout->addWidget(nameLabel);
    itemLayout->addWidget(statusContainer);
    
    // 设置item尺寸
    QSize itemSize = deviceWidget->sizeHint();
    itemSize.setHeight(qMax(itemSize.height(), 56));
    item->setSizeHint(itemSize);
    connectedDevicesList->addItem(item);
    connectedDevicesList->setItemWidget(item, deviceWidget);
    
    // 存储设备信息
    item->setData(Qt::UserRole, deviceName);
    item->setData(Qt::UserRole + 1, isConnected);
}

void MainWindow::updateUSBStatus(bool connected) {
    if (!usbStatusIcon || !usbStatusLabel) return;
    
    if (connected) {
        // 连接状态
        usbStatusIcon->setStyleSheet(
            "QLabel#USBStatusIcon {"
            "    background-color: #4CAF50;"
            "    border-radius: 9px;"
            "}"
        );
        usbStatusLabel->setText("USB Device Connected");
        usbStatusLabel->setStyleSheet(
            "QLabel#USBStatusLabel {"
            "    font-size: 12px;"
            "    color: #4CAF50;"
            "    font-weight: bold;"
            "    margin: 0;"
            "    background-color: #f0f0f0;"  // 确保背景色一致
            "}"
        );
    } else {
        // 未连接状态
        usbStatusIcon->setStyleSheet(
            "QLabel#USBStatusIcon {"
            "    background-color: #f44336;"
            "    border-radius: 9px;"
            "}"
        );
        usbStatusLabel->setText("No device connected");
        usbStatusLabel->setStyleSheet(
            "QLabel#USBStatusLabel {"
            "    font-size: 12px;"
            "    color: #777;"
            "    margin: 0;"
            "    background-color: #f0f0f0;"  // 确保背景色一致
            "}"
        );
    }
}

void MainWindow::createSidebarSeparator() {
    QFrame *separator = new QFrame();
    separator->setObjectName("SidebarSeparator");
    separator->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    separator->setFixedHeight(1);
    sidebarLayout->addWidget(separator);
}

// WiFiSetupWidget 实现
WiFiSetupWidget::WiFiSetupWidget(const QString &deviceType, QWidget *parent)
    : QWidget(parent), m_deviceType(deviceType)
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

