#include "components/wifi_setup_widget.h"
#include "serial.hpp"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QListWidgetItem>
#include <QSplitter>
#include <QDateTime>

// 静态常量定义
const QString WiFiSetupWidget::SETTINGS_GROUP = "WiFiHistory";
const QString WiFiSetupWidget::WIFI_HISTORY_KEY = "history";

WiFiSetupWidget::WiFiSetupWidget(const QString &deviceType, std::shared_ptr<SerialPortManager> serialManager, QWidget *parent)
    : WidgetComponentBase(parent)
    , m_deviceType(deviceType)
    , m_serialManager(serialManager)
{
    setupUI();
    loadWiFiHistory();
    retranslateUI();
}

WiFiSetupWidget::~WiFiSetupWidget()
{
    saveWiFiHistory();
}

void WiFiSetupWidget::setupUI()
{
    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("WiFiScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 创建内容容器
    m_contentWidget = new QWidget();
    m_contentWidget->setObjectName("contentWidget");
    
    // 主布局
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(m_scrollArea);
    
    // 内容主布局
    m_mainLayout = new QHBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(30, 30, 30, 30);
    m_mainLayout->setSpacing(40);
    
    // 设置左右面板
    setupLeftPanel();
    setupRightPanel();
    
    // 添加到主布局，设置比例
    m_mainLayout->addLayout(m_leftLayout, 2);  // 左侧占2/3
    m_mainLayout->addLayout(m_rightLayout, 1); // 右侧占1/3
    
    // 设置滚动区域内容
    m_scrollArea->setWidget(m_contentWidget);
}

void WiFiSetupWidget::setupLeftPanel()
{
    m_leftLayout = new QVBoxLayout();
    m_leftLayout->setSpacing(20);
    
    // 标题和描述
    m_titleLabel = new QLabel();
    m_titleLabel->setObjectName("WiFiSetupTitle");
    m_titleLabel->setWordWrap(true);
    
    m_descLabel = new QLabel();
    m_descLabel->setObjectName("WiFiSetupDesc");
    m_descLabel->setWordWrap(true);
    
    // WiFi配置组
    m_wifiGroupBox = new QGroupBox();
    m_wifiGroupBox->setObjectName("WiFiConfigGroup");
    
    m_formLayout = new QFormLayout(m_wifiGroupBox);
    m_formLayout->setSpacing(15);
    m_formLayout->setContentsMargins(20, 20, 20, 20);
    
    // WiFi名称输入框
    m_wifiNameEdit = new QLineEdit();
    m_wifiNameEdit->setObjectName("WiFiNameEdit");
    m_wifiNameEdit->setPlaceholderText("请输入WiFi网络名称");
    
    // WiFi密码输入框
    m_wifiPasswordEdit = new QLineEdit();
    m_wifiPasswordEdit->setObjectName("WiFiPasswordEdit");
    m_wifiPasswordEdit->setPlaceholderText("请输入WiFi密码");
    m_wifiPasswordEdit->setEchoMode(QLineEdit::Password);
    
    // 显示密码复选框
    m_showPasswordCheckBox = new QCheckBox();
    m_showPasswordCheckBox->setObjectName("ShowPasswordCheckBox");
    
    // 状态标签
    m_statusLabel = new QLabel();
    m_statusLabel->setObjectName("WiFiStatusLabel");
    m_statusLabel->setWordWrap(true);
    
    // 添加到表单布局
    m_formLayout->addRow("WiFi名称:", m_wifiNameEdit);
    m_formLayout->addRow("WiFi密码:", m_wifiPasswordEdit);
    m_formLayout->addRow("", m_showPasswordCheckBox);
    m_formLayout->addRow("状态:", m_statusLabel);
    
    // 按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(15);
    
    m_clearButton = new QPushButton();
    m_clearButton->setObjectName("ClearButton");
    
    m_sendConfigButton = new QPushButton();
    m_sendConfigButton->setObjectName("SendConfigButton");
    
    m_buttonLayout->addWidget(m_clearButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_sendConfigButton);
    
    // 添加到左侧布局
    m_leftLayout->addWidget(m_titleLabel);
    m_leftLayout->addWidget(m_descLabel);
    m_leftLayout->addWidget(m_wifiGroupBox);
    m_leftLayout->addLayout(m_buttonLayout);
    m_leftLayout->addStretch();
    
    // 连接信号
    connect(m_showPasswordCheckBox, &QCheckBox::toggled, this, &WiFiSetupWidget::onShowPasswordToggled);
    connect(m_clearButton, &QPushButton::clicked, this, &WiFiSetupWidget::onClearClicked);
    connect(m_sendConfigButton, &QPushButton::clicked, this, &WiFiSetupWidget::onSendConfigClicked);
    connect(m_wifiNameEdit, &QLineEdit::textChanged, this, &WiFiSetupWidget::validateInputs);
    connect(m_wifiPasswordEdit, &QLineEdit::textChanged, this, &WiFiSetupWidget::validateInputs);
}

void WiFiSetupWidget::setupRightPanel()
{
    m_rightLayout = new QVBoxLayout();
    m_rightLayout->setSpacing(15);
    
    // 历史记录组
    m_historyGroupBox = new QGroupBox();
    m_historyGroupBox->setObjectName("WiFiHistoryGroup");
    
    m_historyLayout = new QVBoxLayout(m_historyGroupBox);
    m_historyLayout->setContentsMargins(15, 20, 15, 15);
    m_historyLayout->setSpacing(10);
    
    // 历史记录数量标签
    m_historyCountLabel = new QLabel();
    m_historyCountLabel->setObjectName("WiFiHistoryCount");
    
    // 历史记录列表
    m_historyList = new QListWidget();
    m_historyList->setObjectName("WiFiHistoryList");
    m_historyList->setAlternatingRowColors(true);
    
    // 清除历史按钮
    m_clearHistoryButton = new QPushButton();
    m_clearHistoryButton->setObjectName("ClearHistoryButton");
    
    // 添加到历史记录布局
    m_historyLayout->addWidget(m_historyCountLabel);
    m_historyLayout->addWidget(m_historyList);
    m_historyLayout->addWidget(m_clearHistoryButton);
    
    // 添加到右侧布局
    m_rightLayout->addWidget(m_historyGroupBox);
    
    // 连接信号
    connect(m_historyList, &QListWidget::itemClicked, this, &WiFiSetupWidget::onHistoryItemClicked);
    connect(m_clearHistoryButton, &QPushButton::clicked, this, &WiFiSetupWidget::onClearHistoryClicked);
}

void WiFiSetupWidget::retranslateUI()
{
    m_titleLabel->setText("WiFi网络配置");
    m_descLabel->setText("请输入要连接的WiFi网络信息，设备将自动连接到指定的网络。");
    
    m_wifiGroupBox->setTitle("网络设置");
    m_showPasswordCheckBox->setText("显示密码");
    m_clearButton->setText("清除");
    m_sendConfigButton->setText("发送配置");
    
    m_historyGroupBox->setTitle("历史记录");
    m_clearHistoryButton->setText("清除历史");
    
    updateConnectionStatus("等待配置...");
    
    // 更新历史记录数量
    int count = m_historyList->count();
    m_historyCountLabel->setText(QString("共 %1 条记录").arg(count));
}

void WiFiSetupWidget::setWiFiInfo(const QString &ssid, const QString &password)
{
    m_wifiNameEdit->setText(ssid);
    m_wifiPasswordEdit->setText(password);
    validateInputs();
}

void WiFiSetupWidget::onSendConfigClicked()
{
    QString ssid = m_wifiNameEdit->text().trimmed();
    QString password = m_wifiPasswordEdit->text();
    
    if (ssid.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入WiFi网络名称！");
        m_wifiNameEdit->setFocus();
        return;
    }
    
    if (password.length() < 8 && !password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "WiFi密码长度不能少于8位！");
        m_wifiPasswordEdit->setFocus();
        return;
    }
    
    // 发送配置到设备
    if (m_serialManager) {
        try {
            m_serialManager->sendWiFiConfig(ssid.toStdString(), password.toStdString());
            updateConnectionStatus("正在发送配置...");
            
            // 添加到历史记录
            addToHistory(ssid, password);
            
            // 发送成功信号
            emit configurationSuccess(m_deviceType, ssid);
            
            updateConnectionStatus("配置发送成功！设备正在连接网络...");
            
        } catch (const std::exception& e) {
            QString error = QString("发送配置失败: %1").arg(e.what());
            updateConnectionStatus(error);
            emit configurationFailed(error);
            QMessageBox::critical(this, "发送失败", error);
        }
    } else {
        QString error = "串口管理器未初始化";
        updateConnectionStatus(error);
        emit configurationFailed(error);
        QMessageBox::critical(this, "系统错误", error);
    }
}

