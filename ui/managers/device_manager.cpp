#include "managers/device_manager.h"
#include "components/wifi_setup_widget.h"
#include "components/face_config_widget.h"
#include "serial.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>

DeviceManager::DeviceManager(std::shared_ptr<SerialPortManager> serialManager, QObject *parent)
    : QObject(parent)
    , m_serialManager(serialManager)
{
}

bool DeviceManager::hasDevice(const QString &deviceName) const
{
    return m_deviceTypes.contains(deviceName);
}

QString DeviceManager::getDeviceType(const QString &deviceName) const
{
    return m_deviceTypes.value(deviceName, "Unknown");
}

void DeviceManager::updateDeviceType(const QString &deviceName, const QString &deviceType)
{
    if (m_deviceTypes.contains(deviceName)) {
        m_deviceTypes[deviceName] = deviceType;
        
        // 如果设备类型改变了，需要重新创建配置页面
        if (m_devicePages.contains(deviceName)) {
            QWidget *oldPage = m_devicePages[deviceName];
            QWidget *newPage = createDeviceConfigPage(deviceName, deviceType);
            m_devicePages[deviceName] = newPage;
            
            if (oldPage) {
                oldPage->deleteLater();
            }
        }
    }
}

void DeviceManager::addDevice(const QString &deviceName, const QString &deviceType)
{
    if (hasDevice(deviceName)) {
        return; // 设备已存在
    }

    m_deviceTypes[deviceName] = deviceType;
    
    // 创建设备配置页面
    QWidget *configPage = createDeviceConfigPage(deviceName, deviceType);
    m_devicePages[deviceName] = configPage;

    emit deviceAdded(deviceName, deviceType);
}

void DeviceManager::removeDevice(const QString &deviceName)
{
    if (!m_deviceTypes.contains(deviceName)) {
        return;
    }

    // 清理资源
    if (m_devicePages.contains(deviceName)) {
        QWidget *page = m_devicePages[deviceName];
        m_devicePages.remove(deviceName);
        if (page) {
            page->deleteLater();
        }
    }

    m_deviceTypes.remove(deviceName);
    emit deviceRemoved(deviceName);
}

void DeviceManager::clearAllDevices()
{
    // 清理所有设备页面
    for (auto it = m_devicePages.begin(); it != m_devicePages.end(); ++it) {
        QWidget *page = it.value();
        if (page) {
            page->deleteLater();
        }
    }
    
    m_devicePages.clear();
    m_deviceTypes.clear();
}

QWidget* DeviceManager::getDeviceConfigPage(const QString &deviceName)
{
    return m_devicePages.value(deviceName, nullptr);
}

void DeviceManager::updateDeviceStatus(const QString &deviceName, const QString &ipAddress, int batteryLevel)
{
    if (!m_deviceTypes.contains(deviceName)) {
        return;
    }

    QString statusText = QString("Connected | IP: %1 | Battery: %2%")
                        .arg(ipAddress)
                        .arg(batteryLevel);

    // 更新对应设备页面的状态显示
    QWidget *configPage = m_devicePages.value(deviceName);
    if (configPage) {
        // 查找状态标签并更新
        QLabel *statusLabel = configPage->findChild<QLabel*>("deviceStatusLabel");
        if (statusLabel) {
            statusLabel->setText(statusText);
            
            // 根据电量设置颜色
            if (batteryLevel > 30) {
                statusLabel->setStyleSheet("color: green; font-weight: bold;");
            } else if (batteryLevel > 15) {
                statusLabel->setStyleSheet("color: orange; font-weight: bold;");
            } else {
                statusLabel->setStyleSheet("color: red; font-weight: bold;");
            }
        }
        
        // 如果是面部追踪器，更新IP地址显示
        if (m_deviceTypes[deviceName] == "Face Tracker") {
            FaceConfigWidget *faceWidget = qobject_cast<FaceConfigWidget*>(configPage);
            if (faceWidget) {
                faceWidget->updateDeviceIP(ipAddress);
            }
        }
    }

    emit deviceStatusUpdated(deviceName, statusText);
}

