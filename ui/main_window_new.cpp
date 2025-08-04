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
    
    // 添加分隔线
    createSidebarSeparator();
    
    // 创建设备状态区域
    createDeviceStatusSection();
    
    // 添加分隔线
    createSidebarSeparator();
    
    // 创建已连接设备列表
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
    
    // 为 Face Tracker 设置其他步骤内容
    for (int i = 2; i <= 4; ++i) {
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
    prevButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f0f0f0;"
        "    border: 1px solid #ddd;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-size: 13px;"  // 稍微减小字体
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #f8f8f8;"
        "    color: #999;"
        "}"
    );
    
    nextButton = new QPushButton("Next");
    nextButton->setObjectName("GuideNextButton");
    nextButton->setFixedHeight(36);  // 固定按钮高度
    nextButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #0070f9;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-size: 13px;"  // 稍微减小字体
        "    color: white;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #005acc;"
        "}"
    );
    
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
    QWidget *wifiContent = new QWidget();
    wifiContent->setObjectName("WiFiSetupContent");
    
    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("WiFiScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { "
        "    background-color: transparent; "
        "    border: none; "
        "}"
        "QScrollBar:vertical {"
        "    border: none;"
        "    background: rgba(0,0,0,0);"
        "    width: 8px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(128,128,128,0.3);"
        "    border-radius: 4px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: rgba(128,128,128,0.5);"
        "}"
    );
    
    // 创建内容容器
    QWidget *contentContainer = new QWidget();
    contentContainer->setObjectName("WiFiContentContainer");
    contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(contentContainer);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(12);  // 恢复合适的间距
    mainLayout->setContentsMargins(20, 15, 20, 15);  // 恢复合适的边距
    
    // 标题 - 恢复合适大小
    QLabel *titleLabel = new QLabel(QString("Configure %1 WiFi Settings").arg(deviceType));
    titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #333; margin: 0; }");  // 恢复合适字体
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFixedHeight(28);  // 恢复合适高度
    
    // 描述 - 恢复合适大小
    QLabel *descLabel = new QLabel(QString("Enter your WiFi network credentials to configure the %1 device.\nThe settings will be sent to the device via USB connection.").arg(deviceType));
    descLabel->setStyleSheet("QLabel { font-size: 13px; color: #666; line-height: 1.3; margin: 0; }");  // 恢复合适字体
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    descLabel->setFixedHeight(35);  // 恢复合适高度
    
    // 创建水平布局
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(15);  // 恢复合适间距
    
    // WiFi 设置表单 (左侧)
    QGroupBox *wifiGroupBox = new QGroupBox("WiFi Configuration");
    wifiGroupBox->setStyleSheet(
        "QGroupBox {"
        "    font-size: 13px;"  // 恢复合适字体
        "    font-weight: bold;"
        "    color: #333;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 8px;"
        "    margin-top: 8px;"  // 恢复合适边距
        "    padding-top: 8px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 6px 0 6px;"  // 恢复合适内边距
        "    background-color: white;"
        "}"
    );
    
    QFormLayout *formLayout = new QFormLayout(wifiGroupBox);
    formLayout->setSpacing(10);  // 恢复合适间距
    formLayout->setContentsMargins(15, 15, 15, 15);  // 恢复合适边距
    
    // WiFi 网络名称输入
    QLineEdit *wifiNameEdit = new QLineEdit();
    wifiNameEdit->setObjectName("WiFiNameEdit");
    wifiNameEdit->setPlaceholderText("Enter WiFi network name");
    wifiNameEdit->setFixedHeight(32);  // 恢复合适高度
    wifiNameEdit->setStyleSheet(
        "QLineEdit {"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"  // 恢复合适内边距
        "    font-size: 13px;"  // 恢复合适字体
        "    background-color: white;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #0070f9;"
        "    outline: none;"
        "}"
    );
    
    // WiFi 密码输入
    QLineEdit *wifiPasswordEdit = new QLineEdit();
    wifiPasswordEdit->setObjectName("WiFiPasswordEdit");
    wifiPasswordEdit->setPlaceholderText("Enter WiFi password");
    wifiPasswordEdit->setEchoMode(QLineEdit::Password);
    wifiPasswordEdit->setFixedHeight(32);  // 恢复合适高度
    wifiPasswordEdit->setStyleSheet(
        "QLineEdit {"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    padding: 6px 10px;"  // 恢复合适内边距
        "    font-size: 13px;"  // 恢复合适字体
        "    background-color: white;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #0070f9;"
        "    outline: none;"
        "}"
    );
    
    // 显示密码复选框
    QCheckBox *showPasswordCheckBox = new QCheckBox("Show password");
    showPasswordCheckBox->setObjectName("ShowPasswordCheckBox");
    showPasswordCheckBox->setStyleSheet(
        "QCheckBox {"
        "    font-size: 12px;"  // 恢复合适字体
        "    color: #666;"
        "}"
        "QCheckBox::indicator {"
        "    width: 14px;"  // 恢复合适尺寸
        "    height: 14px;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 3px;"
        "    background-color: white;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: #0070f9;"
        "    border-color: #0070f9;"
        "}"
        "QCheckBox::indicator:hover {"
        "    border-color: #0070f9;"
        "}"
    );
    
    // 连接显示密码功能
    connect(showPasswordCheckBox, &QCheckBox::toggled, this, [wifiPasswordEdit](bool checked) {
        wifiPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    
    // 设备状态指示器
    QLabel *statusLabel = new QLabel();
    statusLabel->setObjectName("WiFiStatusLabel");
    statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");  // 恢复合适字体
    statusLabel->setText("Ready to send WiFi configuration to device");
    statusLabel->setWordWrap(true);
    statusLabel->setMaximumHeight(50);  // 恢复合适高度
    
    // 按钮容器
    QWidget *buttonContainer = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);  // 恢复合适间距
    
    // 清除输入按钮
    QPushButton *clearButton = new QPushButton("Clear");
    clearButton->setObjectName("ClearButton");
    clearButton->setFixedHeight(30);  // 恢复合适高度
    clearButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f0f0f0;"
        "    border: 1px solid #ddd;"
        "    border-radius: 6px;"
        "    padding: 6px 12px;"  // 恢复合适内边距
        "    font-size: 13px;"  // 恢复合适字体
        "    color: #333;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;"
        "}"
    );
    
    // 发送配置按钮
    QPushButton *sendConfigButton = new QPushButton("Send to Device");
    sendConfigButton->setObjectName("SendConfigButton");
    sendConfigButton->setFixedHeight(30);  // 恢复合适高度
    sendConfigButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #0070f9;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 6px 12px;"  // 恢复合适内边距
        "    font-size: 13px;"  // 恢复合适字体
        "    color: white;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #005acc;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #004499;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #cccccc;"
        "    color: #999;"
        "}"
    );
    
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(sendConfigButton);
    
    // WiFi 历史列表 (右侧)
    QGroupBox *historyGroupBox = new QGroupBox("Connection History");
    historyGroupBox->setStyleSheet(
        "QGroupBox {"
        "    font-size: 13px;"  // 恢复合适字体
        "    font-weight: bold;"
        "    color: #333;"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 8px;"
        "    margin-top: 8px;"  // 恢复合适边距
        "    padding-top: 8px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 6px 0 6px;"  // 恢复合适内边距
        "    background-color: white;"
        "}"
    );
    historyGroupBox->setMaximumWidth(220);
    historyGroupBox->setMinimumWidth(180);
    
    QVBoxLayout *historyLayout = new QVBoxLayout(historyGroupBox);
    historyLayout->setContentsMargins(12, 12, 12, 12);  // 恢复合适边距
    historyLayout->setSpacing(8);  // 恢复合适间距
    
    // 创建历史列表
    QListWidget *historyListWidget = new QListWidget();
    historyListWidget->setObjectName("WiFiHistoryList");
    historyListWidget->setStyleSheet(
        "QListWidget {"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    background-color: white;"
        "    font-size: 11px;"  // 恢复合适字体
        "    selection-background-color: #e7ebf0;"
        "}"
        "QListWidget::item {"
        "    padding: 6px;"  // 恢复合适内边距
        "    border-bottom: 1px solid #f0f0f0;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #f8f9ff;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #e7ebf0;"
        "    color: #0070f9;"
        "}"
    );
    historyListWidget->setFixedHeight(200);
    
    // 添加示例历史记录
    QListWidgetItem *item1 = new QListWidgetItem();
    item1->setText("📶 Home_WiFi\n🕐 2024-08-04 10:30");
    item1->setData(Qt::UserRole, "Home_WiFi");
    historyListWidget->addItem(item1);
    
    QListWidgetItem *item2 = new QListWidgetItem();
    item2->setText("📶 Office_Network\n🕐 2024-08-03 14:15");
    item2->setData(Qt::UserRole, "Office_Network");
    historyListWidget->addItem(item2);
    
    QListWidgetItem *item3 = new QListWidgetItem();
    item3->setText("📶 Guest_WiFi\n🕐 2024-08-02 16:45");
    item3->setData(Qt::UserRole, "Guest_WiFi");
    historyListWidget->addItem(item3);
    
    // 清除历史按钮
    QPushButton *clearHistoryButton = new QPushButton("Clear History");
    clearHistoryButton->setObjectName("ClearHistoryButton");
    clearHistoryButton->setFixedHeight(26);  // 恢复合适高度
    clearHistoryButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f0f0f0;"
        "    border: 1px solid #ddd;"
        "    border-radius: 4px;"
        "    padding: 4px 8px;"  // 恢复合适内边距
        "    font-size: 11px;"  // 恢复合适字体
        "    color: #666;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "    color: #333;"
        "}"
    );
    
    // 连接清除历史按钮
    connect(clearHistoryButton, &QPushButton::clicked, this, [historyListWidget]() {
        historyListWidget->clear();
    });
    
    // 连接历史列表项点击事件
    connect(historyListWidget, &QListWidget::itemClicked, this, [wifiNameEdit](QListWidgetItem *item) {
        QString networkName = item->data(Qt::UserRole).toString();
        wifiNameEdit->setText(networkName);
        wifiNameEdit->setFocus();
    });
    
    historyLayout->addWidget(historyListWidget);
    historyLayout->addWidget(clearHistoryButton);
    
    // 添加到表单布局
    formLayout->addRow(new QLabel("Network Name:"), wifiNameEdit);
    formLayout->addRow(new QLabel("Password:"), wifiPasswordEdit);
    formLayout->addRow("", showPasswordCheckBox);
    formLayout->addRow("", buttonContainer);
    formLayout->addRow(new QLabel("Status:"), statusLabel);
    
    // 设置表单标签样式
    QList<QLabel*> formLabels = wifiGroupBox->findChildren<QLabel*>();
    for (QLabel *label : formLabels) {
        if (label->text().contains(":")) {
            label->setStyleSheet("QLabel { font-size: 12px; color: #333; font-weight: bold; }");  // 恢复合适字体
        }
    }
    
    // 在发送配置按钮的成功回调中修改：
    connect(sendConfigButton, &QPushButton::clicked, this, [this, wifiNameEdit, wifiPasswordEdit, statusLabel, deviceType, historyListWidget]() {
        QString wifiName = wifiNameEdit->text().trimmed();
        QString wifiPassword = wifiPasswordEdit->text();
        
        if (wifiName.isEmpty()) {
            statusLabel->setText("❌ Please enter a WiFi network name");
            statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #d32f2f; }");
            return;
        }
        
        if (wifiPassword.isEmpty()) {
            statusLabel->setText("❌ Please enter a WiFi password");
            statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #d32f2f; }");
            return;
        }
        
        // 显示发送中状态
        statusLabel->setText("📡 Sending WiFi configuration to device...");
        statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #ff9800; }");
        
        // 模拟发送过程
        QTimer::singleShot(2000, [this, statusLabel, wifiName, deviceType, historyListWidget]() {
            statusLabel->setText(QString("✅ Configuration sent successfully!\nDevice will connect to: %1").arg(wifiName));
            statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #388e3c; }");
            
            // 添加到历史列表 - 检查是否已存在
            bool exists = false;
            for (int i = 0; i < historyListWidget->count(); ++i) {
                QListWidgetItem *existingItem = historyListWidget->item(i);
                if (existingItem->data(Qt::UserRole).toString() == wifiName) {
                    exists = true;
                    // 更新时间戳
                    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
                    existingItem->setText(QString("📶 %1\n🕐 %2").arg(wifiName, currentTime));
                    // 移动到顶部
                    historyListWidget->takeItem(i);
                    historyListWidget->insertItem(0, existingItem);
                    break;
                }
            }
            
            // 如果不存在，添加新项目
            if (!exists) {
                QListWidgetItem *newItem = new QListWidgetItem();
                QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");
                newItem->setText(QString("📶 %1\n🕐 %2").arg(wifiName, currentTime));
                newItem->setData(Qt::UserRole, wifiName);
                historyListWidget->insertItem(0, newItem);
                
                // 限制历史记录数量（最多保留8条）
                while (historyListWidget->count() > 8) {
                    QListWidgetItem *lastItem = historyListWidget->takeItem(historyListWidget->count() - 1);
                    delete lastItem;
                }
            }
            
            // 添加设备到连接列表
            onWiFiConfigurationSuccess(deviceType, wifiName);
        });
    });
    
    // 清除按钮功能
    connect(clearButton, &QPushButton::clicked, this, [wifiNameEdit, wifiPasswordEdit, statusLabel, showPasswordCheckBox]() {
        wifiNameEdit->clear();
        wifiPasswordEdit->clear();
        showPasswordCheckBox->setChecked(false);
        statusLabel->setText("Ready to send WiFi configuration to device");
        statusLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
    });
    
    // 验证输入字段以启用/禁用发送按钮
    auto validateInputs = [sendConfigButton, wifiNameEdit, wifiPasswordEdit]() {
        bool isValid = !wifiNameEdit->text().trimmed().isEmpty() && 
                      !wifiPasswordEdit->text().isEmpty();
        sendConfigButton->setEnabled(isValid);
    };
    
    connect(wifiNameEdit, &QLineEdit::textChanged, validateInputs);
    connect(wifiPasswordEdit, &QLineEdit::textChanged, validateInputs);
    
    // 初始状态下禁用发送按钮
    sendConfigButton->setEnabled(false);
    
    // 添加设备连接状态提示
    QLabel *deviceStatusLabel = new QLabel("💻 Ensure your device is connected via USB");
    deviceStatusLabel->setStyleSheet("QLabel { font-size: 11px; color: #888; font-style: italic; }");  // 恢复合适字体
    deviceStatusLabel->setAlignment(Qt::AlignCenter);
    deviceStatusLabel->setFixedHeight(18);  // 恢复合适高度
    
    // 添加配置表单和历史列表到水平布局
    contentLayout->addWidget(wifiGroupBox, 3);      // 配置表单占 3 份空间
    contentLayout->addWidget(historyGroupBox, 2);   // 历史列表占 2 份空间
    
    // 添加到主布局 - 恢复合适间距
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(5);  // 恢复合适间距
    mainLayout->addWidget(descLabel);
    mainLayout->addSpacing(8);  // 恢复合适间距
    mainLayout->addLayout(contentLayout);
    mainLayout->addSpacing(5);  // 恢复合适间距
    mainLayout->addWidget(deviceStatusLabel);
    // 不添加 addStretch()，保持紧凑布局
    
    // 设置滚动区域
    scrollArea->setWidget(contentContainer);
    
    // 创建最终容器
    QVBoxLayout *finalLayout = new QVBoxLayout(wifiContent);
    finalLayout->setContentsMargins(0, 0, 0, 0);
    finalLayout->addWidget(scrollArea);
    
    return wifiContent;
}

