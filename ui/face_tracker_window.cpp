//
// Created by JellyfishKnight on 25-3-11.
//
/*
 * PaperTracker - 面部追踪应用程序
 * Copyright (C) 2025 PAPER TRACKER
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file contains code from projectbabble:
 * Copyright 2023 Sameer Suri
 * Licensed under the Apache License, Version 2.0
 */

#include <opencv2/imgproc.hpp>
#include "face_tracker_window.hpp"
#include <QMessageBox>
#include <codecvt>
#include <locale>
#include <windows.h>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>
#include <roi_event.hpp>
#include <QInputDialog>
#include <QBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
PaperFaceTrackerWindow::PaperFaceTrackerWindow(QWidget *parent)
    : QWidget(parent)
{
    if (instance == nullptr)
        instance = this;
    else
        throw std::exception(QApplication::translate("PaperTrackerMainWindow", "当前已经打开了面捕窗口，请不要重复打开").toUtf8().constData());
    // 基本UI设置
    setFixedSize(848, 538);
    InitUi();
    InitLayout();
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    LogText->setMaximumBlockCount(200);
    append_log_window(LogText);
    LOG_INFO("系统初始化中...");
    // 初始化串口连接状态
    SerialConnectLabel->setText(QApplication::translate("PaperTrackerMainWindow", "有线模式未连接"));
    WifiConnectLabel->setText(tr("无线模式未连接"));
    // 初始化页面导航
    bound_pages();

    // 初始化亮度控制相关成员
    current_brightness = 0;
    // 连接信号和槽
    connect_callbacks();
    // 添加输入框焦点事件处理
    SSIDText->installEventFilter(this);
    PasswordText->installEventFilter(this);
    // 允许Tab键在输入框之间跳转
    SSIDText->setTabChangesFocus(true);
    PasswordText->setTabChangesFocus(true);
    // 清除所有控件的初始焦点，确保没有文本框自动获得焦点
    setFocus();
    config_writer = std::make_shared<ConfigWriter>("./config.json");

    // 添加ROI事件
    auto *roiFilter = new ROIEventFilter([this] (QRect rect, bool isEnd, int tag)
    {
        int x = rect.x ();
        int y = rect.y ();
        int width = rect.width ();
        int height = rect.height ();

        // 规范化宽度和高度为正值
        if (width < 0) {
            x += width;
            width = -width;
        }
        if (height < 0) {
            y += height;
            height = -height;
        }

        // 裁剪坐标到图像边界内
        if (x < 0) {
            width += x;  // 减少宽度
            x = 0;       // 将 x 设为 0
        }
        if (y < 0) {
            height += y; // 减少高度
            y = 0;       // 将 y 设为 0
        }

        // 确保 ROI 不超出图像边界
        if (x + width > 280) {
            width = 280 - x;
        }
        if (y + height > 280) {
            height = 280 - y;
        }
        // 确保最终的宽度和高度为正值
        width = max(0, width);
        height = max(0, height);

        // 更新 roi_rect
        roi_rect.is_roi_end = isEnd;
        roi_rect = Rect (x, y, width, height);
    },ImageLabel);
    ImageLabel->installEventFilter(roiFilter);
    ImageLabelCal->installEventFilter(roiFilter);
    inference = std::make_shared<FaceInference>();
    osc_manager = std::make_shared<OscManager>();
    set_config();
    // Load model
    LOG_INFO("正在加载推理模型...");
    try {
        inference->load_model("");
        LOG_INFO("模型加载完成");
    } catch (const std::exception& e) {
        // 使用Qt方式记录日志，而不是minilog
        LOG_ERROR("错误: 模型加载异常: {}", e.what());
    }
    // 初始化OSC管理器
    LOG_INFO("正在初始化OSC...");
    if (osc_manager->init("127.0.0.1", 8888)) {
        osc_manager->setLocationPrefix("");
        LOG_INFO("OSC初始化成功");
    } else {
        LOG_ERROR("OSC初始化失败，请检查网络连接");
    }
    // 初始化串口和wifi
    serial_port_manager = std::make_shared<SerialPortManager>();
    image_downloader = std::make_shared<ESP32VideoStream>();
    LOG_INFO("初始化有线模式");
    serial_port_manager->init();
    // init serial port manager
    serial_port_manager->registerCallback(
        PACKET_DEVICE_STATUS,
        [this](const std::string& ip, int brightness, int power, int version) {
            if (version != 1)
            {
                static bool version_warning = false;
                QString version_str = version == 2 ? QApplication::translate("PaperTrackerMainWindow", "左眼追") : QApplication::translate("PaperTrackerMainWindow", "右眼追");
                if (!version_warning)
                {
                    QMessageBox msgBox;
                    msgBox.setWindowIcon(this->windowIcon());
                    msgBox.setText(QApplication::translate("PaperTrackerMainWindow", "检测到") + version_str + QApplication::translate("PaperTrackerMainWindow", "设备，请打开眼追界面进行设置"));
                    msgBox.exec();
                    version_warning = true;
                }
                serial_port_manager->stop();
                return ;
            }
            // 使用Qt的线程安全方式更新UI
            QMetaObject::invokeMethod(this, [ip, brightness, power, version, this]() {
                // 只在 IP 地址变化时更新显示
                if (current_ip_ != "http://" + ip)
                {
                    current_ip_ = "http://" + ip;
                    // 更新IP地址显示，添加 http:// 前缀
                    this->setIPText(QString::fromStdString(current_ip_));
                    LOG_INFO("IP地址已更新: {}", current_ip_);
                    start_image_download();
                }
                firmware_version = std::to_string(version);
                // 可以添加其他状态更新的日志，如果需要的话
            }, Qt::QueuedConnection);
        }
    );
    showSerialData = false; // 确保初始状态为false
    serial_port_manager->registerRawDataCallback([this](const std::string& data) {
        if (showSerialData) {
            serialRawDataLog.push_back(data);
            LOG_INFO("串口原始数据: {}", data);
        }
    });
    LOG_DEBUG("等待有线模式面捕连接");
    while (serial_port_manager->status() == SerialStatus::CLOSED) {}
    LOG_DEBUG("有线模式面捕连接完毕");

    if (serial_port_manager->status() == SerialStatus::FAILED)
    {
        setSerialStatusLabel("有线模式面捕连接失败");
        LOG_WARN("有线模式面捕未连接，尝试从配置文件中读取地址...");
        if (!config.wifi_ip.empty())
        {
            LOG_INFO("从配置文件中读取地址成功");
            current_ip_ = config.wifi_ip;
            start_image_download();
        } else
        {
            QMessageBox msgBox;
            msgBox.setWindowIcon(this->windowIcon());
            msgBox.setText(QApplication::translate("PaperTrackerMainWindow", "未找到配置文件信息，请将面捕通过数据线连接到电脑进行首次配置"));
            msgBox.exec();
        }
    } else
    {
        LOG_INFO("有线模式面捕连接成功");
        setSerialStatusLabel("有线模式面捕连接成功");
    }
    // 读取配置文件
    set_config();
    // setupKalmanFilterControls();
    create_sub_threads();
    // 创建自动保存配置的定时器
    auto_save_timer = new QTimer(this);
    connect(auto_save_timer, &QTimer::timeout, this, [this]() {
        config = generate_config();
        config_writer->write_config(config);
        LOG_DEBUG("面捕配置已自动保存");
    });
    auto_save_timer->start(10000); // 10000毫秒 = 10秒
    retranslateUI();
    connect(&TranslatorManager::instance(), &TranslatorManager::languageSwitched,
        this, &PaperFaceTrackerWindow::retranslateUI);
}