void WiFiSetupWidget::onClearClicked()
{
    m_wifiNameEdit->clear();
    m_wifiPasswordEdit->clear();
    updateConnectionStatus("等待配置...");
    m_wifiNameEdit->setFocus();
}

void WiFiSetupWidget::onShowPasswordToggled(bool show)
{
    m_wifiPasswordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void WiFiSetupWidget::onHistoryItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    // 从项目数据中获取WiFi信息
    QVariant data = item->data(Qt::UserRole);
    if (data.isValid()) {
        QJsonObject wifiInfo = data.toJsonObject();
        QString ssid = wifiInfo["ssid"].toString();
        QString password = wifiInfo["password"].toString();
        
        // 填充到输入框
        setWiFiInfo(ssid, password);
        updateConnectionStatus("已从历史记录加载配置");
    }
}

void WiFiSetupWidget::onClearHistoryClicked()
{
    if (m_wifiHistory.isEmpty()) {
        return;
    }
    
    int ret = QMessageBox::question(this, "确认操作", 
        "确定要清除所有WiFi历史记录吗？\n此操作无法撤销。",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_wifiHistory = QJsonArray();
        m_historyList->clear();
        saveWiFiHistory();
        
        // 更新历史记录数量
        m_historyCountLabel->setText("共 0 条记录");
        updateConnectionStatus("历史记录已清除");
    }
}

