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
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStyle>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , centralWidget(nullptr)
    , mainLayout(nullptr)
    , sidebarWidget(nullptr)
    , sidebarLayout(nullptr)
    , contentStack(nullptr)
    , titleBarWidget(nullptr)
    , titleLeftArea(nullptr)
    , titleRightArea(nullptr)
    , minimizeButton(nullptr)
    , barsButton(nullptr)
    , bellButton(nullptr)
    , closeButton(nullptr)
    , titleLabel(nullptr)
, dropdownMenu(nullptr)
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
    setFixedSize(960, 540);
    // 创建自定义标题栏
    setupTitleBar();
    // 创建阴影框架
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setColor(QColor(127, 127, 127, 127));  // 设置阴影颜色,透明度
    shadowEffect->setBlurRadius(20);                     // 设置阴影范围
    shadowEffect->setXOffset(0);                         // 设置阴影开始位置
    shadowEffect->setYOffset(0);

    auto frame = new QFrame(this);
    frame->setGeometry(10, 10, width() - 20, height() - 20);
    frame->setGraphicsEffect(shadowEffect);
    frame->setObjectName("centralWidget");

    // 创建主布局（不包含标题栏）
    mainLayout = new QHBoxLayout(frame);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->setMenuBar(titleBarWidget); // 将标题栏添加到布局中

    // 创建侧边栏
    sidebarWidget = new QWidget();
    sidebarWidget->setObjectName("sidebarWidget");
    sidebarWidget->setFixedWidth(200);  // 设置固定宽度
    // 移除直接样式设置，使用QSS

    sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setAlignment(Qt::AlignTop);
    sidebarLayout->setSpacing(0);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);

    // 创建主内容区域
    contentStack = new QStackedWidget();
    contentStack->setObjectName("contentStack");
    // 移除直接样式设置，使用QSS

    // 添加到主布局
    mainLayout->addWidget(sidebarWidget);
    mainLayout->addWidget(contentStack);

    // 创建侧边栏项示例
    createSidebarItem(":/resources/resources/images/vr-cardboard-solid-full.png", "Face Tracker");
    createSidebarItem(":/resources/resources/images/face-smile-regular-full.png", "Eye Tracker");
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
}

void MainWindow::createSidebarItem(const QString &iconPath, const QString &text) {
    // 创建侧边栏项容器
    QWidget *itemWidget = new QWidget();
    itemWidget->setObjectName("SiderBarItem");
    itemWidget->setProperty("selected", false); // 添加选中状态属性
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
    itemLayout->addStretch();  // 推到左侧

    // 添加到侧边栏布局
    sidebarLayout->addWidget(itemWidget);

    // 保存引用以便后续操作
    sidebarItems.append(itemWidget);

    // 连接点击事件
    connect(itemWidget, &QWidget::customContextMenuRequested, this, [=]() {
        setSelectedItem(itemWidget);
    });

    // 安装事件过滤器处理hover效果
    itemWidget->installEventFilter(this);
    iconLabel->installEventFilter(this);
    textLabel->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // 查找包含在sidebarItems中的对象
    QWidget *itemWidget = nullptr;
    for (QWidget *widget : sidebarItems) {
        if (widget == obj || widget->isAncestorOf(qobject_cast<QWidget*>(obj))) {
            itemWidget = widget;
            break;
        }
    }

    if (itemWidget) {
        if (event->type() == QEvent::Enter) {
            // 鼠标进入时应用hover样式（除非已选中）
            if (!itemWidget->property("selected").toBool()) {
                itemWidget->setStyleSheet(
                    "QWidget#SiderBarItem {"
                    "    background-color: #e7ebf0;"
                    "}"
                    "QLabel#sidebarIconLabel {"
                    "    background-color: #e7ebf0;"
                    "}"
                    "QLabel#sidebarTextLabel {"
                    "    background-color: #e7ebf0;"
                    "    color: #333333;"
                    "}"
                );
            }
            return true;
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开时恢复默认样式（除非已选中）
            if (!itemWidget->property("selected").toBool()) {
                itemWidget->setStyleSheet("");
            }
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            // 点击时设置为选中状态
            setSelectedItem(itemWidget);
            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
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
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
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

// DropdownMenu 实现
DropdownMenu::DropdownMenu(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置菜单样式
    setObjectName("DropdownMenu");
    setStyleSheet(
        "QWidget#DropdownMenu {"
        "    background-color: white;"
        "    border-radius: 8px;"
        "}"
    );

    menuLayout = new QVBoxLayout(this);
    menuLayout->setContentsMargins(0, 10, 0, 10);
    menuLayout->setSpacing(0);
}

void DropdownMenu::addItem(QWidget *item)
{
    menuLayout->addWidget(item);
}

void DropdownMenu::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255));

    // 绘制圆角矩形背景
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    painter.drawPath(path);

    QWidget::paintEvent(event);
}