void MainWindow::createDeviceStatusSection() {
    // USB连接状态区域
    usbStatusWidget = new QWidget();
    usbStatusWidget->setObjectName("USBStatusWidget");
    usbStatusWidget->setFixedHeight(75);
    usbStatusWidget->setStyleSheet(
        "QWidget#USBStatusWidget {"
        "    background-color: #f0f0f0;"  // 设置背景色与侧边栏一致
        "}"
    );
    
    QVBoxLayout *usbLayout = new QVBoxLayout(usbStatusWidget);
    usbLayout->setContentsMargins(15, 10, 15, 10);
    usbLayout->setSpacing(6);
    
    // 标题
    QLabel *usbTitle = new QLabel("USB Connection");
    usbTitle->setObjectName("USBSectionTitle");
    usbTitle->setStyleSheet(
        "QLabel#USBSectionTitle {"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    color: #555;"
        "    margin: 0;"
        "    background-color: #f0f0f0;"  // 确保标题背景色一致
        "}"
    );
    
    // 状态容器
    QWidget *statusContainer = new QWidget();
    statusContainer->setStyleSheet(
        "QWidget {"
        "    background-color: #f0f0f0;"  // 设置状态容器背景色
        "}"
    );
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
    usbStatusLabel->setStyleSheet(
        "QLabel#USBStatusLabel {"
        "    font-size: 12px;"
        "    color: #777;"
        "    margin: 0;"
        "    background-color: #f0f0f0;"  // 确保状态文本背景色一致
        "}"
    );
    
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
    deviceListWidget->setStyleSheet(
        "QWidget#DeviceListWidget {"
        "    background-color: #f0f0f0;"  // 设置背景色与侧边栏一致
        "}"
    );
    
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceListWidget);
    deviceLayout->setContentsMargins(15, 10, 15, 12);
    deviceLayout->setSpacing(8);
    
    // 标题容器
    QWidget *titleContainer = new QWidget();
    titleContainer->setStyleSheet(
        "QWidget {"
        "    background-color: #f0f0f0;"  // 设置标题容器背景色
        "}"
    );
    QHBoxLayout *titleLayout = new QHBoxLayout(titleContainer);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(6);
    
    QLabel *deviceTitle = new QLabel("Connected Devices");
    deviceTitle->setObjectName("DeviceSectionTitle");
    deviceTitle->setStyleSheet(
        "QLabel#DeviceSectionTitle {"
        "    font-size: 13px;"
        "    font-weight: bold;"
        "    color: #555;"
        "    margin: 0;"
        "    background-color: #f0f0f0;"  // 确保设备标题背景色一致
        "}"
    );
    
    // 设备数量标签
    QLabel *deviceCount = new QLabel("(0)");
    deviceCount->setObjectName("DeviceCount");
    deviceCount->setStyleSheet(
        "QLabel#DeviceCount {"
        "    font-size: 12px;"
        "    color: #888;"
        "    margin: 0;"
        "    background-color: #f0f0f0;"  // 确保数量标签背景色一致
        "}"
    );
    
    titleLayout->addWidget(deviceTitle);
    titleLayout->addWidget(deviceCount);
    titleLayout->addStretch();
    
    // 设备列表
    connectedDevicesList = new QListWidget();
    connectedDevicesList->setObjectName("ConnectedDevicesList");
    connectedDevicesList->setStyleSheet(
        "QListWidget#ConnectedDevicesList {"
        "    border: 1px solid #e0e0e0;"
        "    border-radius: 6px;"
        "    background-color: white;"  // 列表保持白色背景便于阅读
        "    font-size: 11px;"
        "    selection-background-color: #e7ebf0;"
        "    outline: none;"
        "}"
        "QListWidget#ConnectedDevicesList::item {"
        "    padding: 8px 10px;"
        "    border-bottom: 1px solid #f5f5f5;"
        "    min-height: 16px;"
        "    background-color: white;"  // 确保列表项背景为白色
        "}"
        "QListWidget#ConnectedDevicesList::item:hover {"
        "    background-color: #f8f9ff;"
        "}"
        "QListWidget#ConnectedDevicesList::item:selected {"
        "    background-color: #e7ebf0;"
        "    color: #0070f9;"
        "}"
        "QListWidget#ConnectedDevicesList::item:last {"
        "    border-bottom: none;"
        "}"
    );
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
    
    // 添加弹性空间推到底部
    sidebarLayout->addStretch();
}