void PaperFaceTrackerWindow::InitUi() {
    // 仅保留控件创建和基本属性设置
    this->setMinimumSize(839, 561);
    this->setStyleSheet(QString::fromUtf8(""));

    // 初始化 QStackedWidget 和页面
    stackedWidget = new QStackedWidget(this);
    stackedWidget->setObjectName("stackedWidget");

    page = new QWidget(this);
    page->setObjectName("page");
    page_2 = new QWidget(this);
    page_2->setObjectName("page_2");

    QFont font;
    font.setBold(true);
    font.setItalic(true);

    // 初始化主页面控件
    ImageLabel = new QLabel(page);
    ImageLabel->setObjectName("ImageLabel");
    ImageLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    SSIDText = new QPlainTextEdit(page);
    SSIDText->setObjectName("SSIDText");
    SSIDText->setFixedHeight(40);
    PasswordText = new QPlainTextEdit(page);
    PasswordText->setObjectName("PasswordText");
    PasswordText->setFixedHeight(40);
    BrightnessBar = new QScrollBar(page);
    BrightnessBar->setObjectName("BrightnessBar");
    BrightnessBar->setOrientation(Qt::Horizontal);
    BrightnessBar->setFixedHeight(20);
    label = new QLabel(page);
    label->setObjectName("label");
    label->setFixedHeight(20);
    wifi_send_Button = new QPushButton(page);
    wifi_send_Button->setObjectName("wifi_send_Button");
    wifi_send_Button->setFixedSize(92, 92);
    FlashFirmwareButton = new QPushButton(page);
    FlashFirmwareButton->setObjectName("FlashFirmwareButton");
    FlashFirmwareButton->setFixedHeight(40);
    LogText = new QPlainTextEdit(page);
    LogText->setObjectName("LogText");
    LogText->setReadOnly(true);
    label_2 = new QLabel(page);
    label_2->setObjectName("label_2");
    textEdit = new QTextEdit(page);
    textEdit->setObjectName("textEdit");
    textEdit->setReadOnly(true);
    textEdit->setFixedHeight(40);

    label_16 = new QLabel(page);
    label_16->setObjectName("label_16");
    label_16->setFixedHeight(30);
    restart_Button = new QPushButton(page);
    restart_Button->setObjectName("restart_Button");
    restart_Button->setFixedHeight(40);
    label_17 = new QLabel(page);
    label_17->setObjectName("label_17");
    label_17->setFixedHeight(20);
    RotateImageBar = new QScrollBar(page);
    RotateImageBar->setObjectName("RotateImageBar");
    RotateImageBar->setOrientation(Qt::Horizontal);
    RotateImageBar->setFixedHeight(20);
    UseFilterBox = new QCheckBox(page);
    UseFilterBox->setObjectName("UseFilterBox");
    UseFilterBox->setFixedHeight(20);
    EnergyModeBox = new QComboBox(page);
    EnergyModeBox->setObjectName("EnergyModeBox");
    EnergyModeBox->setFixedHeight(20);
    label_18 = new QLabel(page);
    label_18->setObjectName("label_18");
    label_18->setFixedHeight(20);
    ShowSerialDataButton = new QPushButton(page);
    ShowSerialDataButton->setObjectName("ShowSerialDataButton");

    // 创建滚动区域并设置内容容器
    scrollArea = new QScrollArea(page_2);
    scrollArea->setMaximumWidth(500);
    scrollArea->setObjectName("scrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding); // 允许扩展

    // 初始化内容容器
    scrollAreaWidgetContents = new QWidget();
    scrollAreaWidgetContents->setMaximumWidth(600);
    scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
    scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding); // 允许扩展
    scrollArea->setWidget(scrollAreaWidgetContents);

    // 初始化校准页面参数名称
    label_5 = new QLabel(scrollAreaWidgetContents);
    label_6 = new QLabel(scrollAreaWidgetContents);
    label_7 = new QLabel(scrollAreaWidgetContents);
    label_8 = new QLabel(scrollAreaWidgetContents);
    label_9 = new QLabel(scrollAreaWidgetContents);
    label_10 = new QLabel(scrollAreaWidgetContents);
    label_11 = new QLabel(scrollAreaWidgetContents);
    label_12 = new QLabel(scrollAreaWidgetContents);
    label_13 = new QLabel(scrollAreaWidgetContents);
    label_14 = new QLabel(scrollAreaWidgetContents);
    label_15 = new QLabel(scrollAreaWidgetContents);
    label_31 = new QLabel(scrollAreaWidgetContents);
    label_mouthClose = new QLabel(scrollAreaWidgetContents);
    label_mouthFunnel = new QLabel(scrollAreaWidgetContents);
    label_mouthPucker = new QLabel(scrollAreaWidgetContents);
    label_mouthRollLower = new QLabel(scrollAreaWidgetContents);
    label_mouthRollUpper = new QLabel(scrollAreaWidgetContents);
    label_mouthShrugLower = new QLabel(scrollAreaWidgetContents);
    label_mouthShrugUpper = new QLabel(scrollAreaWidgetContents);

    // 校准页面参数控件
    CheekPuffLeftOffset = new QLineEdit(scrollAreaWidgetContents);
    CheekPuffRightOffset = new QLineEdit(scrollAreaWidgetContents);
    JawOpenOffset = new QLineEdit(scrollAreaWidgetContents);
    TongueOutOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthCloseOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthFunnelOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthPuckerOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthRollUpperOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthRollLowerOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthShrugUpperOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthShrugLowerOffset = new QLineEdit(scrollAreaWidgetContents);

    // 滑动条控件
    CheekPuffLeftBar = new QScrollBar(scrollAreaWidgetContents);
    CheekPuffRightBar = new QScrollBar(scrollAreaWidgetContents);
    JawOpenBar = new QScrollBar(scrollAreaWidgetContents);
    JawLeftBar = new QScrollBar(scrollAreaWidgetContents);
    MouthLeftBar = new QScrollBar(scrollAreaWidgetContents);
    JawRightBar = new QScrollBar(scrollAreaWidgetContents);
    TongueOutBar = new QScrollBar(scrollAreaWidgetContents);
    MouthRightBar = new QScrollBar(scrollAreaWidgetContents);
    TongueDownBar = new QScrollBar(scrollAreaWidgetContents);
    TongueUpBar = new QScrollBar(scrollAreaWidgetContents);
    TongueRightBar = new QScrollBar(scrollAreaWidgetContents);
    TongueLeftBar = new QScrollBar(scrollAreaWidgetContents);
    MouthCloseBar = new QScrollBar(scrollAreaWidgetContents);
    MouthFunnelBar = new QScrollBar(scrollAreaWidgetContents);
    MouthPuckerBar = new QScrollBar(scrollAreaWidgetContents);
    MouthRollUpperBar = new QScrollBar(scrollAreaWidgetContents);
    MouthRollLowerBar = new QScrollBar(scrollAreaWidgetContents);
    MouthShrugUpperBar = new QScrollBar(scrollAreaWidgetContents);
    MouthShrugLowerBar = new QScrollBar(scrollAreaWidgetContents);

    // 进度条控件
    CheekPullLeftValue = new QProgressBar(scrollAreaWidgetContents);
    CheekPullRightValue = new QProgressBar(scrollAreaWidgetContents);
    JawOpenValue = new QProgressBar(scrollAreaWidgetContents);
    JawLeftValue = new QProgressBar(scrollAreaWidgetContents);
    MouthLeftValue = new QProgressBar(scrollAreaWidgetContents);
    JawRightValue = new QProgressBar(scrollAreaWidgetContents);
    TongueOutValue = new QProgressBar(scrollAreaWidgetContents);
    MouthRightValue = new QProgressBar(scrollAreaWidgetContents);
    TongueDownValue = new QProgressBar(scrollAreaWidgetContents);
    TongueUpValue = new QProgressBar(scrollAreaWidgetContents);
    TongueRightValue = new QProgressBar(scrollAreaWidgetContents);
    TongueLeftValue = new QProgressBar(scrollAreaWidgetContents);
    MouthCloseValue = new QProgressBar(scrollAreaWidgetContents);
    MouthFunnelValue = new QProgressBar(scrollAreaWidgetContents);
    MouthPuckerValue = new QProgressBar(scrollAreaWidgetContents);
    MouthRollUpperValue = new QProgressBar(scrollAreaWidgetContents);
    MouthRollLowerValue = new QProgressBar(scrollAreaWidgetContents);
    MouthShrugUpperValue = new QProgressBar(scrollAreaWidgetContents);
    MouthShrugLowerValue = new QProgressBar(scrollAreaWidgetContents);

    // 其他页面控件
    ImageLabelCal = new QLabel(page_2);
    ImageLabelCal->setObjectName("ImageLabelCal");
    label_3 = new QLabel(page_2);
    label_4 = new QLabel(page_2);
    label_4->setText("x3");
    label_19 = new QLabel(page_2);
    label_19->setText("x1");
    label_20 = new QLabel(page_2);
    MainPageButton = new QPushButton(this);
    MainPageButton->setFixedHeight(24);
    MainPageButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    CalibrationPageButton = new QPushButton(this);
    CalibrationPageButton->setFixedHeight(24);
    CalibrationPageButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    WifiConnectLabel = new QLabel(this);
    WifiConnectLabel->setFont(font);
    SerialConnectLabel = new QLabel(this);
    SerialConnectLabel->setFont(font);
    BatteryStatusLabel = new QLabel(this);
    BatteryStatusLabel->setFont(font);

    // 添加到 stackedWidget
    stackedWidget->addWidget(page);
    stackedWidget->addWidget(page_2);
    stackedWidget->setCurrentIndex(1);

    // 添加教程链接文本
    tutorialLink = new QLabel(page);
    tutorialLink->setText(QString("<a href='https://fcnk6r4c64fa.feishu.cn/wiki/VSlnw4Zr0i4VzXFkvT8TcbQFMn7c' style='color: #0066cc; font-size: 14pt; font-weight: bold;'>%1</a>")
                          .arg(QApplication::translate("PaperTrackerMainWindow", "面捕调整教程")));
    tutorialLink->setOpenExternalLinks(true);
    tutorialLink->setTextFormat(Qt::RichText);
    tutorialLink->setStyleSheet("background-color: #f0f0f0; padding: 5px; border-radius: 5px;");

}