QWidget* DeviceManager::createDeviceConfigPage(const QString &deviceName, const QString &deviceType)
{
    if (deviceType == "Face Tracker") {
        return new FaceConfigWidget(deviceType);
    } else if (deviceType == "Eye Tracker") {
        return createEyeTrackerConfig(deviceName, deviceType);
    } else {
        return createGenericDeviceConfig(deviceName, deviceType);
    }
}

QWidget* DeviceManager::createEyeTrackerConfig(const QString &deviceName, const QString &deviceType)
{
    QWidget *eyeConfigWidget = new QWidget();
    eyeConfigWidget->setObjectName("EyeTrackerConfig");

    QVBoxLayout *layout = new QVBoxLayout(eyeConfigWidget);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(20);
    layout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel(QString("Eye Tracker Configuration - %1").arg(deviceName));
    titleLabel->setObjectName("eyeConfigTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    color: #333;"
        "    margin-bottom: 10px;"
        "}"
    );

    // 状态显示
    QLabel *statusLabel = new QLabel("Initializing...");
    statusLabel->setObjectName("deviceStatusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    color: #666;"
        "    margin-bottom: 20px;"
        "    padding: 8px;"
        "    background-color: #f8f9fa;"
        "    border-radius: 4px;"
        "}"
    );

    // 占位内容
    QLabel *placeholderLabel = new QLabel("Eye tracker configuration panel will be implemented here.\n\n"
                                          "Features to include:\n"
                                          "• Left/Right eye calibration\n"
                                          "• Tracking sensitivity adjustment\n"
                                          "• Eye openness monitoring\n"
                                          "• Battery status display");
    placeholderLabel->setAlignment(Qt::AlignCenter);
    placeholderLabel->setWordWrap(true);
    placeholderLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    color: #666;"
        "    line-height: 1.6;"
        "    padding: 20px;"
        "    background-color: #f8f9fa;"
        "    border: 1px solid #e9ecef;"
        "    border-radius: 6px;"
        "}"
    );

    layout->addWidget(titleLabel);
    layout->addWidget(statusLabel);
    layout->addWidget(placeholderLabel);
    layout->addStretch();

    return eyeConfigWidget;
}

QWidget* DeviceManager::createGenericDeviceConfig(const QString &deviceName, const QString &deviceType)
{
    QWidget *genericConfigWidget = new QWidget();
    genericConfigWidget->setObjectName("GenericDeviceConfig");

    QVBoxLayout *layout = new QVBoxLayout(genericConfigWidget);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(20);
    layout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel *titleLabel = new QLabel(QString("%1 Configuration - %2").arg(deviceType, deviceName));
    titleLabel->setObjectName("genericConfigTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    color: #333;"
        "    margin-bottom: 10px;"
        "}"
    );

    // 状态显示
    QLabel *statusLabel = new QLabel("Device connected");
    statusLabel->setObjectName("deviceStatusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    color: #28a745;"
        "    font-weight: bold;"
        "    margin-bottom: 20px;"
        "    padding: 8px;"
        "    background-color: #d4edda;"
        "    border: 1px solid #c3e6cb;"
        "    border-radius: 4px;"
        "}"
    );

    // 通用配置内容
    QLabel *infoLabel = new QLabel(QString("Device Type: %1\nDevice Name: %2\n\n"
                                          "This device is connected and ready to use.\n"
                                          "Configuration options will be available based on device capabilities.")
                                  .arg(deviceType, deviceName));
    infoLabel->setAlignment(Qt::AlignLeft);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    color: #495057;"
        "    line-height: 1.6;"
        "    padding: 20px;"
        "    background-color: #f8f9fa;"
        "    border: 1px solid #e9ecef;"
        "    border-radius: 6px;"
        "}"
    );

    layout->addWidget(titleLabel);
    layout->addWidget(statusLabel);
    layout->addWidget(infoLabel);
    layout->addStretch();

    return genericConfigWidget;
}