void WiFiSetupWidget::loadWiFiHistory()
{
    QSettings settings;
    settings.beginGroup(SETTINGS_GROUP);
    
    QByteArray data = settings.value(WIFI_HISTORY_KEY).toByteArray();
    if (!data.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            m_wifiHistory = doc.array();
        }
    }
    
    settings.endGroup();
    
    // 更新历史列表显示
    m_historyList->clear();
    for (int i = 0; i < m_wifiHistory.size(); ++i) {
        QJsonObject wifiInfo = m_wifiHistory[i].toObject();
        QString ssid = wifiInfo["ssid"].toString();
        QString timestamp = wifiInfo["timestamp"].toString();
        
        // 创建列表项
        QString displayText = QString("%1\n%2").arg(ssid, timestamp);
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, wifiInfo);
        item->setToolTip(QString("SSID: %1\n时间: %2\n点击加载此配置").arg(ssid, timestamp));
        
        m_historyList->addItem(item);
    }
}

void WiFiSetupWidget::saveWiFiHistory()
{
    QSettings settings;
    settings.beginGroup(SETTINGS_GROUP);
    
    QJsonDocument doc(m_wifiHistory);
    settings.setValue(WIFI_HISTORY_KEY, doc.toJson(QJsonDocument::Compact));
    
    settings.endGroup();
}

void WiFiSetupWidget::addToHistory(const QString &ssid, const QString &password)
{
    // 检查是否已存在相同的SSID
    for (int i = 0; i < m_wifiHistory.size(); ++i) {
        QJsonObject obj = m_wifiHistory[i].toObject();
        if (obj["ssid"].toString() == ssid) {
            // 移除旧记录
            m_wifiHistory.removeAt(i);
            break;
        }
    }
    
    // 创建新记录
    QJsonObject wifiInfo;
    wifiInfo["ssid"] = ssid;
    wifiInfo["password"] = password;
    wifiInfo["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    
    // 添加到数组开头
    m_wifiHistory.prepend(wifiInfo);
    
    // 限制历史记录数量
    while (m_wifiHistory.size() > MAX_HISTORY_COUNT) {
        m_wifiHistory.removeLast();
    }
    
    // 重新加载历史列表
    loadWiFiHistory();
    
    // 更新历史记录数量
    m_historyCountLabel->setText(QString("共 %1 条记录").arg(m_wifiHistory.size()));
    
    // 保存到设置
    saveWiFiHistory();
}

void WiFiSetupWidget::updateConnectionStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

void WiFiSetupWidget::validateInputs()
{
    QString ssid = m_wifiNameEdit->text().trimmed();
    bool isValid = !ssid.isEmpty();
    
    m_sendConfigButton->setEnabled(isValid);
    
    if (ssid.isEmpty()) {
        updateConnectionStatus("请输入WiFi网络名称");
    } else {
        updateConnectionStatus("配置已就绪，点击发送");
    }
}