void PaperFaceTrackerWindow::InitLayout() {
    // 主窗口整体布局
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    //顶部tab切换按钮和状态label
    auto* topButtonLayout = new QHBoxLayout();
    topButtonLayout->addWidget(MainPageButton);
    topButtonLayout->addWidget(CalibrationPageButton);
    topButtonLayout->addStretch();
    topButtonLayout->addWidget(WifiConnectLabel);
    topButtonLayout->addWidget(SerialConnectLabel);
    topButtonLayout->addWidget(BatteryStatusLabel);
    topButtonLayout->addStretch();
    topButtonLayout->setSpacing(10);  // 设置控件间距
    topButtonLayout->setContentsMargins(10, 5, 10, 5);  // 设置边距

    mainLayout->addItem(topButtonLayout);
    mainLayout->addWidget(stackedWidget);

    // **********************************************************************
    // *                          主页面布局                          *
    // **********************************************************************
    // 主页面布局
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(10, 10, 10, 10);
    pageLayout->setSpacing(10);

    auto pageContentWidget = new QWidget(page);
    // 图像和参数区域
    auto imageAndParamsLayout = new QHBoxLayout(pageContentWidget);

    // 左侧图像区域
    auto imageLayout = new QVBoxLayout();
    ImageLabel->setFixedSize(280, 280);
    ImageLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    imageLayout->addWidget(ImageLabel);
    imageLayout->addStretch();

    auto rightPanelLayout = new QVBoxLayout();
    rightPanelLayout->setSpacing(12);

    auto ConnectionLayout = new QHBoxLayout();
    ConnectionLayout->setSpacing(12);
    ConnectionLayout->setContentsMargins(0, 0, 0, 0);

    auto wifiLayout = new QVBoxLayout();
    wifiLayout->setSpacing(0);
    wifiLayout->addWidget(SSIDText);
    wifiLayout->addSpacing(12);
    wifiLayout->addWidget(PasswordText);

    auto FirmwareLayout = new QVBoxLayout();
    FirmwareLayout->addWidget(FlashFirmwareButton);
    FirmwareLayout->addWidget(restart_Button);

    ConnectionLayout->addItem(wifiLayout);
    ConnectionLayout->addWidget(wifi_send_Button);
    ConnectionLayout->addItem(FirmwareLayout);

    // 控制区域
    auto* controlLayout = new QVBoxLayout();
    controlLayout->setSpacing(8);
    // 亮度控制
    auto* brightnessLayout = new QHBoxLayout();
    brightnessLayout->addWidget(label);
    brightnessLayout->addWidget(BrightnessBar);
    BrightnessBar->setMinimumWidth(240);
    brightnessLayout->addStretch();
    controlLayout->addLayout(brightnessLayout);

    // 旋转控制
    auto* rotateLayout = new QHBoxLayout();
    rotateLayout->addWidget(label_17);
    rotateLayout->addWidget(RotateImageBar);
    RotateImageBar->setMinimumWidth(240);
    rotateLayout->addStretch();
    controlLayout->addLayout(rotateLayout);

    // 模式选择
    auto* modeLayout = new QHBoxLayout();
    modeLayout->setSpacing(8);
    EnergyModeBox->addItem(QApplication::translate("PaperTrackerMainWindow", "普通模式"));
    EnergyModeBox->addItem(QApplication::translate("PaperTrackerMainWindow", "节能模式"));
    EnergyModeBox->addItem(QApplication::translate("PaperTrackerMainWindow", "性能模式"));
    EnergyModeBox->setCurrentIndex(0);
    modeLayout->addWidget(label_18);
    modeLayout->addWidget(EnergyModeBox);
    modeLayout->addWidget(UseFilterBox);
    modeLayout->addStretch();
    controlLayout->addLayout(modeLayout);

    // IP地址
    auto* ipLayout = new QVBoxLayout();
    auto* tutorialLayout = new QHBoxLayout();
    tutorialLayout->setSpacing(8);
    ipLayout->addWidget(label_16);
    tutorialLayout->addWidget(textEdit);
    tutorialLayout->addWidget(tutorialLink);
    ipLayout->addItem(tutorialLayout);
    controlLayout->addLayout(ipLayout);

    rightPanelLayout->addItem(ConnectionLayout);
    rightPanelLayout->addItem(controlLayout);

    // 日志区域
    auto* logTextLayout = new QHBoxLayout();
    label_2->setText("日志窗口:");
    logTextLayout->addWidget(label_2);
    logTextLayout->addStretch();
    logTextLayout->addWidget(ShowSerialDataButton);

    imageAndParamsLayout->addLayout(imageLayout);
    imageAndParamsLayout->addLayout(rightPanelLayout);
    imageAndParamsLayout->setStretch(0, 1);  // 左侧图像区域固定
    imageAndParamsLayout->setStretch(1, 2);  // 右侧参数区域更大

    pageLayout->addWidget(pageContentWidget);
    pageLayout->addItem(logTextLayout);
    pageLayout->addWidget(LogText);

    // **********************************************************************
    // *                          校准页面布局                          *
    // **********************************************************************
    // 修正校准页面布局
    calibrationPageLayout = new QHBoxLayout(page_2);
    calibrationPageLayout->setContentsMargins(10, 10, 10, 10);
    calibrationPageLayout->setSpacing(10);

    auto* leftPageLayout = new QVBoxLayout(page_2);
    leftPageLayout->setContentsMargins(10, 0, 10, 10);
    leftPageLayout->setSpacing(0);

    // 创建参数布局并应用到内容容器
    auto* paramsLayout = new QGridLayout(scrollAreaWidgetContents);
    paramsLayout->setSpacing(4);
    paramsLayout->setContentsMargins(10, 10, 10, 10);

    int row = 0;
    static int maxLabelWidth = 0;
    addCalibrationParam(paramsLayout, label_5, CheekPuffLeftOffset, CheekPuffLeftBar, CheekPullLeftValue, row++);
    addCalibrationParam(paramsLayout, label_6, CheekPuffRightOffset, CheekPuffRightBar, CheekPullRightValue, row++);
    addCalibrationParam(paramsLayout, label_7, JawOpenOffset, JawOpenBar, JawOpenValue, row++);
    addCalibrationParam(paramsLayout, label_8, nullptr, JawLeftBar, JawLeftValue, row++);
    addCalibrationParam(paramsLayout, label_9, nullptr, JawRightBar, JawRightValue, row++);
    addCalibrationParam(paramsLayout, label_10, nullptr, MouthLeftBar, MouthLeftValue, row++);
    addCalibrationParam(paramsLayout, label_11, nullptr, MouthRightBar, MouthRightValue, row++);
    addCalibrationParam(paramsLayout, label_12, TongueOutOffset, TongueOutBar, TongueOutValue, row++);
    addCalibrationParam(paramsLayout, label_13, nullptr, TongueUpBar, TongueUpValue, row++);
    addCalibrationParam(paramsLayout, label_14, nullptr, TongueDownBar, TongueDownValue, row++);
    addCalibrationParam(paramsLayout, label_15, nullptr, TongueLeftBar, TongueLeftValue, row++);
    addCalibrationParam(paramsLayout, label_31, nullptr, TongueRightBar, TongueRightValue, row++);
    addCalibrationParam(paramsLayout, label_mouthClose, MouthCloseOffset, MouthCloseBar, MouthCloseValue, row++);
    addCalibrationParam(paramsLayout, label_mouthFunnel, MouthFunnelOffset, MouthFunnelBar, MouthFunnelValue, row++);
    addCalibrationParam(paramsLayout, label_mouthPucker, MouthPuckerOffset, MouthPuckerBar, MouthPuckerValue, row++);
    addCalibrationParam(paramsLayout, label_mouthRollUpper, MouthRollUpperOffset, MouthRollUpperBar, MouthRollUpperValue, row++);
    addCalibrationParam(paramsLayout, label_mouthRollLower, MouthRollLowerOffset, MouthRollLowerBar, MouthRollLowerValue, row++);
    addCalibrationParam(paramsLayout, label_mouthShrugUpper, MouthShrugUpperOffset, MouthShrugUpperBar, MouthShrugUpperValue, row++);
    addCalibrationParam(paramsLayout, label_mouthShrugLower, MouthShrugLowerOffset, MouthShrugLowerBar, MouthShrugLowerValue, row++);

    // 或者使用伸缩比例（如果希望动态适应）
    paramsLayout->setColumnStretch(0, 1); // label 列
    paramsLayout->setColumnStretch(1, 1); // 输入框paramsLayout->setColumnStretch(2, 2); // 滚动条
    paramsLayout->setColumnStretch(3, 2); // 进度条

    // 添加参数布局到 scrollArea
    scrollAreaWidgetContents->setLayout(paramsLayout);

    topTitleWidget = new QWidget(page_2);
    topTitleWidget->setMinimumWidth(270);
    auto topTitleLayout = new QHBoxLayout(topTitleWidget);
    topTitleLayout->setContentsMargins(0 ,0 ,0 ,0);
    topTitleWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    topTitleLayout->setSpacing(10);
    topTitleLayout->addSpacing(70);
    topTitleLayout->addWidget(label_20);
    topTitleLayout->addWidget(label_19);
    topTitleLayout->addWidget(label_3);
    topTitleLayout->addWidget(label_4);

    // 将滚动区域添加到校准页面布局
    leftPageLayout->addWidget(topTitleWidget);
    leftPageLayout->addWidget(scrollArea);

    page2RightLayout = new QVBoxLayout(page_2);
    setupKalmanFilterControls();

    calibrationPageLayout->addItem(leftPageLayout);
    calibrationPageLayout->addItem(page2RightLayout);
    // 更新校准页面布局
    page_2->setLayout(calibrationPageLayout);

}

void PaperFaceTrackerWindow::addCalibrationParam(QGridLayout* layout, QLabel* label, QLineEdit* offsetEdit, QScrollBar* bar, QProgressBar* value, int row){

    // 检查 layout 和 label 必须非空
    if (!layout || !label) {
        LOG_ERROR("布局或标签为空，无法添加校准参数行");
        return;
    }

    // 添加 label 到布局
    layout->addWidget(label, row, 0);

    // 如果 offsetEdit 不为空，设置属性并添加到布局
    if (offsetEdit) {
        offsetEdit->setFixedWidth(60);
        offsetEdit->setPlaceholderText("偏置值");
        layout->addWidget(offsetEdit, row, 1);
    } else {
        layout->addWidget(new QWidget(), row, 1); // 占位符
    }

    // 如果 bar 不为空，设置属性并添加到布局
    if (bar) {
        bar->setOrientation(Qt::Horizontal);
        bar->setFixedWidth(200);
        layout->addWidget(bar, row, 2);
    } else {
        layout->addWidget(new QWidget(), row, 2); // 占位符
    }

    // 如果 value 不为空，设置属性并添加到布局
    if (value) {
        value->setFixedWidth(150);
        value->setValue(24);
        layout->addWidget(value, row, 3);
    } else {
        layout->addWidget(new QWidget(), row, 3); // 占位符
    }
}

