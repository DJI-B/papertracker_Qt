#include "components/wifi_setup_widget.h"
#include "serial.hpp"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QSettings>
#include <QMessageBox>

WiFiSetupWidget::WiFiSetupWidget(const QString &deviceType, std::shared_ptr<SerialPortManager> serialManager, QWidget *parent)
    : WidgetComponentBase(parent)
    , m_deviceType(deviceType)
    , m_serialManager(serialManager)
{
    setupUI();
    retranslateUI();
}

void WiFiSetupWidget::setupUI()
{
    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("WiFiScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    // 创建内容容器
    QWidget *contentContainer = new QWidget();
    contentContainer->setObjectName("contentWidget");
    contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout *mainLayout = new QVBoxLayout(contentContainer);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    // 标题
    m_titleLabel = new QLabel();
    m_titleLabel->setObjectName("WiFiSetupTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setFixedHeight(32);

    // 描述
    m_descLabel = new QLabel();
    m_descLabel->setObjectName("WiFiSetupDesc");
    m_descLabel->setAlignment(Qt::AlignCenter);
    m_descLabel->setWordWrap(true);
    m_descLabel->setFixedHeight(40);

    // 创建水平布局来分割WiFi配置和历史记录
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // WiFi配置组
    m_wifiGroupBox = new QGroupBox();
    m_wifiGroupBox->setObjectName("WiFiConfigGroupBox");

    QVBoxLayout *wifiLayout = new QVBoxLayout(m_wifiGroupBox);
    wifiLayout->setSpacing(8);
    wifiLayout->setContentsMargins(15, 15, 15, 15);

    // 表单布局
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // 网络名称标签和输入框
    m_networkNameLabel = new QLabel();
    m_networkNameLabel->setObjectName("NetworkNameLabel");

    m_wifiNameEdit = new QLineEdit();
    m_wifiNameEdit->setObjectName("WiFiNameEdit");
    m_wifiNameEdit->setFixedHeight(32);
    m_wifiNameEdit->setPlaceholderText("Enter WiFi network name");

    // 密码标签和输入框
    m_passwordLabel = new QLabel();
    m_passwordLabel->setObjectName("PasswordLabel");

    m_wifiPasswordEdit = new QLineEdit();
    m_wifiPasswordEdit->setObjectName("WiFiPasswordEdit");
    m_wifiPasswordEdit->setFixedHeight(32);
    m_wifiPasswordEdit->setEchoMode(QLineEdit::Password);
    m_wifiPasswordEdit->setPlaceholderText("Enter WiFi password");

    // 显示密码复选框
    m_showPasswordCheckBox = new QCheckBox();
    m_showPasswordCheckBox->setObjectName("ShowPasswordCheckBox");

    // 按钮容器
    QWidget *buttonContainer = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(8);

    m_clearButton = new QPushButton();
    m_clearButton->setObjectName("ClearButton");
    m_clearButton->setFixedHeight(32);

    m_sendConfigButton = new QPushButton();
    m_sendConfigButton->setObjectName("SendConfigButton");
    m_sendConfigButton->setFixedHeight(32);
    m_sendConfigButton->setEnabled(false);

    buttonLayout->addWidget(m_clearButton);
    buttonLayout->addWidget(m_sendConfigButton);
    buttonLayout->addStretch();

    // 状态标签
    m_statusLabel = new QLabel();
    m_statusLabel->setObjectName("StatusLabel");

    m_wifiStatusLabel = new QLabel();
    m_wifiStatusLabel->setObjectName("WiFiStatusLabel");
    m_wifiStatusLabel->setWordWrap(true);
    m_wifiStatusLabel->setFixedHeight(18);

    // 添加到表单布局
    formLayout->addRow(m_networkNameLabel, m_wifiNameEdit);
    formLayout->addRow(m_passwordLabel, m_wifiPasswordEdit);
    formLayout->addRow("", m_showPasswordCheckBox);
    formLayout->addRow("", buttonContainer);
    formLayout->addRow(m_statusLabel, m_wifiStatusLabel);

    wifiLayout->addLayout(formLayout);

    // 历史记录组
    m_historyGroupBox = new QGroupBox();
    m_historyGroupBox->setObjectName("WiFiHistoryGroupBox");

    QVBoxLayout *historyLayout = new QVBoxLayout(m_historyGroupBox);
    historyLayout->setSpacing(8);
    historyLayout->setContentsMargins(15, 15, 15, 15);

    m_historyListWidget = new QListWidget();
    m_historyListWidget->setObjectName("WiFiHistoryList");
    m_historyListWidget->setMaximumHeight(150);

    m_clearHistoryButton = new QPushButton();
    m_clearHistoryButton->setObjectName("ClearHistoryButton");
    m_clearHistoryButton->setFixedHeight(28);

    historyLayout->addWidget(m_historyListWidget);
    historyLayout->addWidget(m_clearHistoryButton);

    // 设备连接状态提示
    m_deviceStatusLabel = new QLabel();
    m_deviceStatusLabel->setAlignment(Qt::AlignCenter);
    m_deviceStatusLabel->setFixedHeight(18);

    // 添加配置表单和历史列表到水平布局
    contentLayout->addWidget(m_wifiGroupBox, 3);
    contentLayout->addWidget(m_historyGroupBox, 2);

    // 添加到主布局
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(m_descLabel);
    mainLayout->addSpacing(8);
    mainLayout->addLayout(contentLayout);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(m_deviceStatusLabel);

    // 设置滚动区域
    scrollArea->setWidget(contentContainer);

    // 创建最终容器
    QVBoxLayout *finalLayout = new QVBoxLayout(this);
    finalLayout->setContentsMargins(0, 0, 0, 0);
    finalLayout->addWidget(scrollArea);

    // 连接信号
    connect(m_sendConfigButton, &QPushButton::clicked, this, &WiFiSetupWidget::onSendConfigClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &WiFiSetupWidget::onClearClicked);
    connect(m_showPasswordCheckBox, &QCheckBox::toggled, this, &WiFiSetupWidget::onShowPasswordToggled);
    connect(m_historyListWidget, &QListWidget::itemClicked, this, &WiFiSetupWidget::onHistoryItemClicked);
    connect(m_clearHistoryButton, &QPushButton::clicked, this, &WiFiSetupWidget::onClearHistoryClicked);
    connect(m_wifiNameEdit, &QLineEdit::textChanged, this, &WiFiSetupWidget::validateInputs);
    connect(m_wifiPasswordEdit, &QLineEdit::textChanged, this, &WiFiSetupWidget::validateInputs);
}

void WiFiSetupWidget::retranslateUI()
{
    m_titleLabel->setText(tr("WiFi Configuration"));
    m_descLabel->setText(tr("Configure WiFi settings for your tracking device"));
    m_wifiGroupBox->setTitle(tr("WiFi Settings"));
    m_historyGroupBox->setTitle(tr("Connection History"));
    m_networkNameLabel->setText(tr("Network Name:"));
    m_passwordLabel->setText(tr("Password:"));
    m_showPasswordCheckBox->setText(tr("Show Password"));
    m_clearButton->setText(tr("Clear"));
    m_sendConfigButton->setText(tr("Send Configuration"));
    m_clearHistoryButton->setText(tr("Clear History"));
    m_statusLabel->setText(tr("Status:"));
    m_wifiStatusLabel->setText(tr("Ready to configure"));
    m_deviceStatusLabel->setText(tr("Please connect your device via USB cable"));
}

void WiFiSetupWidget::updateWifiInfo(const QString &wifiName, const QString &wifiPassword)
{
    if (!wifiName.isEmpty()) {
        m_wifiNameEdit->setText(wifiName);
    }
    if (!wifiPassword.isEmpty()) {
        m_wifiPasswordEdit->setText(wifiPassword);
    }
}

void WiFiSetupWidget::onSendConfigClicked()
{
    QString wifiName = m_wifiNameEdit->text().trimmed();
    QString wifiPassword = m_wifiPasswordEdit->text();

    if (wifiName.isEmpty()) {
        QMessageBox::warning(this, tr("Invalid Input"), tr("Please enter a WiFi network name."));
        return;
    }

    if (m_serialManager) {
        try {
            // 发送WiFi配置信息
            /*m_serialManager->send_wifi_info(wifiName.toStdString(), wifiPassword.toStdString());*/
            
            m_wifiStatusLabel->setText(tr("Configuration sent successfully"));
            m_wifiStatusLabel->setStyleSheet("color: green;");
            
            // 添加到历史记录
            addToHistory(wifiName);
            
            // 发送成功信号
            emit configurationSuccess(m_deviceType, wifiName);
            
        } catch (const std::exception& e) {
            m_wifiStatusLabel->setText(tr("Failed to send configuration: %1").arg(QString::fromStdString(e.what())));
            m_wifiStatusLabel->setStyleSheet("color: red;");
        }
    } else {
        m_wifiStatusLabel->setText(tr("Serial manager not available"));
        m_wifiStatusLabel->setStyleSheet("color: red;");
    }
}

void WiFiSetupWidget::onClearClicked()
{
    m_wifiNameEdit->clear();
    m_wifiPasswordEdit->clear();
    m_wifiStatusLabel->setText(tr("Ready to configure"));
    m_wifiStatusLabel->setStyleSheet("");
}

void WiFiSetupWidget::onShowPasswordToggled(bool checked)
{
    m_wifiPasswordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void WiFiSetupWidget::onHistoryItemClicked(QListWidgetItem *item)
{
    if (item) {
        QString wifiName = item->text();
        m_wifiNameEdit->setText(wifiName);
        m_wifiPasswordEdit->clear(); // 出于安全考虑，不保存密码
        m_wifiPasswordEdit->setFocus();
    }
}

void WiFiSetupWidget::onClearHistoryClicked()
{
    int result = QMessageBox::question(this, tr("Clear History"),
        tr("Are you sure you want to clear all WiFi connection history?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_historyListWidget->clear();
        
        // 清除设置中的历史记录
        QSettings settings;
        settings.remove("WiFiHistory");
    }
}

void WiFiSetupWidget::validateInputs()
{
    QString wifiName = m_wifiNameEdit->text().trimmed();
    bool isValid = !wifiName.isEmpty();
    
    m_sendConfigButton->setEnabled(isValid);
}

void WiFiSetupWidget::addToHistory(const QString &wifiName)
{
    if (wifiName.isEmpty() || isWifiNameInHistory(wifiName)) {
        return;
    }

    // 添加到列表控件
    m_historyListWidget->insertItem(0, wifiName);

    // 限制历史记录数量
    while (m_historyListWidget->count() > 10) {
        QListWidgetItem *item = m_historyListWidget->takeItem(m_historyListWidget->count() - 1);
        delete item;
    }

    // 保存到设置
    QSettings settings;
    QStringList history;
    for (int i = 0; i < m_historyListWidget->count(); ++i) {
        history << m_historyListWidget->item(i)->text();
    }
    settings.setValue("WiFiHistory", history);
}

bool WiFiSetupWidget::isWifiNameInHistory(const QString &wifiName)
{
    for (int i = 0; i < m_historyListWidget->count(); ++i) {
        if (m_historyListWidget->item(i)->text() == wifiName) {
            return true;
        }
    }
    return false;
}