void MainWindow::addConnectedDevice(const QString &deviceName, const QString &status, bool isConnected) {
    QListWidgetItem *item = new QListWidgetItem();
    
    // 创建自定义widget作为列表项
    QWidget *deviceWidget = new QWidget();
    deviceWidget->setObjectName("DeviceItem");
    deviceWidget->setStyleSheet(
        "QWidget#DeviceItem {"
        "    background-color: white;"  // 设备项保持白色背景
        "}"
    );
    
    QVBoxLayout *itemLayout = new QVBoxLayout(deviceWidget);
    itemLayout->setContentsMargins(4, 4, 4, 4);
    itemLayout->setSpacing(3);
    
    // 设备名称
    QLabel *nameLabel = new QLabel(deviceName);
    nameLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    color: #333;"
        "    margin: 0;"
        "    background-color: white;"  // 确保名称标签背景为白色
        "}"
    );
    
    // 状态容器
    QWidget *statusContainer = new QWidget();
    statusContainer->setStyleSheet(
        "QWidget {"
        "    background-color: white;"  // 状态容器背景为白色
        "}"
    );
    QHBoxLayout *statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(5);
    
    // 状态图标
    QLabel *statusIcon = new QLabel();
    statusIcon->setFixedSize(10, 10);
    statusIcon->setStyleSheet(QString(
        "QLabel {"
        "    background-color: %1;"
        "    border-radius: 5px;"
        "}"
    ).arg(isConnected ? "#4CAF50" : "#FF9800"));
    
    // 状态文本
    QLabel *statusLabel = new QLabel(status);
    statusLabel->setStyleSheet(QString(
        "QLabel {"
        "    font-size: 10px;"
        "    color: %1;"
        "    margin: 0;"
        "    background-color: white;"  // 确保状态文本背景为白色
        "}"
    ).arg(isConnected ? "#4CAF50" : "#FF9800"));
    
    statusLayout->addWidget(statusIcon);
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    itemLayout->addWidget(nameLabel);
    itemLayout->addWidget(statusContainer);
    
    // 设置item尺寸
    QSize itemSize = deviceWidget->sizeHint();
    itemSize.setHeight(qMax(itemSize.height(), 45));
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
    separator->setStyleSheet(
        "QFrame#SidebarSeparator {"
        "    color: #e0e0e0;"
        "    background-color: #e0e0e0;"
        "    margin: 12px 20px;"
        "}"
    );
    sidebarLayout->addWidget(separator);
}