void PaperFaceTrackerWindow::setVideoImage(const cv::Mat& image)
{
    if (image.empty())
    {
        QMetaObject::invokeMethod(this, [this]()
        {
            if (stackedWidget->currentIndex() == 0) {
                ImageLabel->clear(); // 清除图片
                ImageLabel->setText(QApplication::translate("PaperTrackerMainWindow", "没有图像输入")); // 恢复默认文本
            } else if (stackedWidget->currentIndex() == 1) {
                ImageLabelCal->clear(); // 清除图片
                ImageLabelCal->setText(QApplication::translate("PaperTrackerMainWindow", "没有图像输入"));
            }
        }, Qt::QueuedConnection);
        return ;
    }
    QMetaObject::invokeMethod(this, [this, image = image.clone()]() {
        auto qimage = QImage(image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
        auto pix_map = QPixmap::fromImage(qimage);
        if (stackedWidget->currentIndex() == 0)
        {
            ImageLabel->setPixmap(pix_map);
            ImageLabel->setScaledContents(true);
            ImageLabel->update();
        } else if (stackedWidget->currentIndex() == 1) {
            ImageLabelCal->setPixmap(pix_map);
            ImageLabelCal->setScaledContents(true);
            ImageLabelCal->update();
        }
    }, Qt::QueuedConnection);
}

PaperFaceTrackerWindow::~PaperFaceTrackerWindow() {
    stop();
    if (auto_save_timer) {
        auto_save_timer->stop();
        delete auto_save_timer;
        auto_save_timer = nullptr;
    }
    config = generate_config();
    config_writer->write_config(config);
    LOG_INFO("正在关闭VRCFT");
    remove_log_window(LogText);
    instance = nullptr;

}

void PaperFaceTrackerWindow::bound_pages() {
    // 页面导航逻辑
    connect(MainPageButton, &QPushButton::clicked, [this] {
        stackedWidget->setCurrentIndex(0);
    });
    connect(CalibrationPageButton, &QPushButton::clicked, [this] {
        stackedWidget->setCurrentIndex(1);
    });
}

// 添加事件过滤器实现
bool PaperFaceTrackerWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 处理焦点获取事件
    if (event->type() == QEvent::FocusIn) {
        if (obj == SSIDText) {
            if (SSIDText->toPlainText() == "请输入WIFI名字（仅支持2.4ghz）") {
                SSIDText->setPlainText("");
            }
        } else if (obj == PasswordText) {
            if (PasswordText->toPlainText() == "请输入WIFI密码") {
                PasswordText->setPlainText("");
            }
        }
    }

    // 处理焦点失去事件
    if (event->type() == QEvent::FocusOut) {
        if (obj == SSIDText) {
            if (SSIDText->toPlainText().isEmpty()) {
                SSIDText->setPlainText(QApplication::translate("PaperTrackerMainWindow", "请输入WIFI名字（仅支持2.4ghz）"));
            }
        } else if (obj == PasswordText) {
            if (PasswordText->toPlainText().isEmpty()) {
                PasswordText->setPlainText(QApplication::translate("PaperTrackerMainWindow", "请输入WIFI密码"));
            }
        }
    }

    // 继续事件处理
    return QWidget::eventFilter(obj, event);
}

// 根据模型输出更新校准页面的进度条
void PaperFaceTrackerWindow::updateCalibrationProgressBars(
    const std::vector<float>& output,
    const std::unordered_map<std::string, size_t>& blendShapeIndexMap
) {
    if (output.empty() || stackedWidget->currentIndex() != 1) {
        // 如果输出为空或者当前不在校准页面，则不更新
        return;
    }

    // 使用Qt的线程安全方式更新UI
    QMetaObject::invokeMethod(this, [this, output, &blendShapeIndexMap]() {
        // 将值缩放到0-100范围内用于进度条显示
        auto scaleValue = [](const float value) -> int {
            // 将值限制在0-1.0范围内，然后映射到0-100
            return static_cast<int>(value * 100);
        };

        // 更新各个进度条
        // 注意：这里假设输出数组中的索引与ARKit模型输出的顺序一致

        // 脸颊
        if (blendShapeIndexMap.contains("cheekPuffLeft") && blendShapeIndexMap.at("cheekPuffLeft") < output.size()) {
            CheekPullLeftValue->setValue(scaleValue(output[blendShapeIndexMap.at("cheekPuffLeft")]));
        }
        if (blendShapeIndexMap.contains("cheekPuffRight") && blendShapeIndexMap.at("cheekPuffRight") < output.size()) {
            CheekPullRightValue->setValue(scaleValue(output[blendShapeIndexMap.at("cheekPuffRight")]));
        }
        // 下巴
        if (blendShapeIndexMap.contains("jawOpen") && blendShapeIndexMap.at("jawOpen") < output.size()) {
            JawOpenValue->setValue(scaleValue(output[blendShapeIndexMap.at("jawOpen")]));
        }
        if (blendShapeIndexMap.contains("jawLeft") && blendShapeIndexMap.at("jawLeft") < output.size()) {
            JawLeftValue->setValue(scaleValue(output[blendShapeIndexMap.at("jawLeft")]));
        }
        if (blendShapeIndexMap.contains("jawRight") && blendShapeIndexMap.at("jawRight") < output.size()) {
            JawRightValue->setValue(scaleValue(output[blendShapeIndexMap.at("jawRight")]));
        }
        // 嘴巴
        if (blendShapeIndexMap.contains("mouthLeft") && blendShapeIndexMap.at("mouthLeft") < output.size()) {
            MouthLeftValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthLeft")]));
        }
        if (blendShapeIndexMap.contains("mouthRight") && blendShapeIndexMap.at("mouthRight") < output.size()) {
            MouthRightValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthRight")]));
        }
        // 舌头
        if (blendShapeIndexMap.contains("tongueOut") && blendShapeIndexMap.at("tongueOut") < output.size()) {
            TongueOutValue->setValue(scaleValue(output[blendShapeIndexMap.at("tongueOut")]));
        }
        if (blendShapeIndexMap.contains("tongueUp") && blendShapeIndexMap.at("tongueUp") < output.size()) {
            TongueUpValue->setValue(scaleValue(output[blendShapeIndexMap.at("tongueUp")]));
        }
        if (blendShapeIndexMap.contains("tongueDown") && blendShapeIndexMap.at("tongueDown") < output.size()) {
            TongueDownValue->setValue(scaleValue(output[blendShapeIndexMap.at("tongueDown")]));
        }
        if (blendShapeIndexMap.contains("tongueLeft") && blendShapeIndexMap.at("tongueLeft") < output.size()) {
            TongueLeftValue->setValue(scaleValue(output[blendShapeIndexMap.at("tongueLeft")]));
        }
        if (blendShapeIndexMap.contains("tongueRight") && blendShapeIndexMap.at("tongueRight") < output.size()) {
            TongueRightValue->setValue(scaleValue(output[blendShapeIndexMap.at("tongueRight")]));
        }
        if (blendShapeIndexMap.contains("mouthClose") && blendShapeIndexMap.at("mouthClose") < output.size()) {
            MouthCloseValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthClose")]));
        }
        if (blendShapeIndexMap.contains("mouthFunnel") && blendShapeIndexMap.at("mouthFunnel") < output.size()) {
            MouthFunnelValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthFunnel")]));
        }
        if (blendShapeIndexMap.contains("mouthPucker") && blendShapeIndexMap.at("mouthPucker") < output.size()) {
            MouthPuckerValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthPucker")]));
        }
        if (blendShapeIndexMap.contains("mouthRollUpper") && blendShapeIndexMap.at("mouthRollUpper") < output.size()) {
            MouthRollUpperValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthRollUpper")]));
        }
        if (blendShapeIndexMap.contains("mouthRollLower") && blendShapeIndexMap.at("mouthRollLower") < output.size()) {
            MouthRollLowerValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthRollLower")]));
        }
        if (blendShapeIndexMap.contains("mouthShrugUpper") && blendShapeIndexMap.at("mouthShrugUpper") < output.size()) {
            MouthShrugUpperValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthShrugUpper")]));
        }
        if (blendShapeIndexMap.contains("mouthShrugLower") && blendShapeIndexMap.at("mouthShrugLower") < output.size()) {
            MouthShrugLowerValue->setValue(scaleValue(output[blendShapeIndexMap.at("mouthShrugLower")]));
        }
    }, Qt::QueuedConnection);
}

void PaperFaceTrackerWindow::connect_callbacks()
{
    brightness_timer = std::make_shared<QTimer>();
    brightness_timer->setSingleShot(true);
    connect(brightness_timer.get(), &QTimer::timeout, this, &PaperFaceTrackerWindow::onSendBrightnessValue);
    // functions
    connect(BrightnessBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onBrightnessChanged);
    connect(RotateImageBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onRotateAngleChanged);
    connect(restart_Button, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onRestartButtonClicked);
    connect(FlashFirmwareButton, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onFlashButtonClicked);
    connect(UseFilterBox, &QCheckBox::checkStateChanged, this, &PaperFaceTrackerWindow::onUseFilterClicked);
    connect(wifi_send_Button, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onSendButtonClicked);
    connect(EnergyModeBox, &QComboBox::currentIndexChanged, this, &PaperFaceTrackerWindow::onEnergyModeChanged);
    connect(JawOpenBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onJawOpenChanged);
    connect(JawLeftBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onJawLeftChanged);
    connect(JawRightBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onJawRightChanged);
    connect(MouthLeftBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthLeftChanged);
    connect(MouthRightBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthRightChanged);
    connect(TongueOutBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onTongueOutChanged);
    connect(TongueLeftBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onTongueLeftChanged);
    connect(TongueRightBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onTongueRightChanged);
    connect(TongueUpBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onTongueUpChanged);
    connect(TongueDownBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onTongueDownChanged);
    connect(CheekPuffLeftBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onCheekPuffLeftChanged);
    connect(CheekPuffRightBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onCheekPuffRightChanged);
    connect(ShowSerialDataButton, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onShowSerialDataButtonClicked);
    connect(CheekPuffLeftOffset, &QLineEdit::editingFinished,
                this, &PaperFaceTrackerWindow::onCheekPuffLeftOffsetChanged);
    connect(CheekPuffRightOffset, &QLineEdit::editingFinished,
            this, &PaperFaceTrackerWindow::onCheekPuffRightOffsetChanged);
    connect(JawOpenOffset, &QLineEdit::editingFinished,
            this, &PaperFaceTrackerWindow::onJawOpenOffsetChanged);
    connect(TongueOutOffset, &QLineEdit::editingFinished,
            this, &PaperFaceTrackerWindow::onTongueOutOffsetChanged);
    // 在connect_callbacks()函数中现有连接代码之后添加以下内容
    connect(MouthCloseBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthCloseChanged);
    connect(MouthFunnelBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthFunnelChanged);
    connect(MouthPuckerBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthPuckerChanged);
    connect(MouthRollUpperBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthRollUpperChanged);
    connect(MouthRollLowerBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthRollLowerChanged);
    connect(MouthShrugUpperBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthShrugUpperChanged);
    connect(MouthShrugLowerBar, &QScrollBar::valueChanged, this, &PaperFaceTrackerWindow::onMouthShrugLowerChanged);

    connect(MouthCloseOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthCloseOffsetChanged);
    connect(MouthFunnelOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthFunnelOffsetChanged);
    connect(MouthPuckerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthPuckerOffsetChanged);
    connect(MouthRollUpperOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollUpperOffsetChanged);
    connect(MouthRollLowerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollLowerOffsetChanged);
    connect(MouthShrugUpperOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugUpperOffsetChanged);
    connect(MouthShrugLowerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugLowerOffsetChanged);
}

