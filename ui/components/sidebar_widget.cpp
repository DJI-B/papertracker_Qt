#include "components/sidebar_widget.h"
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPixmap>
#include <QIcon>
#include <QEvent>
#include <QMouseEvent>
#include <QFrame>
#include <QSizePolicy>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void SidebarWidget::setupUI()
{
    setFixedWidth(200);
    
    // 为 SidebarWidget 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 创建主容器 widget
    QWidget *mainWidget = new QWidget();
    mainWidget->setObjectName("SidebarWidget");

    // 为主容器创建布局
    sidebarLayout = new QVBoxLayout(mainWidget);
    sidebarLayout->setAlignment(Qt::AlignTop);
    sidebarLayout->setSpacing(0);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    
    // 将主容器添加到 SidebarWidget 的布局中
    mainLayout->addWidget(mainWidget);

    // 创建"添加设备"选项   
    createSidebarItem(":/resources/resources/images/vr-cardboard-solid-full.png", "Add Device");
    
    // 创建分隔线
    createSidebarSeparator();
    
    // 添加弹性空间，确保内容在顶部对齐
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

    // 样式由 light.qss 控制，不需要硬编码

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
    // 样式由 light.qss 控制
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
    // 初始状态样式，具体样式由 light.qss 控制
    statusLabel->setStyleSheet("QLabel { color: #28a745; font-size: 12px; }");

    // 添加到布局
    layout->addWidget(iconLabel);
    layout->addWidget(nameLabel);
    layout->addStretch();
    layout->addWidget(statusLabel);

    // 安装事件过滤器
    deviceTab->installEventFilter(this);
    iconLabel->installEventFilter(this);
    nameLabel->installEventFilter(this);
    statusLabel->installEventFilter(this);

    // 找到分隔线的位置，在分隔线后插入设备标签页
    int insertIndex = -1;
    for (int i = 0; i < sidebarLayout->count(); ++i) {
        QLayoutItem *item = sidebarLayout->itemAt(i);
        if (item && item->widget()) {
            QWidget *widget = item->widget();
            if (widget->objectName() == "sidebarSeparator") {
                insertIndex = i + 1;  // 在分隔线后插入
                break;
            }
        }
    }

    // 如果找到分隔线，在其后插入；否则在末尾插入（但要在stretch之前）
    if (insertIndex >= 0) {
        sidebarLayout->insertWidget(insertIndex, deviceTab);
    } else {
        // 在stretch之前插入
        int stretchIndex = sidebarLayout->count() - 1;
        if (stretchIndex >= 0) {
            sidebarLayout->insertWidget(stretchIndex, deviceTab);
        } else {
            sidebarLayout->addWidget(deviceTab);
        }
    }

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

void SidebarWidget::updateDeviceType(const QString &deviceName, const QString &deviceType)
{
    if (!deviceTabs.contains(deviceName)) {
        return;
    }

    QWidget *deviceTab = deviceTabs[deviceName];

    // 更新设备类型属性
    deviceTab->setProperty("deviceType", deviceType);

    // 查找设备标签中的各个组件
    QList<QLabel*> labels = deviceTab->findChildren<QLabel*>();
    for (QLabel *label : labels) {
        // 更新设备图标
        if (label->objectName() == "deviceIconLabel") {
            QString iconPath = getDeviceIcon(deviceType);
            if (!iconPath.isEmpty()) {
                QPixmap iconPixmap(iconPath);
                if (!iconPixmap.isNull()) {
                    label->setPixmap(iconPixmap.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                }
            }
        }

        // 更新设备名称显示（如果设备类型改变，可以调整显示名称）
        else if (label->objectName() == "deviceNameLabel") {
            // 保持原有设备名称，但可以根据需要进行调整
            // 例如：如果从 "WiFi配置中" 变为具体设备类型，可以更新显示文本
            if (deviceType == "Face Tracker") {
                // 可以选择保持原名称或者更新为更友好的显示名称
                // label->setText(deviceName); // 保持原名称
            } else if (deviceType == "Eye Tracker") {
                // 同样保持原名称或进行相应调整
                // label->setText(deviceName); // 保持原名称
            }
            // 目前保持原有名称不变
        }

        // 更新连接状态指示器
        else if (label->objectName() == "deviceStatusLabel") {
            if (deviceType == "WiFi配置中") {
                // WiFi配置中 - 橙色
                label->setStyleSheet("QLabel { color: #ffc107; font-size: 12px; }");
                label->setToolTip("设备正在配置WiFi网络");
            } else if (deviceType == "Face Tracker" || deviceType == "Eye Tracker") {
                // 已连接 - 绿色
                label->setStyleSheet("QLabel { color: #28a745; font-size: 12px; }");
                label->setToolTip("设备已连接");
            } else {
                // 未知状态 - 灰色
                label->setStyleSheet("QLabel { color: #6c757d; font-size: 12px; }");
                label->setToolTip("设备状态未知");
            }
        }
    }
}

void SidebarWidget::setSelectedItem(QWidget *selectedItem)
{
    // 取消之前选中项的选中状态
    for (QWidget *item : sidebarItems) {
        if (item->property("selected").toBool()) {
            item->setProperty("selected", false);
            // 清除内联样式，让QSS生效
            item->setStyleSheet("");
            // 强制样式更新
        }
    }

    // 设置新的选中项
    if (selectedItem) {
        selectedItem->setProperty("selected", true);
        // 清除内联样式，让QSS生效
        selectedItem->setStyleSheet("");
        // 强制样式更新
    }
}

QString SidebarWidget::getDeviceIcon(const QString &deviceType) const
{
    if (deviceType == "Face Tracker") {
        return ":/resources/resources/images/face-smile-regular-full.png";
    } else if (deviceType == "Eye Tracker") {
        return ":/resources/resources/images/eye-regular-full.png";
    } else if (deviceType == "WiFi配置中") {
        return ":/resources/resources/images/vr-cardboard-solid-full.png"; // WiFi配置中使用默认图标
    } else {
        return ":/resources/resources/images/vr-cardboard-solid-full.png"; // 默认图标
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
            // 悬停效果由 QSS 的 :hover 伪状态控制
            // 不需要手动设置样式
        } else if (event->type() == QEvent::Leave) {
            // 离开效果由 QSS 自动处理
            // 不需要手动设置样式
        }
    }

    return QWidget::eventFilter(obj, event);
}
