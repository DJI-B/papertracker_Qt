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

    void setupUI() override;
    void retranslateUI() override;
    
    void updateWifiInfo(const QString &wifiName, const QString &wifiPassword);

signals:
    void configurationSuccess(const QString &deviceType, const QString &wifiName);

private slots:
    void onSendConfigClicked();
    void onClearClicked();
    void onShowPasswordToggled(bool checked);
    void onHistoryItemClicked(QListWidgetItem *item);
    void onClearHistoryClicked();

private:
    void validateInputs();
    void addToHistory(const QString &wifiName);
    bool isWifiNameInHistory(const QString &wifiName);

    QString m_deviceType;
    std::shared_ptr<SerialPortManager> m_serialManager;
    
    // UI组件
    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    QLabel *m_deviceStatusLabel = nullptr;
    QGroupBox *m_wifiGroupBox = nullptr;
    QGroupBox *m_historyGroupBox = nullptr;
    QLineEdit *m_wifiNameEdit = nullptr;
    QLineEdit *m_wifiPasswordEdit = nullptr;
    QCheckBox *m_showPasswordCheckBox = nullptr;
    QLabel *m_wifiStatusLabel = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_sendConfigButton = nullptr;
    QListWidget *m_historyListWidget = nullptr;
    QPushButton *m_clearHistoryButton = nullptr;
    QLabel *m_networkNameLabel = nullptr;
    QLabel *m_passwordLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // WIFI_SETUP_WIDGET_H