float PaperFaceTrackerWindow::getRotateAngle() const
{
    auto rotate_angle = static_cast<float>(current_rotate_angle);
    rotate_angle = rotate_angle / (static_cast<float>(RotateImageBar->maximum()) -
        static_cast<float>(RotateImageBar->minimum())) * 360.0f;
    return rotate_angle;
}


void PaperFaceTrackerWindow::setOnUseFilterClickedFunc(FuncWithVal func)
{
    onUseFilterClickedFunc = std::move(func);
}

void PaperFaceTrackerWindow::setSerialStatusLabel(const QString& text) const
{
    SerialConnectLabel->setText(QApplication::translate("PaperTrackerMainWindow", text.toUtf8().constData()));
}

void PaperFaceTrackerWindow::setWifiStatusLabel(const QString& text) const
{
    WifiConnectLabel->setText(QApplication::translate("PaperTrackerMainWindow", text.toUtf8().constData()));
}

void PaperFaceTrackerWindow::setIPText(const QString& text) const
{
    textEdit->setText(tr(text.toUtf8().constData()));
}

QPlainTextEdit* PaperFaceTrackerWindow::getLogText() const
{
    return LogText;
}

Rect PaperFaceTrackerWindow::getRoiRect()
{
    return roi_rect;
}

std::string PaperFaceTrackerWindow::getSSID() const
{
    return SSIDText->toPlainText().toStdString();
}

std::string PaperFaceTrackerWindow::getPassword() const
{
    return PasswordText->toPlainText().toStdString();
}

void PaperFaceTrackerWindow::onSendButtonClicked()
{
    // onSendButtonClickedFunc();
    // 获取SSID和密码
    auto ssid = getSSID();
    auto password = getPassword();
    // 输入验证
    if (ssid == QApplication::translate("PaperTrackerMainWindow", "请输入WIFI名字（仅支持2.4ghz）").toStdString() || ssid.empty()) {
        QMessageBox::warning(this, QApplication::translate("PaperTrackerMainWindow", "输入错误"), QApplication::translate("PaperTrackerMainWindow", "请输入有效的WIFI名字"));
        return;
    }

    if (password == QApplication::translate("PaperTrackerMainWindow", "请输入WIFI密码").toStdString() || password.empty()) {
        QMessageBox::warning(this, QApplication::translate("PaperTrackerMainWindow", "输入错误"), QApplication::translate("PaperTrackerMainWindow", "请输入有效的密码"));
        return;
    }

    // 构建并发送数据包
    LOG_INFO("已发送WiFi配置: SSID = {}, PWD = {}", ssid, password);
    LOG_INFO("等待数据被发送后开始自动重启ESP32...");
    serial_port_manager->sendWiFiConfig(ssid, password);

    QTimer::singleShot(3000, this, [this] {
        // 3秒后自动重启ESP32
        onRestartButtonClicked();
    });
}

void PaperFaceTrackerWindow::onRestartButtonClicked()
{
    serial_port_manager->stop_heartbeat_timer();
    image_downloader->stop_heartbeat_timer();
    serial_port_manager->restartESP32(this);
    serial_port_manager->start_heartbeat_timer();
    image_downloader->stop();
    image_downloader->start();
    image_downloader->start_heartbeat_timer();
}

void PaperFaceTrackerWindow::onUseFilterClicked(int value) const
{
    QTimer::singleShot(10, this, [this, value] {
        inference->set_use_filter(value);
    });
}

void PaperFaceTrackerWindow::onFlashButtonClicked()
{
    // 弹出固件选择对话框
    QStringList firmwareTypes;
    firmwareTypes << "普通版面捕固件 (face_tracker.bin)"
                  << "旧版面捕固件 (old_face_tracker.bin)"
                  << "轻薄板面捕固件 (light_face_tracker.bin)";

    bool ok;
    QString selectedType = QInputDialog::getItem(this, "选择固件类型",
                                                "请选择要烧录的固件类型:",
                                                firmwareTypes, 0, false, &ok);
    if (!ok || selectedType.isEmpty()) {
        // 用户取消操作
        return;
    }

    // 根据选择设置固件类型
    std::string firmwareType;
    if (selectedType.contains("普通版面捕固件")) {
        firmwareType = "face_tracker";
    } else if (selectedType.contains("旧版面捕固件")) {
        firmwareType = "old_face_tracker";
    } else if (selectedType.contains("轻薄板面捕固件")) {
        firmwareType = "light_face_tracker";
    }

    LOG_INFO("用户选择烧录固件类型: {}", firmwareType);

    serial_port_manager->stop_heartbeat_timer();
    image_downloader->stop_heartbeat_timer();
    serial_port_manager->flashESP32(this, firmwareType);
    serial_port_manager->start_heartbeat_timer();
    image_downloader->stop();
    image_downloader->start();
    image_downloader->start_heartbeat_timer();
}

void PaperFaceTrackerWindow::onBrightnessChanged(int value) {
    // 更新当前亮度值
    current_brightness = value;

    // 更新值后可以安全返回，即使系统未准备好也先保存用户设置
    if (!serial_port_manager || !brightness_timer) {
        return;
    }

    // 只在设备正确连接状态下才发送命令
    if (serial_port_manager->status() == SerialStatus::OPENED) {
        brightness_timer->start(100);
    } else {
        QMessageBox::warning(this, "警告", "面捕设备未连接，请先连接设备");
    }
}
void PaperFaceTrackerWindow::onRotateAngleChanged(int value)
{
    current_rotate_angle = value;
}

void PaperFaceTrackerWindow::onSendBrightnessValue() const
{
    // 发送亮度控制命令 - 确保亮度值为三位数字
    std::string brightness_str = std::to_string(current_brightness);
    // 补齐三位数字，前面加0
    while (brightness_str.length() < 3) {
        brightness_str = std::string("0") + brightness_str;
    }
    std::string packet = "A6" + brightness_str + "B6";
    serial_port_manager->write_data(packet);
    // 记录操作
    LOG_INFO("已设置亮度: {}", current_brightness);
}

bool PaperFaceTrackerWindow:: is_running() const
{
    return app_is_running;
}

void PaperFaceTrackerWindow::stop()
{
    LOG_INFO("正在关闭系统...");
    app_is_running = false;
    if (update_thread.joinable())
    {
        update_thread.join();
    }
    if (inference_thread.joinable())
    {
        inference_thread.join();
    }
    if (osc_send_thread.joinable())
    {
        osc_send_thread.join();
    }
    if (brightness_timer) {
        brightness_timer->stop();
        brightness_timer.reset();
    }
    serial_port_manager->stop();
    image_downloader->stop();
    inference.reset();
    osc_manager->close();
    // 其他清理工作
    LOG_INFO("系统已安全关闭");
}

void PaperFaceTrackerWindow::onEnergyModeChanged(int index)
{
    if (index == 0)
    {
        max_fps = 38;
    } else if (index == 1)
    {
        max_fps = 15;
    } else if (index == 2)
    {
        max_fps = 70;
    }
}

int PaperFaceTrackerWindow::get_max_fps() const
{
    return max_fps;
}

PaperFaceTrackerConfig PaperFaceTrackerWindow::generate_config() const
{
    PaperFaceTrackerConfig res_config;
    res_config.brightness = current_brightness;
    res_config.rotate_angle = current_rotate_angle;
    res_config.energy_mode = EnergyModeBox->currentIndex();
    res_config.use_filter = UseFilterBox->isChecked();
    res_config.wifi_ip = textEdit->toPlainText().toStdString();
    res_config.amp_map = {
        {"cheekPuffLeft", CheekPuffLeftBar->value()},
        {"cheekPuffRight", CheekPuffRightBar->value()},
        {"jawOpen", JawOpenBar->value()},
        {"jawLeft", JawLeftBar->value()},
        {"jawRight", JawRightBar->value()},
        {"mouthLeft", MouthLeftBar->value()},
        {"mouthRight", MouthRightBar->value()},
        {"tongueOut", TongueOutBar->value()},
        {"tongueUp", TongueUpBar->value()},
        {"tongueDown", TongueDownBar->value()},
        {"tongueLeft", TongueLeftBar->value()},
        {"tongueRight", TongueRightBar->value()},
    };
    // 添加偏置值
    res_config.cheek_puff_left_offset = cheek_puff_left_offset;
    res_config.cheek_puff_right_offset = cheek_puff_right_offset;
    res_config.jaw_open_offset = jaw_open_offset;
    res_config.tongue_out_offset = tongue_out_offset;
    res_config.rect = roi_rect;

    // 添加卡尔曼滤波参数
    res_config.dt = current_dt;
    res_config.q_factor = current_q_factor;
    res_config.r_factor = current_r_factor;
    res_config.mouth_close_offset = mouth_close_offset;
    res_config.mouth_funnel_offset = mouth_funnel_offset;
    res_config.mouth_pucker_offset = mouth_pucker_offset;
    res_config.mouth_roll_upper_offset = mouth_roll_upper_offset;
    res_config.mouth_roll_lower_offset = mouth_roll_lower_offset;
    res_config.mouth_shrug_upper_offset = mouth_shrug_upper_offset;
    res_config.mouth_shrug_lower_offset = mouth_shrug_lower_offset;
    return res_config;
}

