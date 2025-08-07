#include "components/title_bar_widget.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QStyle>
#include <QIcon>
#include <QPixmap>

// TitleBarWidget 实现
TitleBarWidget::TitleBarWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("titleBarWidget");
    setFixedHeight(40);
    setupTitleBar();
}

TitleBarWidget::~TitleBarWidget()
{
    destroyCustomMenu();
}

void TitleBarWidget::setTitle(const QString &title)
{
    if (titleLabel) {
        titleLabel->setText(title);
    }
}

void TitleBarWidget::setupTitleBar()
{
    // 创建标题栏布局
    titleBarLayout = new QHBoxLayout(this);
    titleBarLayout->setContentsMargins(0, 0, 0, 0);
    titleBarLayout->setSpacing(0);

    // 创建标题栏左侧区域（与侧边栏同色）
    titleLeftArea = new QWidget();
    titleLeftArea->setObjectName("titleLeftArea");
    titleLeftArea->setFixedWidth(200);
    titleLeftArea->setCursor(Qt::ArrowCursor);

    // 创建标题文本
    titleLabel = new QLabel("PaperTracker");
    titleLabel->setObjectName("titleLabel");

    // 左侧区域布局
    QHBoxLayout *leftLayout = new QHBoxLayout(titleLeftArea);
    leftLayout->setContentsMargins(20, 0, 0, 0);
    leftLayout->addWidget(titleLabel);
    leftLayout->addStretch();

    // 创建标题栏右侧区域
    titleRightArea = new QWidget();
    titleRightArea->setObjectName("titleRightArea");

    // 右侧区域布局
    titleRightLayout = new QHBoxLayout(titleRightArea);
    titleRightLayout->setContentsMargins(0, 0, 0, 0);
    titleRightLayout->setSpacing(0);

    setupButtons();

    // 添加到标题栏布局
    titleBarLayout->addWidget(titleLeftArea);
    titleBarLayout->addWidget(titleRightArea);
}

void TitleBarWidget::setupButtons()
{
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
    connect(minimizeButton, &QPushButton::clicked, this, &TitleBarWidget::onMinimizeButtonClicked);
    connect(barsButton, &QPushButton::clicked, this, &TitleBarWidget::onBarsButtonClicked);
    connect(bellButton, &QPushButton::clicked, this, &TitleBarWidget::onBellButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &TitleBarWidget::onCloseButtonClicked);

    // 添加按钮到右侧布局
    titleRightLayout->addStretch();
    titleRightLayout->addWidget(barsButton);
    titleRightLayout->addWidget(bellButton);
    titleRightLayout->addSpacing(4);
    titleRightLayout->addWidget(minimizeButton);
    titleRightLayout->addWidget(closeButton);
}

void TitleBarWidget::onMinimizeButtonClicked()
{
    emit minimizeRequested();
}

void TitleBarWidget::onCloseButtonClicked()
{
    emit closeRequested();
}

void TitleBarWidget::onBarsButtonClicked()
{
    // 每次都重新创建菜单
    destroyCustomMenu();
    createCustomMenu();

    // 计算菜单显示位置
    QPoint buttonGlobalPos = barsButton->mapToGlobal(QPoint(0, 0));
    QSize menuSize(200, 220);
    customMenu->setFixedSize(menuSize);

    QPoint menuPos = QPoint(
        buttonGlobalPos.x() - menuSize.width() + 8,
        buttonGlobalPos.y() + barsButton->height() + 2
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

    emit menuRequested();
}

void TitleBarWidget::onBellButtonClicked()
{
    emit notificationRequested();
}

void TitleBarWidget::createCustomMenu()
{
    customMenu = new CustomMenu(this);

    // 连接菜单隐藏信号到销毁方法
    connect(customMenu, &CustomMenu::menuHidden, this, &TitleBarWidget::destroyCustomMenu);

    setupMenuItems();
}

void TitleBarWidget::setupMenuItems()
{
    if (!customMenu) return;

    // 添加菜单项
    addMenuItem(":/resources/resources/images/bars-solid-full.png", "Settings", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/bell-regular-full.png", "Notifications", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/face-smile-regular-full.png", "Profile", ":/resources/resources/images/chevron-down-solid-full.png");
    addMenuItem(":/resources/resources/images/vr-cardboard-solid-full.png", "Help", ":/resources/resources/images/chevron-down-solid-full.png");
}

void TitleBarWidget::addMenuItem(const QString &iconPath, const QString &text, const QString &rightIconPath)
{
    if (!customMenu) return;

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
        iconLabel->setPixmap(iconPixmap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
            rightIconLabel->setPixmap(rightIconPixmap.scaled(12, 12, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

    // 添加到菜单
    customMenu->addItem(menuItem);
}

void TitleBarWidget::resetMenuItemsStyle()
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

void TitleBarWidget::destroyCustomMenu()
{
    if (customMenu) {
        customMenu->deleteLater();
        customMenu = nullptr;
    }
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
    if (menuLayout) {
        menuLayout->addWidget(item);
    }
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
