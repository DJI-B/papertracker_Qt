#include "components/sidebar_widget.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPixmap>
#include <QIcon>
#include <QEvent>
#include <QMouseEvent>
#include <QFrame>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void SidebarWidget::setupUI()
{
    setObjectName("sidebarWidget");
    setFixedWidth(200);

    sidebarLayout = new QVBoxLayout(this);
    sidebarLayout->setAlignment(Qt::AlignTop);
    sidebarLayout->setSpacing(0);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);

    // 创建"添加设备"选项
    createSidebarItem(":/resources/resources/images/vr-cardboard-solid-full.png", "Add Device");

    // 添加分隔线
    createSidebarSeparator();
    sidebarLayout->addStretch();
}

void SidebarWidget::createSidebarItem(const QString &iconPath, const QString &text)
{
    // 创建侧边栏项容器
    QWidget *item = new QWidget();
    item->setObjectName("SiderBarItem");
    item->setFixedHeight(48);
    item->setCursor(Qt::PointingHandCursor);
    item->setProperty("itemText", text);

    // 创建水平布局
    QHBoxLayout *layout = new QHBoxLayout(item);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    // 图标
    QLabel *iconLabel = new QLabel();
    iconLabel->setObjectName("sidebarIconLabel");
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // 文字
    QLabel *textLabel = new QLabel(text);
    textLabel->setObjectName("sidebarTextLabel");

    // 添加到布局
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();

    // 设置样式
    item->setStyleSheet(
        "QWidget#SiderBarItem {"
        "    background-color: transparent;"
        "    border-radius: 4px;"
        "}"
        "QLabel#sidebarTextLabel {"
        "    color: #333;"
        "    font-size: 14px;"
        "    font-weight: 400;"
        "}"
    );

    // 安装事件过滤器
    item->installEventFilter(this);
    iconLabel->installEventFilter(this);
    textLabel->installEventFilter(this);

    // 添加到布局和列表
    sidebarLayout->addWidget(item);
    sidebarItems.append(item);
}

void SidebarWidget::createSidebarSeparator()
{
    QFrame *separator = new QFrame();
    separator->setObjectName("sidebarSeparator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setFixedHeight(1);
    separator->setStyleSheet(
        "QFrame#sidebarSeparator {"
        "    color: #e0e0e0;"
        "    margin: 8px 16px;"
        "}"
    );
    sidebarLayout->addWidget(separator);
}

void SidebarWidget::addDeviceTab(const QString &deviceName, const QString &deviceType)
{
    // 检查是否已存在
    if (deviceTabs.contains(deviceName)) {
        return;
    }

    // 获取设备图标
    QString iconPath = getDeviceIcon(deviceType);

    // 创建设备tab
    QWidget *deviceTab = new QWidget();
    deviceTab->setObjectName("DeviceTab");
    deviceTab->setFixedHeight(48);
    deviceTab->setCursor(Qt::PointingHandCursor);
    deviceTab->setProperty("deviceName", deviceName);
    deviceTab->setProperty("deviceType", deviceType);

    // 创建布局
    QHBoxLayout *layout = new QHBoxLayout(deviceTab);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    // 设备图标
    QLabel *iconLabel = new QLabel();
    iconLabel->setObjectName("deviceIconLabel");
    QPixmap iconPixmap(iconPath);
    if (!iconPixmap.isNull()) {
        iconLabel->setPixmap(iconPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // 设备名称
    QLabel *nameLabel = new QLabel(deviceName);
    nameLabel->setObjectName("deviceNameLabel");

    // 状态指示器
    QLabel *statusLabel = new QLabel("●");
    statusLabel->setObjectName("deviceStatusLabel");
    statusLabel->setStyleSheet("QLabel { color: #28a745; font-size: 12px; }"); // 绿色表示在线

    // 添加到布局
    layout->addWidget(iconLabel);
    layout->addWidget(nameLabel);
    layout->addStretch();
    layout->addWidget(statusLabel);

    // 设置样式
    deviceTab->setStyleSheet(
        "QWidget#DeviceTab {"
        "    background-color: transparent;"
        "    border-radius: 4px;"
        "}"
        "QLabel#deviceNameLabel {"
        "    color: #333;"
        "    font-size: 12px;"
        "    font-weight: 400;"
        "}"
    );

    // 安装事件过滤器
    deviceTab->installEventFilter(this);
    iconLabel->installEventFilter(this);
    nameLabel->installEventFilter(this);
    statusLabel->installEventFilter(this);

    // 插入到分隔线前面
    int separatorIndex = sidebarLayout->count() - 2; // 分隔线和stretch的位置
    sidebarLayout->insertWidget(separatorIndex, deviceTab);

    // 存储映射
    deviceTabs[deviceName] = deviceTab;
    sidebarItems.append(deviceTab);
}

void SidebarWidget::removeDeviceTab(const QString &deviceName)
{
    if (!deviceTabs.contains(deviceName)) {
        return;
    }

    QWidget *deviceTab = deviceTabs[deviceName];
    
    // 从布局和列表中移除
    sidebarLayout->removeWidget(deviceTab);
    sidebarItems.removeAll(deviceTab);
    deviceTabs.remove(deviceName);
    
    // 删除widget
    deviceTab->deleteLater();
}

void SidebarWidget::clearAllDeviceTabs()
{
    // 移除所有设备标签页
    for (auto it = deviceTabs.begin(); it != deviceTabs.end(); ++it) {
        QWidget *deviceTab = it.value();
        sidebarLayout->removeWidget(deviceTab);
        sidebarItems.removeAll(deviceTab);
        deviceTab->deleteLater();
    }
    deviceTabs.clear();
}

void SidebarWidget::setSelectedItem(QWidget *selectedItem)
{
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
        "QWidget#SiderBarItem, QWidget#DeviceTab {"
        "    background-color: #e7ebf0;"
        "}"
        "QLabel#sidebarIconLabel, QLabel#deviceIconLabel {"
        "    background-color: #e7ebf0;"
        "}"
        "QLabel#sidebarTextLabel, QLabel#deviceNameLabel {"
        "    background-color: #e7ebf0;"
        "    color: #0070f9;"
        "}"
    );
}

QString SidebarWidget::getDeviceIcon(const QString &deviceType) const
{
    if (deviceType == "Face Tracker") {
        return ":/resources/resources/images/face-smile-regular-full.png";
    } else if (deviceType == "Eye Tracker") {
        return ":/resources/resources/images/eye-regular-full.png";
    } else {
        return ":/resources/resources/images/vr-cardboard-solid-full.png";
    }
}

bool SidebarWidget::eventFilter(QObject *obj, QEvent *event)
{
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
                
                // 判断是设备标签页还是普通侧边栏项
                if (itemWidget->objectName() == "DeviceTab") {
                    QString deviceName = itemWidget->property("deviceName").toString();
                    emit deviceTabClicked(deviceName);
                } else {
                    QString itemText = itemWidget->property("itemText").toString();
                    emit itemClicked(itemText);
                }
            }
        } else if (event->type() == QEvent::Enter) {
            if (!itemWidget->property("selected").toBool()) {
                itemWidget->setStyleSheet(
                    "QWidget#SiderBarItem, QWidget#DeviceTab {"
                    "    background-color: #f5f5f5;"
                    "}"
                );
            }
        } else if (event->type() == QEvent::Leave) {
            if (!itemWidget->property("selected").toBool()) {
                itemWidget->setStyleSheet("");
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}
