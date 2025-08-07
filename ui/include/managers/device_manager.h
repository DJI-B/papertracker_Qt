#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <QObject>
#include <QWidget>
#include <QMap>
#include <QString>
#include <memory>

class SerialPortManager;
class WiFiSetupWidget;
class FaceConfigWidget;

/**
 * @brief 设备管理器
 * 负责管理设备的连接、断连、配置页面创建等
 */
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(std::shared_ptr<SerialPortManager> serialManager, QObject *parent = nullptr);

    // 设备管理
    void addDevice(const QString &deviceName, const QString &deviceType);
    void removeDevice(const QString &deviceName);
    void clearAllDevices();

    // 添加设备存在性检查方法
    bool hasDevice(const QString &deviceName) const;
    
    // 获取和更新设备类型
    QString getDeviceType(const QString &deviceName) const;
    void updateDeviceType(const QString &deviceName, const QString &deviceType);

    // 获取设备配置页面
    QWidget* getDeviceConfigPage(const QString &deviceName);
    
    // 设备状态更新
    void updateDeviceStatus(const QString &deviceName, const QString &ipAddress, int batteryLevel);

signals:
    void deviceAdded(const QString &deviceName, const QString &deviceType);
    void deviceRemoved(const QString &deviceName);
    void deviceStatusUpdated(const QString &deviceName, const QString &status);

private:
    QWidget* createDeviceConfigPage(const QString &deviceName, const QString &deviceType);
    QWidget* createEyeTrackerConfig(const QString &deviceName, const QString &deviceType);
    QWidget* createGenericDeviceConfig(const QString &deviceName, const QString &deviceType);

    std::shared_ptr<SerialPortManager> m_serialManager;
    QMap<QString, QString> m_deviceTypes; // 设备名称 -> 设备类型
    QMap<QString, QWidget*> m_devicePages; // 设备名称 -> 配置页面
};

#endif // DEVICE_MANAGER_H