void PaperFaceTrackerWindow::set_config()
{
    config = config_writer->get_config<PaperFaceTrackerConfig>();
    current_brightness = config.brightness;
    current_rotate_angle = config.rotate_angle == 0 ? 540 : config.rotate_angle;
    BrightnessBar->setValue(config.brightness);
    RotateImageBar->setValue(config.rotate_angle);
    EnergyModeBox->setCurrentIndex(config.energy_mode);
    UseFilterBox->setChecked(config.use_filter);
    textEdit->setPlainText(QString::fromStdString(config.wifi_ip));

    // 设置偏置值
    cheek_puff_left_offset = config.cheek_puff_left_offset;
    cheek_puff_right_offset = config.cheek_puff_right_offset;
    jaw_open_offset = config.jaw_open_offset;
    tongue_out_offset = config.tongue_out_offset;

    // 设置新的偏置值（安全方式，避免使用未初始化的值）
    mouth_close_offset = 0.0f;  // 默认值
    mouth_funnel_offset = 0.0f;
    mouth_pucker_offset = 0.0f;
    mouth_roll_upper_offset = 0.0f;
    mouth_roll_lower_offset = 0.0f;
    mouth_shrug_upper_offset = 0.0f;
    mouth_shrug_lower_offset = 0.0f;
    // 加载卡尔曼滤波参数 - 这里可能缺少了这部分
    current_dt = config.dt;
    current_q_factor = config.q_factor;
    current_r_factor = config.r_factor;

    // 更新UI显示
    if (dtLineEdit) dtLineEdit->setText(QString::number(current_dt, 'f', 3));
    if (qFactorLineEdit) qFactorLineEdit->setText(QString::number(current_q_factor, 'f', 2));
    if (rFactorLineEdit) rFactorLineEdit->setText(QString::number(current_r_factor, 'f', 6));

    // 设置输入框的文本
    CheekPuffLeftOffset->setText(QString::number(cheek_puff_left_offset));
    CheekPuffRightOffset->setText(QString::number(cheek_puff_right_offset));
    JawOpenOffset->setText(QString::number(jaw_open_offset));
    TongueOutOffset->setText(QString::number(tongue_out_offset));

    // 设置新增的输入框文本
    MouthCloseOffset->setText(QString::number(mouth_close_offset));
    MouthFunnelOffset->setText(QString::number(mouth_funnel_offset));
    MouthPuckerOffset->setText(QString::number(mouth_pucker_offset));
    MouthRollUpperOffset->setText(QString::number(mouth_roll_upper_offset));
    MouthRollLowerOffset->setText(QString::number(mouth_roll_lower_offset));
    MouthShrugUpperOffset->setText(QString::number(mouth_shrug_upper_offset));
    MouthShrugLowerOffset->setText(QString::number(mouth_shrug_lower_offset));

    // 更新偏置值到推理引擎
    updateOffsetsToInference();

    try
    {
        CheekPuffLeftBar->setValue(config.amp_map.at("cheekPuffLeft"));
        CheekPuffRightBar->setValue(config.amp_map.at("cheekPuffRight"));
        JawOpenBar->setValue(config.amp_map.at("jawOpen"));
        JawLeftBar->setValue(config.amp_map.at("jawLeft"));
        JawRightBar->setValue(config.amp_map.at("jawRight"));
        MouthLeftBar->setValue(config.amp_map.at("mouthLeft"));
        MouthRightBar->setValue(config.amp_map.at("mouthRight"));
        TongueOutBar->setValue(config.amp_map.at("tongueOut"));
        TongueUpBar->setValue(config.amp_map.at("tongueUp"));
        TongueDownBar->setValue(config.amp_map.at("tongueDown"));
        TongueLeftBar->setValue(config.amp_map.at("tongueLeft"));
        TongueRightBar->setValue(config.amp_map.at("tongueRight"));

        // 新增的滑动条设置（先检查是否存在对应的键，不存在则使用默认值）
        MouthCloseBar->setValue(config.amp_map.count("mouthClose") > 0 ? config.amp_map.at("mouthClose") : 0);
        MouthFunnelBar->setValue(config.amp_map.count("mouthFunnel") > 0 ? config.amp_map.at("mouthFunnel") : 0);
        MouthPuckerBar->setValue(config.amp_map.count("mouthPucker") > 0 ? config.amp_map.at("mouthPucker") : 0);
        MouthRollUpperBar->setValue(config.amp_map.count("mouthRollUpper") > 0 ? config.amp_map.at("mouthRollUpper") : 0);
        MouthRollLowerBar->setValue(config.amp_map.count("mouthRollLower") > 0 ? config.amp_map.at("mouthRollLower") : 0);
        MouthShrugUpperBar->setValue(config.amp_map.count("mouthShrugUpper") > 0 ? config.amp_map.at("mouthShrugUpper") : 0);
        MouthShrugLowerBar->setValue(config.amp_map.count("mouthShrugLower") > 0 ? config.amp_map.at("mouthShrugLower") : 0);
    }
    catch (std::exception& e)
    {
        LOG_ERROR("配置文件中的振幅映射错误: {}", e.what());
    }
    roi_rect = config.rect;
}

