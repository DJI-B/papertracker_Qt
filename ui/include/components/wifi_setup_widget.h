#ifndef WIFI_SETUP_WIDGET_H
#define WIFI_SETUP_WIDGET_H

#include "components/widget_component_base.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMessageBox>
#include <QSettings>
#include <memory>

class SerialPortManager;
class QListWidgetItem;

/**
 * @brief WiFi设置组件
 * 负责WiFi网络配置、历史记录管理等
 */
class WiFiSetupWidget : public WidgetComponentBase
{
    Q_OBJECT

public:
    explicit WiFiSetupWidget(const QString &deviceType, std::shared_ptr<SerialPortManager> serialManager, QWidget *parent = nullptr);
    ~WiFiSetupWidget() override;

    // 基类虚函数实现
    void setupUI() override;
    void retranslateUI() override;

    // 设置WiFi信息（用于外部调用）
    void setWiFiInfo(const QString &ssid, const QString &password = QString());

signals:
    void configurationSuccess(const QString &deviceType, const QString &wifiName);
    void configurationFailed(const QString &error);

public slots:
    void onSendConfigClicked();
    void onClearClicked();
    void onShowPasswordToggled(bool show);
    void onHistoryItemClicked(QListWidgetItem *item);
    void onClearHistoryClicked();

private:
    void setupLeftPanel();   // 设置左侧WiFi配置面板
    void setupRightPanel();  // 设置右侧历史记录面板
    void loadWiFiHistory();  // 加载WiFi历史记录
    void saveWiFiHistory();  // 保存WiFi历史记录
    void addToHistory(const QString &ssid, const QString &password);
    void updateConnectionStatus(const QString &status);
    void validateInputs();   // 验证输入
    
    // UI组件 - 左侧配置面板
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_contentWidget = nullptr;
    QHBoxLayout *m_mainLayout = nullptr;
    QVBoxLayout *m_leftLayout = nullptr;
    QVBoxLayout *m_rightLayout = nullptr;
    
    // 标题和描述
    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    
    // WiFi配置组
    QGroupBox *m_wifiGroupBox = nullptr;
    QFormLayout *m_formLayout = nullptr;
    QLineEdit *m_wifiNameEdit = nullptr;
    QLineEdit *m_wifiPasswordEdit = nullptr;
    QCheckBox *m_showPasswordCheckBox = nullptr;
    QLabel *m_statusLabel = nullptr;
    
    // 按钮
    QHBoxLayout *m_buttonLayout = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_sendConfigButton = nullptr;
    
    // 右侧历史记录面板
    QGroupBox *m_historyGroupBox = nullptr;
    QVBoxLayout *m_historyLayout = nullptr;
    QListWidget *m_historyList = nullptr;
    QPushButton *m_clearHistoryButton = nullptr;
    QLabel *m_historyCountLabel = nullptr;
    
    // 成员变量
    QString m_deviceType;
    std::shared_ptr<SerialPortManager> m_serialManager;
    QJsonArray m_wifiHistory;
    
    // 常量
    static const int MAX_HISTORY_COUNT = 20;
    static const QString SETTINGS_GROUP;
    static const QString WIFI_HISTORY_KEY;
};

#endif // WIFI_SETUP_WIDGET_H
