#ifndef MAIN_WINDOW_REFACTORED_H
#define MAIN_WINDOW_REFACTORED_H

#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QPoint>
#include <QMap>
#include <memory>

// 前向声明
class SerialPortManager;
class TitleBarWidget;
class SidebarWidget;
class WiFiSetupWidget;
class GuideWidget;
class DeviceManager;

/**
 * @brief 主窗口类（重构版）
 * 只负责窗口管理、布局协调和事件分发
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    // 标题栏事件处理
    void onMinimizeRequested();
    void onCloseRequested();
    void onMenuRequested();
    void onNotificationRequested();

    // 侧边栏事件处理
    void onSidebarItemClicked(const QString &itemText);
    void onDeviceTabClicked(const QString &deviceName);

    // 设备管理事件处理
    void onDeviceConnected(const QString &deviceName, const QString &deviceType);
    void onDeviceDisconnected(const QString &deviceName);

    // WiFi配置事件处理
    void onWiFiConfigurationSuccess(const QString &deviceType, const QString &wifiName);
    void onWifiConfigRequest(const QString &wifiName, const QString &wifiPassword);

private:
    void setupUI();
    void setupSerialManager();
    void createContentPages();
    void createDefaultContent();
    void loadStyleSheet();
    
    // WiFi配置相关
    void showWifiConfigPrompt(const QString &wifiName, const QString &wifiPassword);
    void showWifiConfigPage(const QString &wifiName, const QString &wifiPassword);

    // UI 组件
    QWidget *m_centralFrame = nullptr;
    QHBoxLayout *m_mainLayout = nullptr;
    TitleBarWidget *m_titleBar = nullptr;
    SidebarWidget *m_sidebar = nullptr;
    QStackedWidget *m_contentStack = nullptr;

    // 内容页面
    QWidget *m_defaultContentWidget = nullptr;
    WiFiSetupWidget *m_wifiConfigWidget = nullptr;
    GuideWidget *m_faceGuideWidget = nullptr;
    GuideWidget *m_eyeGuideWidget = nullptr;

    // 管理器
    std::shared_ptr<SerialPortManager> m_serialManager;
    DeviceManager *m_deviceManager = nullptr;

    // 窗口拖动
    QPoint m_dragPosition;
};

#endif // MAIN_WINDOW_REFACTORED_H