void PaperFaceTrackerWindow::onCheekPuffLeftChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onCheekPuffRightChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onJawOpenChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onJawLeftChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onJawRightChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthLeftChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthRightChanged(int value)
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueOutChanged(int value)
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueLeftChanged(int value)
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueRightChanged(int value)
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueUpChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueDownChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}
void PaperFaceTrackerWindow::onMouthCloseChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthFunnelChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthPuckerChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthRollUpperChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthRollLowerChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthShrugUpperChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthShrugLowerChanged(int value) const
{
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthCloseOffsetChanged()
{
    bool ok;
    float value = MouthCloseOffset->text().toFloat(&ok);
    if (ok) {
        mouth_close_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthFunnelOffsetChanged()
{
    bool ok;
    float value = MouthFunnelOffset->text().toFloat(&ok);
    if (ok) {
        mouth_funnel_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthPuckerOffsetChanged()
{
    bool ok;
    float value = MouthPuckerOffset->text().toFloat(&ok);
    if (ok) {
        mouth_pucker_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthRollUpperOffsetChanged()
{
    bool ok;
    float value = MouthRollUpperOffset->text().toFloat(&ok);
    if (ok) {
        mouth_roll_upper_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthRollLowerOffsetChanged()
{
    bool ok;
    float value = MouthRollLowerOffset->text().toFloat(&ok);
    if (ok) {
        mouth_roll_lower_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthShrugUpperOffsetChanged()
{
    bool ok;
    float value = MouthShrugUpperOffset->text().toFloat(&ok);
    if (ok) {
        mouth_shrug_upper_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthShrugLowerOffsetChanged()
{
    bool ok;
    float value = MouthShrugLowerOffset->text().toFloat(&ok);
    if (ok) {
        mouth_shrug_lower_offset = value;
        updateOffsetsToInference();
    }
}
std::unordered_map<std::string, int> PaperFaceTrackerWindow::getAmpMap() const
{
    return {
            {"cheekPuffLeft", CheekPuffLeftBar->value()},
            {"cheekPuffRight", CheekPuffRightBar->value()},
            {"jawOpen", JawOpenBar->value()},
            {"jawLeft", JawLeftBar->value()},
            {"jawRight", JawRightBar->value()},
            {"mouthLeft", MouthLeftBar->value()},
            {"mouthRight", MouthRightBar->value()},
            {"tongueOut", TongueOutBar->value()},
            {"tongueUp", TongueUpBar->value()},
            {"tongueDown", TongueDownBar->value()},
            {"tongueLeft", TongueLeftBar->value()},
            {"tongueRight", TongueRightBar->value()},
            // 添加新的形态键
            {"mouthClose", MouthCloseBar->value()},
            {"mouthFunnel", MouthFunnelBar->value()},
            {"mouthPucker", MouthPuckerBar->value()},
            {"mouthRollUpper", MouthRollUpperBar->value()},
            {"mouthRollLower", MouthRollLowerBar->value()},
            {"mouthShrugUpper", MouthShrugUpperBar->value()},
            {"mouthShrugLower", MouthShrugLowerBar->value()},
        };
}

void PaperFaceTrackerWindow::start_image_download() const
{
    if (image_downloader->isStreaming())
    {
        image_downloader->stop();
    }
    // 开始下载图片 - 修改为支持WebSocket协议
    // 检查URL格式
    const std::string& url = current_ip_;
    if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://" ||
        url.substr(0, 5) == "ws://" || url.substr(0, 6) == "wss://") {
        // URL已经包含协议前缀，直接使用
        image_downloader->init(url, DEVICE_TYPE_FACE);
    } else {
        // 添加默认ws://前缀
        image_downloader->init("ws://" + url, DEVICE_TYPE_FACE);
    }
    image_downloader->start();
}

void PaperFaceTrackerWindow::updateWifiLabel() const
{
    if (image_downloader->isStreaming())
    {
        setWifiStatusLabel("Wifi已连接");
    } else
    {
        setWifiStatusLabel("Wifi连接失败");
    }
}

void PaperFaceTrackerWindow::updateSerialLabel() const
{
    if (serial_port_manager->status() == SerialStatus::OPENED)
    {
        setSerialStatusLabel("面捕有线模式已连接");
    } else
    {
        setSerialStatusLabel("面捕有线模式连接失败");
    }
}

cv::Mat PaperFaceTrackerWindow::getVideoImage() const
{
    return std::move(image_downloader->getLatestFrame());
}

std::string PaperFaceTrackerWindow::getFirmwareVersion() const
{
    return firmware_version;
}

SerialStatus PaperFaceTrackerWindow::getSerialStatus() const
{
    return serial_port_manager->status();
}

void PaperFaceTrackerWindow::set_osc_send_thead(FuncWithoutArgs func)
{
    osc_send_thread = std::thread(std::move(func));
}
void PaperFaceTrackerWindow::updateBatteryStatus() const
{
    if (image_downloader && image_downloader->isStreaming())
    {
        float battery = image_downloader->getBatteryPercentage();
        QString batteryText = QString(QApplication::translate("PaperTrackerMainWindow", "电池电量: %1%").arg(battery, 0, 'f', 1));
        BatteryStatusLabel->setText(batteryText);
    }
    else
    {
        BatteryStatusLabel->setText(QApplication::translate("PaperTrackerMainWindow", "电池电量: 未知"));
    }
}
void PaperFaceTrackerWindow::create_sub_threads()
{
    update_thread = std::thread([this]()
    {
        auto last_time = std::chrono::high_resolution_clock::now();
        double fps_total = 0;
        double fps_count = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while (is_running())
        {
            updateWifiLabel();
            updateSerialLabel();
            updateBatteryStatus();
            auto start_time = std::chrono::high_resolution_clock::now();
            try {
                if (fps_total > 1000)
                {
                    fps_count = 0;
                    fps_total = 0;
                }
                // caculate fps
                auto start = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(start - last_time);
                last_time = start;
                auto fps = 1000.0 / static_cast<double>(duration.count());
                fps_total += fps;
                fps_count += 1;
                fps = fps_total/fps_count;
                cv::Mat frame = getVideoImage();
                if (!frame.empty())
                {
                    auto rotate_angle = getRotateAngle();
                    cv::resize(frame, frame, cv::Size(280, 280), cv::INTER_NEAREST);
                    int y = frame.rows / 2;
                    int x = frame.cols / 2;
                    auto rotate_matrix = cv::getRotationMatrix2D(cv::Point(x, y), rotate_angle, 1);
                    cv::warpAffine(frame, frame, rotate_matrix, frame.size(), cv::INTER_NEAREST);

                    auto roi_rect = getRoiRect();
                    // 显示图像
                    cv::rectangle(frame, roi_rect.rect, cv::Scalar(0, 255, 0), 2);
                }
                // draw rect on frame
                cv::Mat show_image;
                if (!frame.empty())
                {
                    show_image = frame;
                }
                setVideoImage(show_image);
                // 控制帧率
            } catch (const std::exception& e) {
                // 使用Qt方式记录日志，而不是minilog
                QMetaObject::invokeMethod(this, [&e]() {
                    LOG_ERROR("错误, 视频处理异常: {}", e.what());
                }, Qt::QueuedConnection);
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count ();
            int delay_ms = max(0, static_cast<int>(1000.0 / min(get_max_fps() + 30, 50) - elapsed));
            // LOG_DEBUG("UIFPS:" +  std::to_string(min(get_max_fps() + 30, 60)));
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    });

    inference_thread = std::thread([this] ()
    {
        auto last_time = std::chrono::high_resolution_clock::now();
        double fps_total = 0;
        double fps_count = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while (is_running())
        {
            if (fps_total > 1000)
            {
                fps_count = 0;
                fps_total = 0;
            }
            // calculate fps
            auto start = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(start - last_time);
            last_time = start;
            auto fps = 1000.0 / static_cast<double>(duration.count());
            fps_total += fps;
            fps_count += 1;
            fps = fps_total/fps_count;
            // LOG_DEBUG("模型FPS： {}", fps);

            auto start_time = std::chrono::high_resolution_clock::now();
            // 设置时间序列
            inference->set_dt(duration.count() / 1000.0);

            auto frame = getVideoImage();
            // 推理处理
            if (!frame.empty())
            {
                auto rotate_angle = getRotateAngle();
                cv::resize(frame, frame, cv::Size(280, 280), cv::INTER_NEAREST);
                int y = frame.rows / 2;
                int x = frame.cols / 2;
                auto rotate_matrix = cv::getRotationMatrix2D(cv::Point(x, y), rotate_angle, 1);
                cv::warpAffine(frame, frame, rotate_matrix, frame.size(), cv::INTER_NEAREST);
                cv::Mat infer_frame;
                infer_frame = frame.clone();
                auto roi_rect = getRoiRect();
                if (!roi_rect.rect.empty() && roi_rect.is_roi_end)
                {
                    infer_frame = infer_frame(roi_rect.rect);
                }
                inference->inference(infer_frame);
                {
                    std::lock_guard<std::mutex> lock(outputs_mutex);
                    outputs = inference->get_output();
                }
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            int delay_ms = max(0, static_cast<int>(1000.0 / get_max_fps() - elapsed));
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    });

    osc_send_thread = std::thread([this] ()
    {
        auto last_time = std::chrono::high_resolution_clock::now();
        double fps_total = 0;
        double fps_count = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while (is_running())
        {
         if (fps_total > 1000)
         {
             fps_count = 0;
             fps_total = 0;
         }
         // calculate fps
         auto start = std::chrono::high_resolution_clock::now();
         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(start - last_time);
         last_time = start;
         auto fps = 1000.0 / static_cast<double>(duration.count());
         fps_total += fps;
         fps_count += 1;
         fps = fps_total/fps_count;
         auto start_time = std::chrono::high_resolution_clock::now();
        // 发送OSC数据
         {
             std::lock_guard<std::mutex> lock(outputs_mutex);
             if (!outputs.empty()) {
                updateCalibrationProgressBars(outputs, inference->getBlendShapeIndexMap());
                osc_manager->sendModelOutput(outputs, blend_shapes);
             }
         }

         auto end_time = std::chrono::high_resolution_clock::now();
         auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
         int delay_ms = max(0, static_cast<int>(1000.0 / 66.0 - elapsed));
         std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    });
}
void PaperFaceTrackerWindow::onCheekPuffLeftOffsetChanged()
{
    bool ok;
    float value = CheekPuffLeftOffset->text().toFloat(&ok);
    if (ok) {
        cheek_puff_left_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onCheekPuffRightOffsetChanged()
{
    bool ok;
    float value = CheekPuffRightOffset->text().toFloat(&ok);
    if (ok) {
        cheek_puff_right_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onJawOpenOffsetChanged()
{
    bool ok;
    float value = JawOpenOffset->text().toFloat(&ok);
    if (ok) {
        jaw_open_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onTongueOutOffsetChanged()
{
    bool ok;
    float value = TongueOutOffset->text().toFloat(&ok);
    if (ok) {
        tongue_out_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::updateOffsetsToInference()
{
    // 创建一个包含偏置值的映射
    std::unordered_map<std::string, float> offset_map;
    offset_map["cheekPuffLeft"] = cheek_puff_left_offset;
    offset_map["cheekPuffRight"] = cheek_puff_right_offset;
    offset_map["jawOpen"] = jaw_open_offset;
    offset_map["tongueOut"] = tongue_out_offset;
    // 添加新的偏置值
    offset_map["mouthClose"] = mouth_close_offset;
    offset_map["mouthFunnel"] = mouth_funnel_offset;
    offset_map["mouthPucker"] = mouth_pucker_offset;
    offset_map["mouthRollUpper"] = mouth_roll_upper_offset;
    offset_map["mouthRollLower"] = mouth_roll_lower_offset;
    offset_map["mouthShrugUpper"] = mouth_shrug_upper_offset;
    offset_map["mouthShrugLower"] = mouth_shrug_lower_offset;

    // 调用inference的设置偏置值方法
    inference->set_offset_map(offset_map);

    // 添加下面这行代码，同时更新振幅映射
    inference->set_amp_map(getAmpMap());

    // 记录日志，便于调试
    //LOG_INFO("偏置值已更新到推理引擎");
}
void PaperFaceTrackerWindow::onShowSerialDataButtonClicked()
{
    showSerialData = !showSerialData;
    if (showSerialData) {
        LOG_INFO("已开启串口原始数据显示");
        ShowSerialDataButton->setText(QApplication::translate("PaperTrackerMainWindow", "停止显示串口数据"));
    } else {
        LOG_INFO("已关闭串口原始数据显示");
        ShowSerialDataButton->setText(QApplication::translate("PaperTrackerMainWindow", "显示串口数据"));
    }
}

void PaperFaceTrackerWindow::setupKalmanFilterControls() {
    // 卡尔曼滤波参数区域（校准页面）
    auto* dtLayout = new QHBoxLayout();
    dtLabel = new QLabel("时间步长(dt):", page_2);
    dtLineEdit = new QLineEdit(QString::number(current_dt, 'f', 3), page_2);
    dtLineEdit->setMinimumWidth(70);
    dtLayout->addWidget(dtLabel);
    dtLayout->addWidget(dtLineEdit);
    dtLayout->addStretch();

    auto* qFactorLayout = new QHBoxLayout();
    qFactorLabel = new QLabel("过程噪声系数(q):", page_2);
    qFactorLineEdit = new QLineEdit(QString::number(current_q_factor, 'f', 2), page_2);
    qFactorLineEdit->setMinimumWidth(70);
    qFactorLayout->addWidget(qFactorLabel);
    qFactorLayout->addWidget(qFactorLineEdit);
    qFactorLayout->addStretch();

    auto* rFactorLayout = new QHBoxLayout();
    rFactorLabel = new QLabel("测量噪声系数(r):", page_2);
    rFactorLineEdit = new QLineEdit(QString::number(current_r_factor, 'f', 6), page_2);
    rFactorLineEdit->setMinimumWidth(70);
    rFactorLayout->addWidget(rFactorLabel);
    rFactorLayout->addWidget(rFactorLineEdit);
    rFactorLayout->addStretch();

    // 说明标签
    helpLabel = new QLabel(page_2);
    auto str1 = QApplication::translate("PaperTrackerMainWindow", "调整建议:");
    auto str2 = QApplication::translate("PaperTrackerMainWindow", "增大q值, 减小r值: 更灵敏, 抖动更明显");
    auto str3 = QApplication::translate("PaperTrackerMainWindow", "减小q值, 增大r值: 更平滑, 反应更滞后");
    helpLabel->setText(str1 + "\n" + str2 + "\n" + str3);
    helpLabel->setWordWrap(true);

    // 创建 QGridLayout

    auto* gridLayout = new QGridLayout();
    gridLayout->setSpacing(12); // 减小全局间距
    gridLayout->setContentsMargins(0, 0, 0, 0);

    // 添加控件
    gridLayout->addWidget(dtLabel, 0, 0);
    gridLayout->addWidget(dtLineEdit, 0, 1);

    gridLayout->addWidget(qFactorLabel, 1, 0);
    gridLayout->addWidget(qFactorLineEdit, 1, 1);

    gridLayout->addWidget(rFactorLabel, 2, 0);
    gridLayout->addWidget(rFactorLineEdit, 2, 1);

    // 添加说明标签
    helpLabel->setWordWrap(true);
    helpLabel->setContentsMargins(0, 0, 0, 0); // 移除边距
    helpLabel->setMargin(0);                   // 移除内部边距


    // 父布局调整
    if (page2RightLayout) {
        page2RightLayout->setSpacing(12); // 减小垂直间距
        page2RightLayout->addLayout(gridLayout);
        page2RightLayout->addWidget(helpLabel,0,Qt::AlignBottom);
        page2RightLayout->addWidget(ImageLabelCal,1, Qt::AlignBottom);
        ImageLabelCal->setFixedSize(280, 280);
        page2RightLayout->addStretch();
    }

}

void PaperFaceTrackerWindow::onDtEditingFinished() {
    bool ok;
    float value = dtLineEdit->text().toFloat(&ok);
    if (ok && value > 0) {
        current_dt = value;
        if (inference) {
            inference->set_dt(current_dt);
            LOG_INFO("卡尔曼滤波参数已更新: dt = {}", current_dt);
        }
    } else {
        // 输入无效，恢复原值
        dtLineEdit->setText(QString::number(current_dt, 'f', 3));
    }
}

void PaperFaceTrackerWindow::onQFactorEditingFinished() {
    bool ok;
    float value = qFactorLineEdit->text().toFloat(&ok);
    if (ok && value > 0) {
        current_q_factor = value;
        if (inference) {
            inference->set_q_factor(current_q_factor);
            LOG_INFO("卡尔曼滤波参数已更新: q_factor = {}", current_q_factor);
        }
    } else {
        // 输入无效，恢复原值
        qFactorLineEdit->setText(QString::number(current_q_factor, 'f', 2));
    }
}

void PaperFaceTrackerWindow::onRFactorEditingFinished() {
    bool ok;
    float value = rFactorLineEdit->text().toFloat(&ok);
    if (ok && value > 0) {
        current_r_factor = value;
        if (inference) {
            inference->set_r_factor(current_r_factor);
            LOG_INFO("卡尔曼滤波参数已更新: r_factor = {}", current_r_factor);
        }
    } else {
        // 输入无效，恢复原值
        rFactorLineEdit->setText(QString::number(current_r_factor, 'f', 6));
    }
}

void PaperFaceTrackerWindow::retranslateUI()
{
    if (is_running()) {
        updateWifiLabel();
        updateSerialLabel();
        updateBatteryStatus();
    }
    // 更新主页面按钮文本
    MainPageButton->setText(QApplication::translate("PaperTrackerMainWindow", "主页"));
    // 更新校准页面按钮文本
    CalibrationPageButton->setText(QApplication::translate("PaperTrackerMainWindow", "标定页面"));

    if (!ImageLabel->text().isEmpty()) {
        ImageLabel->setText(QApplication::translate("PaperTrackerMainWindow", "没有图像输入"));
    }

    restart_Button->setText(QApplication::translate("PaperTrackerMainWindow", "重启"));
    FlashFirmwareButton->setText(QApplication::translate("PaperTrackerMainWindow", "刷写固件"));
    label->setText(QApplication::translate("PaperTrackerMainWindow", "亮度调整"));
    label_2->setText(QApplication::translate("PaperTrackerMainWindow", "日志窗口："));
    label_16->setText(QApplication::translate("PaperTrackerMainWindow", "IP地址："));
    label_17->setText(QApplication::translate("PaperTrackerMainWindow", "旋转角度调整"));
    label_18->setText(QApplication::translate("PaperTrackerMainWindow", "性能模式选择"));
    ShowSerialDataButton->setText(QApplication::translate("PaperTrackerMainWindow", "串口日志"));
    SSIDText->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "请输入WIFI名字（仅支持2.4ghz）"));
    PasswordText->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "请输入WIFI密码"));
    wifi_send_Button->setText(QApplication::translate("PaperTrackerMainWindow", "发送"));
    UseFilterBox->setText(QApplication::translate("PaperTrackerMainWindow", "启用滤波（减少抖动）"));
    EnergyModeBox->setItemText(0, QApplication::translate("PaperTrackerMainWindow", "普通模式"));
    EnergyModeBox->setItemText(1, QApplication::translate("PaperTrackerMainWindow", "节能模式"));
    EnergyModeBox->setItemText(2, QApplication::translate("PaperTrackerMainWindow", "性能模式"));

    label_11->setText(QApplication::translate("PaperTrackerMainWindow", "嘴右移"));
    label_10->setText(QApplication::translate("PaperTrackerMainWindow", "嘴左移"));
    label_9->setText(QApplication::translate("PaperTrackerMainWindow", "下巴右移"));
    label_8->setText(QApplication::translate("PaperTrackerMainWindow", "下巴左移"));
    label_7 ->setText(QApplication::translate("PaperTrackerMainWindow", "下巴下移"));
    label_6->setText(QApplication::translate("PaperTrackerMainWindow", "右脸颊"));
    label_5->setText(QApplication::translate("PaperTrackerMainWindow", "左脸颊"));
    label_14->setText(QApplication::translate("PaperTrackerMainWindow", "舌头向下"));
    label_15->setText(QApplication::translate("PaperTrackerMainWindow", "舌头向左"));
    label_12->setText(QApplication::translate("PaperTrackerMainWindow", "舌头伸出"));
    label_13->setText(QApplication::translate("PaperTrackerMainWindow", "舌头向上"));
    label_31->setText(QApplication::translate("PaperTrackerMainWindow", "舌头向右"));

    label_mouthClose->setText(QApplication::translate("PaperTrackerMainWindow", "嘴闭合"));
    label_mouthFunnel->setText(QApplication::translate("PaperTrackerMainWindow", "嘴漏斗形"));
    label_mouthPucker->setText(QApplication::translate("PaperTrackerMainWindow", "嘴撅起"));
    label_mouthRollUpper->setText(QApplication::translate("PaperTrackerMainWindow", "上唇内卷"));
    label_mouthRollLower->setText(QApplication::translate("PaperTrackerMainWindow", "下唇内卷"));
    label_mouthShrugUpper->setText(QApplication::translate("PaperTrackerMainWindow", "上唇耸起"));
    label_mouthShrugLower->setText(QApplication::translate("PaperTrackerMainWindow", "下唇耸起"));

    dtLabel->setText(QApplication::translate("PaperTrackerMainWindow", "时间步长(dt):"));
    qFactorLabel->setText(QApplication::translate("PaperTrackerMainWindow", "过程噪声系数(q):"));
    rFactorLabel->setText(QApplication::translate("PaperTrackerMainWindow", "测量噪声系数(r):"));

    label_3->setText(QApplication::translate("PaperTrackerMainWindow", "放大倍率"));
    label_20->setText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    CheekPuffLeftOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    CheekPuffRightOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    JawOpenOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    TongueOutOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    MouthCloseOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    MouthFunnelOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    MouthPuckerOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    MouthRollUpperOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    MouthRollLowerOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    MouthShrugUpperOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));
    MouthShrugLowerOffset->setPlaceholderText(QApplication::translate("PaperTrackerMainWindow", "偏置值"));

    tutorialLink->setText(QString("<a href='https://fcnk6r4c64fa.feishu.cn/wiki/VSlnw4Zr0i4VzXFkvT8TcbQFMn7c' style='color: #0066cc; font-size: 14pt; font-weight: bold;'>%1</a>")
                          .arg(QApplication::translate("PaperTrackerMainWindow", "面捕调整教程")));

    auto str1 = QApplication::translate("PaperTrackerMainWindow", "调整建议:");
    auto str2 = QApplication::translate("PaperTrackerMainWindow", "增大q值, 减小r值: 更灵敏, 抖动更明显");
    auto str3 = QApplication::translate("PaperTrackerMainWindow", "减小q值, 增大r值: 更平滑, 反应更滞后");
    helpLabel->setText(str1 + "\n" + str2 + "\n" + str3);
}