// 在 MainWindow::setupUI() 方法中添加以下代码来创建菜单
void MainWindow::setupDropdownMenu()
{
    dropdownMenu = new DropdownMenu(this);

    // 添加示例菜单项
    addMenuItem(":/resources/resources/images/bars-solid-full.png", "Settings", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/bars-solid-full.png", "Profile", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/bars-solid-full.png", "Help", ":/resources/resources/images/chevron-down-solid-full.png");
}

// 添加菜单项的方法
void MainWindow::addMenuItem(const QString &iconPath, const QString &text, const QString &arrowPath)
{
    // 创建菜单项容器
    QWidget *menuItem = new QWidget();
    menuItem->setObjectName("MenuItem");
    menuItem->setStyleSheet(
        "QWidget#MenuItem {"
        "    background-color: transparent;"
        "    padding: 10px 20px;"
        "}"
        "QWidget#MenuItem:hover {"
        "    background-color: #f0f0f0;"
        "}"
    );

    QHBoxLayout *itemLayout = new QHBoxLayout(menuItem);
    itemLayout->setContentsMargins(20, 0, 20, 0);
    itemLayout->setSpacing(10);

    // 左侧功能图标
    QLabel *iconLabel = new QLabel();
    iconLabel->setObjectName("MenuItemIcon");
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        iconLabel->setFixedSize(20, 20);
    }

    // 中间文字内容
    QLabel *textLabel = new QLabel(text);
    textLabel->setObjectName("MenuItemText");
    textLabel->setStyleSheet("QLabel#MenuItemText { color: #333333; font-size: 14px; }");

    // 右侧跳转图标
    QLabel *arrowLabel = new QLabel();
    arrowLabel->setObjectName("MenuItemArrow");
    QPixmap arrowPixmap(arrowPath);
    if (!arrowPixmap.isNull()) {
        arrowLabel->setPixmap(arrowPixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        arrowLabel->setFixedSize(16, 16);
    }

    // 添加到布局
    itemLayout->addWidget(iconLabel);
    itemLayout->addWidget(textLabel);
    itemLayout->addStretch();
    itemLayout->addWidget(arrowLabel);

    // 添加到菜单布局
    dropdownMenu->addItem(menuItem);
}

// 修改 onBarsButtonClicked 方法
void MainWindow::onBarsButtonClicked() {
    // 创建下拉菜单（如果尚未创建）
    if (!dropdownMenu) {
        setupDropdownMenu();
    }

    // 计算菜单显示位置
    QPoint globalPos = barsButton->mapToGlobal(QPoint(0, barsButton->height()));

    // 设置菜单大小
    dropdownMenu->setFixedSize(250, 150);

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(dropdownMenu);
    shadowEffect->setColor(QColor(0, 0, 0, 60));
    shadowEffect->setBlurRadius(20);
    shadowEffect->setOffset(0, 5);
    dropdownMenu->setGraphicsEffect(shadowEffect);

    // 显示菜单
    dropdownMenu->move(globalPos);
    dropdownMenu->show();
}