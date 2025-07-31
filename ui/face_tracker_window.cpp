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

#include "opencv2/imgcodecs.hpp"

static bool is_show_tip = false;

PaperFaceTrackerWindow::PaperFaceTrackerWindow(QWidget *parent)
    : QWidget(parent)
{
    if (instance == nullptr)
        instance = this;
    else
        throw std::exception(Translator::tr("当前已经打开了面捕窗口，请不要重复打开").toUtf8().constData());
    is_show_tip = false;
    // 基本UI设置
    setFixedSize(900, 388);
    initUi();
    initializeParameters();
    initLayout();
    // 初始化校准超时计时器
    calibrationTimer = new QTimer(this);
    connect(calibrationTimer, &QTimer::timeout, this, &PaperFaceTrackerWindow::onCalibrationTimeout);

    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    LogText->setMaximumBlockCount(200);
    append_log_window(LogText);
    LOG_INFO("系统初始化中...");
    // 初始化串口连接状态
    SerialConnectLabel->setText(Translator::tr("有线模式未连接"));
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
        if (x + width > 350) {
            width = 350 - x;
        }
        if (y + height > 259) {
            height = 259 - y;
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
                QString version_str = version == 2 ? Translator::tr("左眼追") : Translator::tr("右眼追");
                if (!version_warning)
                {
                    QMessageBox msgBox;
                    msgBox.setWindowIcon(this->windowIcon());
                    msgBox.setText(Translator::tr("检测到") + version_str + Translator::tr("设备，请打开眼追界面进行设置"));
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
            msgBox.setText(Translator::tr("未找到配置文件信息，请将面捕通过数据线连接到电脑进行首次配置"));
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

void PaperFaceTrackerWindow::initUi() {
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
    LogText->setVisible(false);
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
    RotateImageBar->setRange(0, 100);
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
    ShowSerialDataButton->setFixedHeight(24);

    // 创建滚动区域并设置内容容器
    scrollArea = new QScrollArea(page_2);
    scrollArea->setMinimumWidth(500);
    scrollArea->setObjectName("scrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding); // 允许扩展
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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
    JawLeftOffset = new QLineEdit(scrollAreaWidgetContents);
    JawRightOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthLeftOffset = new QLineEdit(scrollAreaWidgetContents);
    MouthRightOffset = new QLineEdit(scrollAreaWidgetContents);
    TongueLeftOffset = new QLineEdit(scrollAreaWidgetContents);
    TongueRightOffset = new QLineEdit(scrollAreaWidgetContents);
    TongueUpOffset = new QLineEdit(scrollAreaWidgetContents);
    TongueDownOffset = new QLineEdit(scrollAreaWidgetContents);

    // 滑动条控件
    CheekPuffLeftBar = new QLineEdit(scrollAreaWidgetContents);
    CheekPuffRightBar = new QLineEdit(scrollAreaWidgetContents);
    JawOpenBar = new QLineEdit(scrollAreaWidgetContents);
    JawLeftBar = new QLineEdit(scrollAreaWidgetContents);
    MouthLeftBar = new QLineEdit(scrollAreaWidgetContents);
    JawRightBar = new QLineEdit(scrollAreaWidgetContents);
    TongueOutBar = new QLineEdit(scrollAreaWidgetContents);
    MouthRightBar = new QLineEdit(scrollAreaWidgetContents);
    TongueDownBar = new QLineEdit(scrollAreaWidgetContents);
    TongueUpBar = new QLineEdit(scrollAreaWidgetContents);
    TongueRightBar = new QLineEdit(scrollAreaWidgetContents);
    TongueLeftBar = new QLineEdit(scrollAreaWidgetContents);
    MouthCloseBar = new QLineEdit(scrollAreaWidgetContents);
    MouthFunnelBar = new QLineEdit(scrollAreaWidgetContents);
    MouthPuckerBar = new QLineEdit(scrollAreaWidgetContents);
    MouthRollUpperBar = new QLineEdit(scrollAreaWidgetContents);
    MouthRollLowerBar = new QLineEdit(scrollAreaWidgetContents);
    MouthShrugUpperBar = new QLineEdit(scrollAreaWidgetContents);
    MouthShrugLowerBar = new QLineEdit(scrollAreaWidgetContents);

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
    label_20 = new QLabel(page_2);
    lockLabel = new QLabel(page_2);
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
    tutorialLink->setText(QString("<a href='https://fcnk6r4c64fa.feishu.cn/wiki/VSlnw4Zr0iVzXFkvT8TcbQFMn7c' style='color: #0066cc; font-size: 14pt; font-weight: bold;'>%1</a>")
                          .arg(Translator::tr("面捕调整教程")));
    tutorialLink->setOpenExternalLinks(true);
    tutorialLink->setTextFormat(Qt::RichText);
    tutorialLink->setStyleSheet("background-color: #f0f0f0; padding: 5px; border-radius: 5px;");

    calibrationModeLabel = new QLabel(page_2);
    calibrationModeLabel->setText(Translator::tr("校准模式") + ":");
    calibrationModeLabel->setFixedHeight(24);
    calibrationModeComboBox = new QComboBox(page_2);
    calibrationModeComboBox->setFixedHeight(24);
    calibrationModeComboBox->addItem(Translator::tr("自然模式"));
    calibrationModeComboBox->addItem(Translator::tr("完整模式"));
    calibrationModeComboBox->setCurrentIndex(0);
    calibrationStartButton = new QPushButton(Translator::tr("开始校准"), page_2);
    calibrationStartButton->setFixedHeight(24);
    calibrationStopButton = new QPushButton(Translator::tr("停止校准"), page_2);
    calibrationStopButton->setFixedHeight(24);
    calibrationStopButton->setEnabled(false);
    calibrationResetButton = new QPushButton(Translator::tr("重置"), page_2);
    calibrationResetButton->setFixedHeight(24);
    calibrationResetButton->setEnabled(true);

    // 为每个参数添加启用控制 ComboBox
    CheekPuffLeftEnable = new QCheckBox(scrollAreaWidgetContents);
    CheekPuffRightEnable = new QCheckBox(scrollAreaWidgetContents);
    JawOpenEnable = new QCheckBox(scrollAreaWidgetContents);
    TongueOutEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthCloseEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthFunnelEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthPuckerEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthRollUpperEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthRollLowerEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthShrugUpperEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthShrugLowerEnable = new QCheckBox(scrollAreaWidgetContents);
    JawLeftEnable = new QCheckBox(scrollAreaWidgetContents);
    JawRightEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthLeftEnable = new QCheckBox(scrollAreaWidgetContents);
    MouthRightEnable = new QCheckBox(scrollAreaWidgetContents);
    TongueLeftEnable = new QCheckBox(scrollAreaWidgetContents);
    TongueRightEnable = new QCheckBox(scrollAreaWidgetContents);
    TongueUpEnable = new QCheckBox(scrollAreaWidgetContents);
    TongueDownEnable = new QCheckBox(scrollAreaWidgetContents);

}
void PaperFaceTrackerWindow::initializeParameters() {
    parameterMap = {
        {"cheekPuffLeft", ParameterInfo(&cheek_puff_left_offset, &cheek_puff_left_amp,
                                       CheekPuffLeftOffset, CheekPuffLeftBar,
                                       CheekPullLeftValue, label_5,
                                       CheekPuffLeftEnable)},
        {"cheekPuffRight", ParameterInfo(&cheek_puff_right_offset, &cheek_puff_right_amp,
                                        CheekPuffRightOffset, CheekPuffRightBar,
                                        CheekPullRightValue, label_6,
                                        CheekPuffRightEnable)},
        {"jawOpen", ParameterInfo(&jaw_open_offset, &jaw_open_amp,
                                 JawOpenOffset, JawOpenBar,
                                 JawOpenValue, label_7,
                                 JawOpenEnable)},
        {"jawLeft", ParameterInfo(&jaw_left_offset, &jaw_left_amp,
                                 JawLeftOffset, JawLeftBar,
                                 JawLeftValue, label_8,
                                 JawLeftEnable)},
        {"jawRight", ParameterInfo(&jaw_right_offset, &jaw_right_amp,
                                  JawRightOffset, JawRightBar,
                                  JawRightValue, label_9,
                                  JawRightEnable)},
        {"mouthLeft", ParameterInfo(&mouth_left_offset, &mouth_left_amp,
                                   MouthLeftOffset, MouthLeftBar,
                                   MouthLeftValue, label_10,
                                   MouthLeftEnable)},
        {"mouthRight", ParameterInfo(&mouth_right_offset, &mouth_right_amp,
                                    MouthRightOffset, MouthRightBar,
                                    MouthRightValue, label_11,
                                    MouthRightEnable)},
        {"tongueOut", ParameterInfo(&tongue_out_offset, &tongue_out_amp,
                                   TongueOutOffset, TongueOutBar,
                                   TongueOutValue, label_12,
                                   TongueOutEnable)},
        {"tongueUp", ParameterInfo(&tongue_up_offset, &tongue_up_amp,
                                  TongueUpOffset, TongueUpBar,
                                  TongueUpValue, label_13,
                                  TongueUpEnable)},
        {"tongueDown", ParameterInfo(&tongue_down_offset, &tongue_down_amp,
                                    TongueDownOffset, TongueDownBar,
                                    TongueDownValue, label_14,
                                    TongueDownEnable)},
        {"tongueLeft", ParameterInfo(&tongue_left_offset, &tongue_left_amp,
                                    TongueLeftOffset, TongueLeftBar,
                                    TongueLeftValue, label_15,
                                    TongueLeftEnable)},
        {"tongueRight", ParameterInfo(&tongue_right_offset, &tongue_right_amp,
                                     TongueRightOffset, TongueRightBar,
                                     TongueRightValue, label_31,
                                     TongueRightEnable)},
        {"mouthClose", ParameterInfo(&mouth_close_offset, &mouth_close_amp,
                                    MouthCloseOffset, MouthCloseBar,
                                    MouthCloseValue, label_mouthClose,
                                    MouthCloseEnable)},
        {"mouthFunnel", ParameterInfo(&mouth_funnel_offset, &mouth_funnel_amp,
                                     MouthFunnelOffset, MouthFunnelBar,
                                     MouthFunnelValue, label_mouthFunnel,
                                     MouthFunnelEnable)},
        {"mouthPucker", ParameterInfo(&mouth_pucker_offset, &mouth_pucker_amp,
                                     MouthPuckerOffset, MouthPuckerBar,
                                     MouthPuckerValue, label_mouthPucker,
                                     MouthPuckerEnable)},
        {"mouthRollUpper", ParameterInfo(&mouth_roll_upper_offset, &mouth_roll_upper_amp,
                                        MouthRollUpperOffset, MouthRollUpperBar,
                                        MouthRollUpperValue, label_mouthRollUpper,
                                        MouthRollUpperEnable)},
        {"mouthRollLower", ParameterInfo(&mouth_roll_lower_offset, &mouth_roll_lower_amp,
                                        MouthRollLowerOffset, MouthRollLowerBar,
                                        MouthRollLowerValue, label_mouthRollLower,
                                        MouthRollLowerEnable)},
        {"mouthShrugUpper", ParameterInfo(&mouth_shrug_upper_offset, &mouth_shrug_upper_amp,
                                         MouthShrugUpperOffset, MouthShrugUpperBar,
                                         MouthShrugUpperValue, label_mouthShrugUpper,
                                         MouthShrugUpperEnable)},
        {"mouthShrugLower", ParameterInfo(&mouth_shrug_lower_offset, &mouth_shrug_lower_amp,
                                         MouthShrugLowerOffset, MouthShrugLowerBar,
                                         MouthShrugLowerValue, label_mouthShrugLower,
                                         MouthShrugLowerEnable)}
    };

    // 定义显示顺序
    parameterOrder = {
        "cheekPuffLeft",
        "cheekPuffRight",
        "jawOpen",
        "jawLeft",
        "jawRight",
        "mouthLeft",
        "mouthRight",
        "tongueOut",
        "tongueUp",
        "tongueDown",
        "tongueLeft",
        "tongueRight",
        "mouthClose",
        "mouthFunnel",
        "mouthPucker",
        "mouthRollUpper",
        "mouthRollLower",
        "mouthShrugUpper",
        "mouthShrugLower"
    };
}


void PaperFaceTrackerWindow::initLayout() {
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
    ImageLabel->setFixedSize(350, 259);
    ImageLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ImageLabel->setStyleSheet("border: 1px solid white;");
    ImageLabel->setAlignment(Qt::AlignCenter);
    QFont font = ImageLabel->font();
    font.setPointSize(14); // 设置字体大小为14
    font.setBold(true);    // 设置粗体
    ImageLabel->setFont(font);
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
    RotateImageBar->setMinimumWidth(340);
    rotateLayout->addStretch();
    controlLayout->addLayout(rotateLayout);

    // 模式选择
    auto* modeLayout = new QHBoxLayout();
    modeLayout->setSpacing(8);
    EnergyModeBox->addItem(Translator::tr("普通模式"));
    EnergyModeBox->addItem(Translator::tr("节能模式"));
    EnergyModeBox->addItem(Translator::tr("性能模式"));
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
    imageAndParamsLayout->addSpacing(8);
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
    calibrationPageLayout->setSpacing(0);

    auto* leftPageLayout = new QVBoxLayout(page_2);
    leftPageLayout->setContentsMargins(10, 0, 10, 10);
    leftPageLayout->setSpacing(0);

    // 创建参数布局并应用到内容容器
    auto* paramsLayout = new QGridLayout(scrollAreaWidgetContents);
    paramsLayout->setSpacing(4);
    paramsLayout->setContentsMargins(10, 10, 10, 10);

    int row = 0;
    for (const auto& paramName : parameterOrder) {
        if (parameterMap.find(paramName) != parameterMap.end()) {
            addCalibrationParam(paramsLayout, paramName, parameterMap[paramName], row++);
        }
    }
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
    topTitleLayout->setSpacing(0);
    topTitleLayout->addSpacing(10);
    topTitleLayout->addWidget(lockLabel);
    topTitleLayout->addSpacing(110);
    topTitleLayout->addWidget(label_20);
    topTitleLayout->addSpacing(60);
    topTitleLayout->addWidget(label_3);

    // 添加校准开始、校准结束按钮
    auto calibrationWidget = new QWidget(page_2);
    auto calibrationLayout = new QHBoxLayout(calibrationWidget);
    calibrationLayout->setSpacing(0);
    calibrationLayout->addWidget(calibrationModeLabel);
    calibrationLayout->addSpacing(4);
    calibrationLayout->addWidget(calibrationModeComboBox);
    calibrationLayout->addSpacing(12);
    calibrationLayout->addWidget(calibrationStartButton);
    calibrationLayout->addSpacing(8);
    calibrationLayout->addWidget(calibrationStopButton);
    calibrationLayout->addSpacing(8);
    calibrationLayout->addWidget(calibrationResetButton);
    calibrationLayout->addStretch();

    // 将滚动区域添加到校准页面布局
    leftPageLayout->addWidget(calibrationWidget);
    leftPageLayout->addSpacing(4);
    leftPageLayout->addWidget(topTitleWidget);
    leftPageLayout->addWidget(scrollArea);

    page2RightLayout = new QVBoxLayout(page_2);
    setupKalmanFilterControls();

    calibrationPageLayout->addItem(leftPageLayout);
    calibrationPageLayout->addItem(page2RightLayout);
    // 更新校准页面布局
    page_2->setLayout(calibrationPageLayout);

}

void PaperFaceTrackerWindow::onCalibrationStartClicked()
{
    // 清除之前的校准数据
    calibration_data.clear();
    calibration_offsets.clear();
    calibration_amp_ratios.clear();
    calibration_sample_count = 0;

    disconnectOffsetChangeEvent();

    auto index = calibrationModeComboBox->currentIndex();
    clearCalibrationParameters(index == 0, index == 1);

    // 设置校准状态
    is_calibrating = true;

    // 禁用滚动区域中的所有控件
    if (scrollAreaWidgetContents) {
        scrollAreaWidgetContents->setEnabled(false);
    }

    // 禁用校准模式选择
    if (calibrationModeComboBox) {
        calibrationModeComboBox->setEnabled(false);
    }

    // 禁用开始校准按钮，启用停止校准按钮
    calibrationStartButton->setEnabled(false);
    calibrationStopButton->setEnabled(true);
    calibrationResetButton->setEnabled(false);

    // 启动60秒超时计时器
    calibrationTimer->start(60000); // 60秒 = 60000毫秒

    LOG_INFO("校准开始，参数设置区域已禁用");
}

// 添加校准超时处理函数
void PaperFaceTrackerWindow::onCalibrationTimeout()
{
    // 停止校准
    onCalibrationStopClicked();

    LOG_INFO("校准超时(60秒)，自动停止校准");
}

void PaperFaceTrackerWindow::onCalibrationStopClicked() {
    // 设置校准状态
    is_calibrating = false;

    bool isNaturalMode = (calibrationModeComboBox && calibrationModeComboBox->currentIndex() == 0);

    if (isNaturalMode) {
        // 计算偏置值
        calculateCalibrationOffsets();
        // 应用偏置值到推理引擎
        applyCalibrationOffsets();
    }
    else {
        // 计算缩放倍率
        calculateCalibrationAMP();
        // 应用缩放倍率
        applyCalibrationAMP();
    }

    // 重新启用滚动区域中的所有控件
    if (scrollAreaWidgetContents) {
        scrollAreaWidgetContents->setEnabled(true);
    }

    // 重新启用校准模式选择
    if (calibrationModeComboBox) {
        calibrationModeComboBox->setEnabled(true);
    }

    // 启用开始校准按钮，禁用停止校准按钮
    calibrationStartButton->setEnabled(true);
    calibrationStopButton->setEnabled(false);
    calibrationResetButton->setEnabled(true);

    LOG_INFO("校准结束，参数设置区域已启用");
}

void PaperFaceTrackerWindow::addCalibrationParam(QGridLayout* layout,
                                                const QString& name,
                                                const ParameterInfo& param,
                                                int row) {
    if (!layout || !param.label) {
        LOG_ERROR("布局或标签为空，无法添加校准参数行");
        return;
    }

    // 添加启用控制 ComboBox 到布局（在 label 之前）
    if (param.enablCheckBox) {
        param.enablCheckBox->setFixedSize(28, 28);
        param.enablCheckBox->setStyleSheet(
         "QCheckBox::indicator {"
            "    width: 20px;"
            "    height: 20px;"
            "    border: 2px solid #04b97f;"
            "    border-radius: 4px;"
            "}"
        );
        layout->addWidget(param.enablCheckBox, row, 0);
    } else {
        layout->addWidget(new QWidget(), row, 0); // 占位符
    }

    // 添加 label 到布局
    layout->addWidget(param.label, row, 1);

    // 添加偏置值输入框
    if (param.offsetEdit) {
        param.offsetEdit->setFixedWidth(100);
        param.offsetEdit->setPlaceholderText(Translator::tr("偏置值"));
        layout->addWidget(param.offsetEdit, row, 2);
    } else {
        layout->addWidget(new QWidget(), row, 2); // 占位符
    }

    // 添加放大倍数输入框
    if (param.ampEdit) {
        param.ampEdit->setFixedWidth(100);
        param.ampEdit->setText("0");
        layout->addWidget(param.ampEdit, row, 3);
    } else {
        layout->addWidget(new QWidget(), row, 3); // 占位符
    }

    // 添加进度条
    if (param.progressBar) {
        param.progressBar->setFixedWidth(150);
        param.progressBar->setValue(24);
        layout->addWidget(param.progressBar, row, 4);
    } else {
        layout->addWidget(new QWidget(), row, 4); // 占位符
    }
}

void PaperFaceTrackerWindow::checkHardwareVersion()
{
    if (image_downloader && image_downloader->isStreaming())
    {
        auto deviceVersion = image_downloader->getHardwareVersion();
        if (deviceVersion!= 0 && deviceVersion != LATEST_HARDWARE_VERSION && !is_show_tip)
        {
            is_show_tip = true;
            QTimer::singleShot(200, this, [this, deviceVersion] {
                // 弹出提示框提示用户重新烧录固件
                QMessageBox msgBox;
                msgBox.setWindowIcon(this->windowIcon());
                msgBox.setWindowTitle(Translator::tr("固件版本不匹配"));
                msgBox.setText(Translator::tr("检测到设备固件版本与软件要求不匹配，需要重新烧录固件。"));
                msgBox.setInformativeText(Translator::tr("请使用数据线进行有线连接并在途中不要断开连接，然后点击“烧录固件”按钮进行固件烧录。"));
                msgBox.setIcon(QMessageBox::Critical);

                QPushButton *flashButton = msgBox.addButton(Translator::tr("烧录固件"), QMessageBox::AcceptRole);
                QPushButton *exitButton = msgBox.addButton(Translator::tr("退出程序"), QMessageBox::RejectRole);

                msgBox.setModal(true);
                msgBox.exec();

                if (msgBox.clickedButton() == flashButton) {
                   // 用户选择烧录固件，调用烧录函数
                   onFlashButtonClicked();
               } else if (msgBox.clickedButton() == exitButton) {
                   // 用户选择关闭窗口，只关闭当前窗口而不是退出整个程序
                   this->close();
               } else {
                   // 如果用户以其他方式关闭对话框，则也只关闭当前窗口
                   this->close();
               }
            });
        }
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
                ImageLabel->setText(Translator::tr("没有图像输入")); // 恢复默认文本
            } else if (stackedWidget->currentIndex() == 1) {
                ImageLabelCal->clear(); // 清除图片
                ImageLabelCal->setText(Translator::tr("没有图像输入"));
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
        auto oldIndex = stackedWidget->currentIndex();
        stackedWidget->setCurrentIndex(0);
        if (oldIndex == 1) {
            updatePageWidth();
        }
    });
    connect(CalibrationPageButton, &QPushButton::clicked, [this] {
        auto oldIndex = stackedWidget->currentIndex();
        stackedWidget->setCurrentIndex(1);
        if (oldIndex == 0) {
            updatePageWidth();
        }
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
                SSIDText->setPlainText(Translator::tr("请输入WIFI名字（仅支持2.4ghz）"));
            }
        } else if (obj == PasswordText) {
            if (PasswordText->toPlainText().isEmpty()) {
                PasswordText->setPlainText(Translator::tr("请输入WIFI密码"));
            }
        }
    }

    // 继续事件处理
    return QWidget::eventFilter(obj, event);
}

// 根据模型输出更新校准页面的进度条
void PaperFaceTrackerWindow::updateCalibrationProgressBars(
    const std::vector<float>& output,
    const std::unordered_map<std::string, size_t>& blendShapeIndexMap) {
    if (output.empty() || stackedWidget->currentIndex() != 1) {
        return;
    }

    QMetaObject::invokeMethod(this, [this, output, &blendShapeIndexMap]() {
        auto scaleValue = [](const float value) -> int {
            return static_cast<int>(std::clamp(value, 0.0f, 1.0f) * 100);
        };

        for (const auto& pair : parameterMap) {
            const QString& paramName = pair.first;
            const ParameterInfo& paramInfo = pair.second;

            std::string paramNameStd = paramName.toStdString();
            if (blendShapeIndexMap.contains(paramNameStd) &&
                blendShapeIndexMap.at(paramNameStd) < output.size()) {
                float value = output[blendShapeIndexMap.at(paramNameStd)];
                if (paramInfo.progressBar) {
                    paramInfo.progressBar->setValue(scaleValue(value));
                }
            }
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

    connect(calibrationStartButton, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onCalibrationStartClicked);
    connect(calibrationStopButton, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onCalibrationStopClicked);

    connect(calibrationResetButton, &QPushButton::clicked, this, [this]() {
        clearCalibrationParameters();
    });
    connectOffsetChangeEvent();
}

void PaperFaceTrackerWindow::connectOffsetChangeEvent() {
    // 重新连接偏置值输入框的信号连接
    connect(MouthCloseOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthCloseOffsetChanged);
    connect(MouthFunnelOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthFunnelOffsetChanged);
    connect(MouthPuckerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthPuckerOffsetChanged);
    connect(MouthRollUpperOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollUpperOffsetChanged);
    connect(MouthRollLowerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollLowerOffsetChanged);
    connect(MouthShrugUpperOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugUpperOffsetChanged);
    connect(MouthShrugLowerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugLowerOffsetChanged);
    connect(CheekPuffLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffLeftOffsetChanged);
    connect(CheekPuffRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffRightOffsetChanged);
    connect(JawOpenOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawOpenOffsetChanged);
    connect(TongueOutOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueOutOffsetChanged);
    connect(JawLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawLeftOffsetChanged);
    connect(JawRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawRightOffsetChanged);
    connect(MouthLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthLeftOffsetChanged);
    connect(MouthRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRightOffsetChanged);
    connect(TongueUpOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueUpOffsetChanged);
    connect(TongueDownOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueDownOffsetChanged);
    connect(TongueLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueLeftOffsetChanged);
    connect(TongueRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueRightOffsetChanged);

    connect(JawOpenBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawOpenChanged);
    connect(JawLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawLeftChanged);
    connect(JawRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawRightChanged);
    connect(MouthLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthLeftChanged);
    connect(MouthRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRightChanged);
    connect(TongueOutBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueOutChanged);
    connect(TongueLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueLeftChanged);
    connect(TongueRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueRightChanged);
    connect(TongueUpBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueUpChanged);
    connect(TongueDownBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueDownChanged);
    connect(CheekPuffLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffLeftChanged);
    connect(CheekPuffRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffRightChanged);
    connect(ShowSerialDataButton, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onShowSerialDataButtonClicked);
    connect(MouthCloseBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthCloseChanged);
    connect(MouthFunnelBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthFunnelChanged);
    connect(MouthPuckerBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthPuckerChanged);
    connect(MouthRollUpperBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollUpperChanged);
    connect(MouthRollLowerBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollLowerChanged);
    connect(MouthShrugUpperBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugUpperChanged);
    connect(MouthShrugLowerBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugLowerChanged);
}

void PaperFaceTrackerWindow::disconnectOffsetChangeEvent() {
    // 重新连接偏置值输入框的信号连接
    disconnect(MouthCloseOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthCloseOffsetChanged);
    disconnect(MouthFunnelOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthFunnelOffsetChanged);
    disconnect(MouthPuckerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthPuckerOffsetChanged);
    disconnect(MouthRollUpperOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollUpperOffsetChanged);
    disconnect(MouthRollLowerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollLowerOffsetChanged);
    disconnect(MouthShrugUpperOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugUpperOffsetChanged);
    disconnect(MouthShrugLowerOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugLowerOffsetChanged);
    disconnect(CheekPuffLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffLeftOffsetChanged);
    disconnect(CheekPuffRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffRightOffsetChanged);
    disconnect(JawOpenOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawOpenOffsetChanged);
    disconnect(TongueOutOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueOutOffsetChanged);
    disconnect(JawLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawLeftOffsetChanged);
    disconnect(JawRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawRightOffsetChanged);
    disconnect(MouthLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthLeftOffsetChanged);
    disconnect(MouthRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRightOffsetChanged);
    disconnect(TongueUpOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueUpOffsetChanged);
    disconnect(TongueDownOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueDownOffsetChanged);
    disconnect(TongueLeftOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueLeftOffsetChanged);
    disconnect(TongueRightOffset, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueRightOffsetChanged);

    disconnect(JawOpenBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawOpenChanged);
    disconnect(JawLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawLeftChanged);
    disconnect(JawRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onJawRightChanged);
    disconnect(MouthLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthLeftChanged);
    disconnect(MouthRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRightChanged);
    disconnect(TongueOutBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueOutChanged);
    disconnect(TongueLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueLeftChanged);
    disconnect(TongueRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueRightChanged);
    disconnect(TongueUpBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueUpChanged);
    disconnect(TongueDownBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onTongueDownChanged);
    disconnect(CheekPuffLeftBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffLeftChanged);
    disconnect(CheekPuffRightBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onCheekPuffRightChanged);
    disconnect(ShowSerialDataButton, &QPushButton::clicked, this, &PaperFaceTrackerWindow::onShowSerialDataButtonClicked);
    disconnect(MouthCloseBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthCloseChanged);
    disconnect(MouthFunnelBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthFunnelChanged);
    disconnect(MouthPuckerBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthPuckerChanged);
    disconnect(MouthRollUpperBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollUpperChanged);
    disconnect(MouthRollLowerBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthRollLowerChanged);
    disconnect(MouthShrugUpperBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugUpperChanged);
    disconnect(MouthShrugLowerBar, &QLineEdit::editingFinished, this, &PaperFaceTrackerWindow::onMouthShrugLowerChanged);
}

float PaperFaceTrackerWindow::getRotateAngle() const
{
    auto rotate_angle = static_cast<float>(current_rotate_angle);
    auto test = RotateImageBar->value();
    auto max =RotateImageBar->maximum();
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
    SerialConnectLabel->setText(Translator::tr(text.toUtf8().constData()));
}

void PaperFaceTrackerWindow::setWifiStatusLabel(const QString& text) const
{
    WifiConnectLabel->setText(Translator::tr(text.toUtf8().constData()));
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
    if (ssid == Translator::tr("请输入WIFI名字（仅支持2.4ghz）").toStdString() || ssid.empty()) {
        QMessageBox::warning(this, Translator::tr("输入错误"), Translator::tr("请输入有效的WIFI名字"));
        return;
    }

    if (password == Translator::tr("请输入WIFI密码").toStdString() || password.empty()) {
        QMessageBox::warning(this, Translator::tr("输入错误"), Translator::tr("请输入有效的密码"));
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
        {"cheekPuffLeft", cheek_puff_left_amp},
        {"cheekPuffRight", cheek_puff_right_amp},
        {"jawOpen", jaw_open_amp},
        {"jawLeft", jaw_left_amp},
        {"jawRight", jaw_right_amp},
        {"mouthLeft", mouth_left_amp},
        {"mouthRight", mouth_right_amp},
        {"tongueOut", tongue_out_amp},
        {"tongueUp", tongue_up_amp},
        {"tongueDown", tongue_down_amp},
        {"tongueLeft", tongue_left_amp},
        {"tongueRight", tongue_right_amp},
        {"mouthClose", mouth_close_amp},
        {"mouthFunnel", mouth_funnel_amp},
        {"mouthPucker", mouth_pucker_amp},
        {"mouthRollUpper", mouth_roll_upper_amp},
        {"mouthRollLower", mouth_roll_lower_amp},
        {"mouthShrugUpper", mouth_shrug_upper_amp},
        {"mouthShrugLower", mouth_shrug_lower_amp},
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
    res_config.mouth_left_offset = mouth_left_offset;
    res_config.mouth_right_offset = mouth_right_offset;
    res_config.jaw_left_offset = jaw_left_offset;
    res_config.jaw_right_offset = jaw_right_offset;
    res_config.tongue_left_offset = tongue_left_offset;
    res_config.tongue_right_offset = tongue_right_offset;
    res_config.tongue_up_offset = tongue_up_offset;
    res_config.tongue_down_offset = tongue_down_offset;
    return res_config;
}

void PaperFaceTrackerWindow::set_config() {
    config = config_writer->get_config<PaperFaceTrackerConfig>();

    // 基础配置设置
    current_brightness = config.brightness;
    current_rotate_angle = config.rotate_angle == 0 ? 50 : config.rotate_angle;
    BrightnessBar->setValue(config.brightness);
    RotateImageBar->setValue(current_rotate_angle);
    EnergyModeBox->setCurrentIndex(config.energy_mode);
    UseFilterBox->setChecked(config.use_filter);
    textEdit->setPlainText(QString::fromStdString(config.wifi_ip));

    disconnectOffsetChangeEvent();

    // 使用映射表设置偏置值
    for (auto& pair : parameterMap) {
        const QString& paramName = pair.first;
        ParameterInfo& paramInfo = pair.second;

        // 根据参数名称从配置中获取对应的偏置值
        float offsetValue = 0.0f;
        if (paramName == "cheekPuffLeft") {
            offsetValue = config.cheek_puff_left_offset;
            cheek_puff_left_offset = offsetValue;
        } else if (paramName == "cheekPuffRight") {
            offsetValue = config.cheek_puff_right_offset;
            cheek_puff_right_offset = offsetValue;
        } else if (paramName == "jawOpen") {
            offsetValue = config.jaw_open_offset;
            jaw_open_offset = offsetValue;
        } else if (paramName == "tongueOut") {
            offsetValue = config.tongue_out_offset;
            tongue_out_offset = offsetValue;
        } else if (paramName == "tongueLeft") {
            offsetValue = config.tongue_left_offset;
            tongue_left_offset = offsetValue;
        } else if (paramName == "tongueRight") {
            offsetValue = config.tongue_right_offset;
            tongue_right_offset = offsetValue;
        } else if (paramName == "tongueUp") {
            offsetValue = config.tongue_up_offset;
            tongue_up_offset = offsetValue;
        } else if (paramName == "tongueDown") {
            offsetValue = config.tongue_down_offset;
            tongue_down_offset = offsetValue;
        } else if (paramName == "jawLeft") {
            offsetValue = config.jaw_left_offset;
            jaw_left_offset = offsetValue;
        } else if (paramName == "jawRight") {
            offsetValue = config.jaw_right_offset;
            jaw_right_offset = offsetValue;
        } else if (paramName == "mouthLeft") {
            offsetValue = config.mouth_left_offset;
            mouth_left_offset = offsetValue;
        } else if (paramName == "mouthRight") {
            offsetValue = config.mouth_right_offset;
            mouth_right_offset = offsetValue;
        } else if (paramName == "mouthClose") {
            offsetValue = config.mouth_close_offset;
            mouth_close_offset = offsetValue;
        } else if (paramName == "mouthFunnel") {
            offsetValue = config.mouth_funnel_offset;
            mouth_funnel_offset = offsetValue;
        } else if (paramName == "mouthPucker") {
            offsetValue = config.mouth_pucker_offset;
            mouth_pucker_offset = offsetValue;
        } else if (paramName == "mouthRollLower") {
            offsetValue = config.mouth_roll_lower_offset;
            mouth_roll_lower_offset = offsetValue;
        } else if (paramName == "mouthRollUpper") {
            offsetValue = config.mouth_roll_upper_offset;
            mouth_roll_upper_offset = offsetValue;
        } else if (paramName == "mouthShrugUpper") {
            offsetValue = config.mouth_shrug_upper_offset;
            mouth_shrug_upper_offset = offsetValue;
        } else if (paramName == "mouthShrugLower") {
            offsetValue = config.mouth_shrug_lower_offset;
            mouth_shrug_lower_offset = offsetValue;
        }

        // 更新参数信息中的偏置值
        *(paramInfo.offsetValue) = offsetValue;

        // 更新UI控件
        if (paramInfo.offsetEdit) {
            paramInfo.offsetEdit->setText(QString::number(offsetValue));
        }
    }

    // 加载卡尔曼滤波参数
    current_dt = config.dt;
    current_q_factor = config.q_factor;
    current_r_factor = config.r_factor;

    // 更新UI显示
    if (dtLineEdit) dtLineEdit->setText(QString::number(current_dt, 'f', 3));
    if (qFactorLineEdit) qFactorLineEdit->setText(QString::number(current_q_factor, 'f', 2));
    if (rFactorLineEdit) rFactorLineEdit->setText(QString::number(current_r_factor, 'f', 6));

    // 设置输入框的文本 - 修改为只显示小数点后四位
    CheekPuffLeftOffset->setText(QString::number(cheek_puff_left_offset, 'f', 4));
    CheekPuffRightOffset->setText(QString::number(cheek_puff_right_offset, 'f', 4));
    JawOpenOffset->setText(QString::number(jaw_open_offset, 'f', 4));
    TongueOutOffset->setText(QString::number(tongue_out_offset, 'f', 4));
    MouthCloseOffset->setText(QString::number(mouth_close_offset, 'f', 4));
    MouthFunnelOffset->setText(QString::number(mouth_funnel_offset, 'f', 4));
    MouthPuckerOffset->setText(QString::number(mouth_pucker_offset, 'f', 4));
    MouthRollUpperOffset->setText(QString::number(mouth_roll_upper_offset, 'f', 4));
    MouthRollLowerOffset->setText(QString::number(mouth_roll_lower_offset, 'f', 4));
    MouthShrugUpperOffset->setText(QString::number(mouth_shrug_upper_offset, 'f', 4));
    MouthShrugLowerOffset->setText(QString::number(mouth_shrug_lower_offset, 'f', 4));
    JawLeftOffset->setText(QString::number(jaw_left_offset, 'f', 4));
    JawRightOffset->setText(QString::number(jaw_right_offset, 'f', 4));
    MouthLeftOffset->setText(QString::number(mouth_left_offset, 'f', 4));
    MouthRightOffset->setText(QString::number(mouth_right_offset, 'f', 4));
    TongueLeftOffset->setText(QString::number(tongue_left_offset, 'f', 4));
    TongueRightOffset->setText(QString::number(tongue_right_offset, 'f', 4));
    TongueUpOffset->setText(QString::number(tongue_up_offset, 'f', 4));
    TongueDownOffset->setText(QString::number(tongue_down_offset, 'f', 4));



    connectOffsetChangeEvent();

    // 设置振幅映射值
    setAmplitudeValuesFromConfig();

    // 更新偏置值到推理引擎,同时更新振幅值
    updateOffsetsToInference();

    roi_rect = config.rect;
}

void PaperFaceTrackerWindow::setAmplitudeValuesFromConfig() {
    try {
        for (auto& pair : parameterMap) {
            const QString& paramName = pair.first;
            ParameterInfo& paramInfo = pair.second;

            if (paramInfo.ampEdit) {
                std::string paramNameStd = paramName.toStdString();
                if (config.amp_map.count(paramNameStd) > 0) {
                    float ampValue = config.amp_map.at(paramNameStd);
                    paramInfo.ampEdit->setText(QString::number(ampValue, 'f', 4));  // 限制4位小数
                    *(paramInfo.ampValue) = ampValue;
                } else {
                    paramInfo.ampEdit->setText("0.0000");  // 保持格式一致
                    *(paramInfo.ampValue) = 0.0f;
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("配置文件中的振幅映射错误: {}", e.what());
    }
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

std::unordered_map<std::string, float> PaperFaceTrackerWindow::getAmpMap() const
{
    return {
            {"cheekPuffLeft", cheek_puff_left_amp},
            {"cheekPuffRight", cheek_puff_right_amp},
            {"jawOpen", jaw_open_amp},
            {"jawLeft", jaw_left_amp},
            {"jawRight", jaw_right_amp},
            {"mouthClose", mouth_close_amp},
            {"mouthFunnel", mouth_funnel_amp},
            {"mouthLeft", mouth_left_amp},
            {"mouthPucker", mouth_pucker_amp},
            {"mouthRight", mouth_right_amp},
            {"mouthShrugUpper", mouth_shrug_upper_amp},
            {"mouthShrugLower", mouth_shrug_lower_amp},
            {"mouthRollUpper", mouth_roll_upper_amp},
            {"mouthRollLower", mouth_roll_lower_amp},
            {"tongueOut", tongue_out_amp},
            {"tongueUp", tongue_up_amp},
            {"tongueDown", tongue_down_amp},
            {"tongueLeft", tongue_left_amp},
            {"tongueRight", tongue_right_amp}
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
        QString batteryText = QString(Translator::tr("电池电量: %1%").arg(battery, 0, 'f', 1));
        BatteryStatusLabel->setText(batteryText);
    }
    else
    {
        BatteryStatusLabel->setText(Translator::tr("电池电量: 未知"));
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
            checkHardwareVersion();
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
                    cv::resize(frame, frame, cv::Size(350, 259), cv::INTER_NEAREST);
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
                cv::resize(frame, frame, cv::Size(350, 259), cv::INTER_NEAREST);
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
                 // 如果正在校准，则收集数据
                 if (is_calibrating) {
                     collectData(outputs, inference->getBlendShapeIndexMap());
                 }
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

void PaperFaceTrackerWindow::onJawLeftOffsetChanged()
{
    bool ok;
    float value = JawLeftOffset->text().toFloat(&ok);
    if (ok) {
        jaw_left_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onJawRightOffsetChanged()
{
    bool ok;
    float value = JawRightOffset->text().toFloat(&ok);
    if (ok) {
        jaw_right_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthLeftOffsetChanged()
{
    bool ok;
    float value = MouthLeftOffset->text().toFloat(&ok);
    if (ok) {
        mouth_left_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onMouthRightOffsetChanged()
{
    bool ok;
    float value = MouthRightOffset->text().toFloat(&ok);
    if (ok) {
        mouth_right_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onTongueUpOffsetChanged()
{
    bool ok;
    float value = TongueUpOffset->text().toFloat(&ok);
    if (ok) {
        tongue_up_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onTongueDownOffsetChanged()
{
    bool ok;
    float value = TongueDownOffset->text().toFloat(&ok);
    if (ok) {
        tongue_down_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onTongueLeftOffsetChanged()
{
    bool ok;
    float value = TongueLeftOffset->text().toFloat(&ok);
    if (ok) {
        tongue_left_offset = value;
        updateOffsetsToInference();
    }
}

void PaperFaceTrackerWindow::onTongueRightOffsetChanged()
{
    bool ok;
    float value = TongueRightOffset->text().toFloat(&ok);
    if (ok) {
        tongue_right_offset = value;
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
    offset_map["jawLeft"] = jaw_left_offset;
    offset_map["jawRight"] = jaw_right_offset;
    offset_map["mouthLeft"] = mouth_left_offset;
    offset_map["mouthRight"] = mouth_right_offset;
    offset_map["tongueUp"] = tongue_up_offset;
    offset_map["tongueDown"] = tongue_down_offset;
    offset_map["tongueLeft"] = tongue_left_offset;
    offset_map["tongueRight"] = tongue_right_offset;

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
        LogText->setVisible(true);
        setFixedSize(900, 538); //
        LOG_INFO("已开启串口原始数据显示");
        ShowSerialDataButton->setText(Translator::tr("停止显示串口数据"));
    } else {
        LogText->setVisible(false);
        setFixedSize(900, 388); //
        LOG_INFO("已关闭串口原始数据显示");
        ShowSerialDataButton->setText(Translator::tr("显示串口数据"));
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
    auto str1 = Translator::tr("调整建议:");
    auto str2 = Translator::tr("增大q值, 减小r值: 更灵敏, 抖动更明显");
    auto str3 = Translator::tr("减小q值, 增大r值: 更平滑, 反应更滞后");
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
        page2RightLayout->addWidget(ImageLabelCal,1);
        ImageLabelCal->setFixedSize(350, 259);
        ImageLabelCal->setStyleSheet("border: 1px solid white;");
        ImageLabelCal->setAlignment(Qt::AlignCenter);
        QFont font = ImageLabelCal->font();
        font.setPointSize(14); // 设置字体大小为14
        font.setBold(true);    // 设置粗体
        ImageLabelCal->setFont(font);
        page2RightLayout->addStretch();
    }
    helpLabel->setVisible(false);
    dtLabel->setVisible(false);
    dtLineEdit->setVisible(false);
    qFactorLabel->setVisible(false);
    qFactorLineEdit->setVisible(false);
    rFactorLabel->setVisible(false);
    rFactorLineEdit->setVisible(false);

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
    MainPageButton->setText(Translator::tr("主页"));
    // 更新校准页面按钮文本
    CalibrationPageButton->setText(Translator::tr("标定页面"));

    if (!ImageLabel->text().isEmpty()) {
        ImageLabel->setText(Translator::tr("没有图像输入"));
    }

    restart_Button->setText(Translator::tr("重启"));
    FlashFirmwareButton->setText(Translator::tr("刷写固件"));
    label->setText(Translator::tr("亮度调整"));
    label_2->setText(Translator::tr("日志窗口："));
    label_16->setText(Translator::tr("IP地址："));
    label_17->setText(Translator::tr("旋转角度调整"));
    label_18->setText(Translator::tr("性能模式选择"));
    ShowSerialDataButton->setText(Translator::tr("串口日志"));
    SSIDText->setPlaceholderText(Translator::tr("请输入WIFI名字（仅支持2.4ghz）"));
    PasswordText->setPlaceholderText(Translator::tr("请输入WIFI密码"));
    wifi_send_Button->setText(Translator::tr("发送"));
    UseFilterBox->setText(Translator::tr("启用滤波（减少抖动）"));
    EnergyModeBox->setItemText(0, Translator::tr("普通模式"));
    EnergyModeBox->setItemText(1, Translator::tr("节能模式"));
    EnergyModeBox->setItemText(2, Translator::tr("性能模式"));

    label_11->setText(Translator::tr("嘴右移"));
    label_10->setText(Translator::tr("嘴左移"));
    label_9->setText(Translator::tr("下巴右移"));
    label_8->setText(Translator::tr("下巴左移"));
    label_7 ->setText(Translator::tr("下巴下移"));
    label_6->setText(Translator::tr("右脸颊"));
    label_5->setText(Translator::tr("左脸颊"));
    label_14->setText(Translator::tr("舌头向下"));
    label_15->setText(Translator::tr("舌头向左"));
    label_12->setText(Translator::tr("舌头伸出"));
    label_13->setText(Translator::tr("舌头向上"));
    label_31->setText(Translator::tr("舌头向右"));

    label_mouthClose->setText(Translator::tr("嘴闭合"));
    label_mouthFunnel->setText(Translator::tr("嘴漏斗形"));
    label_mouthPucker->setText(Translator::tr("嘴撅起"));
    label_mouthRollUpper->setText(Translator::tr("上唇内卷"));
    label_mouthRollLower->setText(Translator::tr("下唇内卷"));
    label_mouthShrugUpper->setText(Translator::tr("上唇耸起"));
    label_mouthShrugLower->setText(Translator::tr("下唇耸起"));

    dtLabel->setText(Translator::tr("时间步长(dt):"));
    qFactorLabel->setText(Translator::tr("过程噪声系数(q):"));
    rFactorLabel->setText(Translator::tr("测量噪声系数(r):"));

    calibrationModeLabel->setText(Translator::tr("校准模式") + ":");
    calibrationModeLabel->setToolTip(Translator::tr("校准模式") + ":");
    calibrationModeComboBox->setItemText(0, Translator::tr("自然模式"));
    calibrationModeComboBox->setItemText(1, Translator::tr("完整模式"));
    // 添加tooltip
    calibrationModeComboBox->setItemData(0, Translator::tr("放松面部，保持无表情的静默状态，以获取最准确的参数"), Qt::ToolTipRole);
    calibrationModeComboBox->setItemData(1, Translator::tr("做出尽可能多的面部表情，保持动作自然，不要特别夸张的表情"), Qt::ToolTipRole);

    calibrationStartButton->setText(Translator::tr("开始校准"));
    calibrationStartButton->setToolTip(Translator::tr("开始校准"));
    calibrationStopButton->setToolTip(Translator::tr("停止校准"));
    calibrationStopButton->setText(Translator::tr("停止校准"));
    calibrationResetButton->setText(Translator::tr("重置"));
    calibrationResetButton->setToolTip(Translator::tr("重置"));

    label_3->setText(Translator::tr("放大倍率"));
    label_20->setText(Translator::tr("偏置值"));
    lockLabel->setText(Translator::tr("锁定"));
    CheekPuffLeftOffset->setPlaceholderText(Translator::tr("偏置值"));
    CheekPuffRightOffset->setPlaceholderText(Translator::tr("偏置值"));
    JawOpenOffset->setPlaceholderText(Translator::tr("偏置值"));
    TongueOutOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthCloseOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthFunnelOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthPuckerOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthRollUpperOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthRollLowerOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthShrugUpperOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthShrugLowerOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthLeftOffset->setPlaceholderText(Translator::tr("偏置值"));
    MouthRightOffset->setPlaceholderText(Translator::tr("偏置值"));
    JawLeftOffset->setPlaceholderText(Translator::tr("偏置值"));
    JawRightOffset->setPlaceholderText(Translator::tr("偏置值"));
    TongueUpOffset->setPlaceholderText(Translator::tr("偏置值"));
    TongueDownOffset->setPlaceholderText(Translator::tr("偏置值"));
    TongueRightOffset->setPlaceholderText(Translator::tr("偏置值"));
    TongueLeftOffset->setPlaceholderText(Translator::tr("偏置值"));

    tutorialLink->setText(QString("<a href='https://fcnk6r4c64fa.feishu.cn/wiki/VSlnw4Zr0iVzXFkvT8TcbQFMn7c' style='color: #0066cc; font-size: 14pt; font-weight: bold;'>%1</a>")
                          .arg(Translator::tr("面捕调整教程")));

    auto str1 = Translator::tr("调整建议:");
    auto str2 = Translator::tr("增大q值, 减小r值: 更灵敏, 抖动更明显");
    auto str3 = Translator::tr("减小q值, 增大r值: 更平滑, 反应更滞后");
    helpLabel->setText(str1 + "\n" + str2 + "\n" + str3);

    updatePageWidth();
}

void PaperFaceTrackerWindow::updatePageWidth()
{
    if (stackedWidget && stackedWidget->currentIndex() == 1) {
        // 增加延迟确保UI更新完成
        setFixedHeight(538);
        QTimer::singleShot(200, this, [this]() {
            auto scrollBarWidth = 16;
            auto scrollWidth = scrollAreaWidgetContents->width();
            auto diffWidth = scrollWidth - scrollArea->width();
            if (diffWidth == -scrollBarWidth)
                return;

            auto newWidth = scrollArea->width() + diffWidth + scrollBarWidth;
            scrollArea->setFixedWidth(newWidth);
            setFixedSize(width() + diffWidth + scrollBarWidth, 538);
            updateGeometry();
        });
    }
    else {
        setFixedSize(900, showSerialData? 538 : 538 - 150);
        scrollArea->setMinimumWidth(500);
        updateGeometry();
    }
}

void PaperFaceTrackerWindow::collectData(const std::vector<float> &output,
    const std::unordered_map<std::string, size_t> &blendShapeIndexMap) {

    // 收集每个启用的参数的数据
    for (const auto& paramName : parameterOrder) {
        // 检查参数是否启用
        bool isLocked = false;
        if (parameterMap.find(paramName) != parameterMap.end()) {
            auto& paramInfo = parameterMap[paramName];
            if (paramInfo.enablCheckBox && paramInfo.enablCheckBox->isChecked()) {
                isLocked = true; // 参数被禁用
            }
        }

        if (!isLocked) {
            auto paramStr = paramName.toStdString();
            if (blendShapeIndexMap.contains(paramStr) && blendShapeIndexMap.at(paramStr) < output.size()) {
                float value = output[blendShapeIndexMap.at(paramStr)];

                // 简单的异常值过滤
                if (value >= 0.0f && value <= 1.0f) {  // 确保在合理范围内
                    calibration_data[paramStr].push_back(value);
                }
            }
        }
    }

    calibration_sample_count++;
}

void PaperFaceTrackerWindow::calculateCalibrationOffsets()
{
    calibration_offsets.clear();

    // 对每个参数计算偏置值（使用中位数作为基准，避免异常值影响）
    for (const auto& pair : calibration_data) {
        const std::string& paramName = pair.first;
        const std::vector<float>& values = pair.second;

        if (values.empty()) continue;

        // 创建副本并排序以计算中位数
        std::vector<float> sortedValues = values;
        std::sort(sortedValues.begin(), sortedValues.end());

        // 计算中位数
        float median;
        size_t size = sortedValues.size();
        if (size % 2 == 0) {
            median = (sortedValues[size/2 - 1] + sortedValues[size/2]) / 2.0f;
        } else {
            median = sortedValues[size/2];
        }

        // 将中位数乘以-1作为偏置值存储
        calibration_offsets[paramName] = median * -1;

        LOG_INFO("参数 {} 的偏置值计算完成: {}", paramName, median);
    }

    LOG_INFO("校准偏置值计算完成，共处理 {} 个样本", calibration_sample_count);
}

void PaperFaceTrackerWindow::applyCalibrationOffsets()
{
    // 断开所有偏置值输入框的信号连接，防止在设置文本时触发槽函数
    cheek_puff_left_offset += calibration_offsets["cheekPuffLeft"];
    cheek_puff_right_offset += calibration_offsets["cheekPuffRight"];
    jaw_open_offset += calibration_offsets["jawOpen"];
    tongue_out_offset += calibration_offsets["tongueOut"];
    mouth_close_offset += calibration_offsets["mouthClose"];
    mouth_funnel_offset += calibration_offsets["mouthFunnel"];
    mouth_pucker_offset += calibration_offsets["mouthPucker"];
    mouth_roll_upper_offset += calibration_offsets["mouthRollUpper"];
    mouth_roll_lower_offset += calibration_offsets["mouthRollLower"];
    mouth_shrug_upper_offset += calibration_offsets["mouthShrugUpper"];
    mouth_shrug_lower_offset += calibration_offsets["mouthShrugLower"];
    jaw_left_offset += calibration_offsets["jawLeft"];
    jaw_right_offset += calibration_offsets["jawRight"];
    mouth_left_offset += calibration_offsets["mouthLeft"];
    mouth_right_offset += calibration_offsets["mouthRight"];
    tongue_left_offset += calibration_offsets["tongueLeft"];
    tongue_right_offset += calibration_offsets["tongueRight"];
    tongue_up_offset += calibration_offsets["tongueUp"];
    tongue_down_offset += calibration_offsets["tongueDown"];

    CheekPuffLeftOffset->setText(QString::number(cheek_puff_left_offset, 'f', 4));
    CheekPuffRightOffset->setText(QString::number(cheek_puff_right_offset, 'f', 4));
    JawOpenOffset->setText(QString::number(jaw_open_offset, 'f', 4));
    TongueOutOffset->setText(QString::number(tongue_out_offset, 'f', 4));
    MouthCloseOffset->setText(QString::number(mouth_close_offset, 'f', 4));
    MouthFunnelOffset->setText(QString::number(mouth_funnel_offset, 'f', 4));
    MouthPuckerOffset->setText(QString::number(mouth_pucker_offset, 'f', 4));
    MouthRollUpperOffset->setText(QString::number(mouth_roll_upper_offset, 'f', 4));
    MouthRollLowerOffset->setText(QString::number(mouth_roll_lower_offset, 'f', 4));
    MouthShrugUpperOffset->setText(QString::number(mouth_shrug_upper_offset, 'f', 4));
    MouthShrugLowerOffset->setText(QString::number(mouth_shrug_lower_offset, 'f', 4));
    JawLeftOffset->setText(QString::number(jaw_left_offset, 'f', 4));
    JawRightOffset->setText(QString::number(jaw_right_offset, 'f', 4));
    MouthLeftOffset->setText(QString::number(mouth_left_offset, 'f', 4));
    MouthRightOffset->setText(QString::number(mouth_right_offset, 'f', 4));
    TongueLeftOffset->setText(QString::number(tongue_left_offset, 'f', 4));
    TongueRightOffset->setText(QString::number(tongue_right_offset, 'f', 4));
    TongueUpOffset->setText(QString::number(tongue_up_offset, 'f', 4));
    TongueDownOffset->setText(QString::number(tongue_down_offset, 'f', 4));

    // 重新连接所有偏置值输入框的信号
    connectOffsetChangeEvent();

    // 更新推理引擎的偏置值
    updateOffsetsToInference();
    LOG_INFO("参数偏置值已应用");
}

void PaperFaceTrackerWindow::calculateCalibrationAMP() {
calibration_amp_ratios.clear();

    // 对每个参数计算放大倍数（使用最大值计算比例）
    for (const auto& pair : calibration_data) {
        const std::string& paramName = pair.first;
        const std::vector<float>& values = pair.second;

        if (values.empty()) continue;

        // 找到最大值
        float max_value = *std::max_element(values.begin(), values.end());

        // 计算放大倍数，目标是让最大值达到1.0
        // 避免除零错误
        if (max_value > 0.0f) {
            float ratio = 1.0f / max_value;
            calibration_amp_ratios[paramName] = ratio;
            LOG_INFO("参数 {} 的放大倍数计算完成: {}", paramName, ratio);
        } else {
            calibration_amp_ratios[paramName] = 1.0f; // 如果最大值为0，则不放大
            LOG_INFO("参数 {} 的最大值为0，放大倍数设为1.0", paramName);
        }
    }

    LOG_INFO("校准放大倍数计算完成，共处理 {} 个样本", calibration_sample_count);
}

void PaperFaceTrackerWindow::applyCalibrationAMP() {
    // 为每个参数应用计算出的放大倍数
    auto applyRatioToBar = [](QLineEdit* bar, const std::unordered_map<std::string, float>& ratios,
                             const std::string& paramName, float currentAmp) {
        if (ratios.count(paramName) > 0) {
            float newAmp = ratios.at(paramName);
            // 确保新的放大倍数不会小于当前值（避免缩小）
            newAmp = max(newAmp, currentAmp);
            bar->setText(QString::number(newAmp, 'f', 4));  // 限制4位小数
            return newAmp;
        }
        return currentAmp;
    };

    // 应用放大倍数到各个振幅控件
    float current_cheek_puff_left = CheekPuffLeftBar->text().toFloat();
    float new_cheek_puff_left = applyRatioToBar(CheekPuffLeftBar, calibration_amp_ratios, "cheekPuffLeft", current_cheek_puff_left);

    float current_cheek_puff_right = CheekPuffRightBar->text().toFloat();
    float new_cheek_puff_right = applyRatioToBar(CheekPuffRightBar, calibration_amp_ratios, "cheekPuffRight", current_cheek_puff_right);

    float current_jaw_open = JawOpenBar->text().toFloat();
    float new_jaw_open = applyRatioToBar(JawOpenBar, calibration_amp_ratios, "jawOpen", current_jaw_open);

    float current_tongue_out = TongueOutBar->text().toFloat();
    float new_tongue_out = applyRatioToBar(TongueOutBar, calibration_amp_ratios, "tongueOut", current_tongue_out);

    float current_mouth_close = MouthCloseBar->text().toFloat();
    float new_mouth_close = applyRatioToBar(MouthCloseBar, calibration_amp_ratios, "mouthClose", current_mouth_close);

    float current_mouth_funnel = MouthFunnelBar->text().toFloat();
    float new_mouth_funnel = applyRatioToBar(MouthFunnelBar, calibration_amp_ratios, "mouthFunnel", current_mouth_funnel);

    float current_mouth_pucker = MouthPuckerBar->text().toFloat();
    float new_mouth_pucker = applyRatioToBar(MouthPuckerBar, calibration_amp_ratios, "mouthPucker", current_mouth_pucker);

    float current_mouth_roll_upper = MouthRollUpperBar->text().toFloat();
    float new_mouth_roll_upper = applyRatioToBar(MouthRollUpperBar, calibration_amp_ratios, "mouthRollUpper", current_mouth_roll_upper);

    float current_mouth_roll_lower = MouthRollLowerBar->text().toFloat();
    float new_mouth_roll_lower = applyRatioToBar(MouthRollLowerBar, calibration_amp_ratios, "mouthRollLower", current_mouth_roll_lower);

    float current_mouth_shrug_upper = MouthShrugUpperBar->text().toFloat();
    float new_mouth_shrug_upper = applyRatioToBar(MouthShrugUpperBar, calibration_amp_ratios, "mouthShrugUpper", current_mouth_shrug_upper);

    float current_mouth_shrug_lower = MouthShrugLowerBar->text().toFloat();
    float new_mouth_shrug_lower = applyRatioToBar(MouthShrugLowerBar, calibration_amp_ratios, "mouthShrugLower", current_mouth_shrug_lower);

    float current_jaw_left = JawLeftBar->text().toFloat();
    float new_jaw_left = applyRatioToBar(JawLeftBar, calibration_amp_ratios, "jawLeft", current_jaw_left);

    float current_jaw_right = JawRightBar->text().toFloat();
    float new_jaw_right = applyRatioToBar(JawRightBar, calibration_amp_ratios, "jawRight", current_jaw_right);

    float current_mouth_left = MouthLeftBar->text().toFloat();
    float new_mouth_left = applyRatioToBar(MouthLeftBar, calibration_amp_ratios, "mouthLeft", current_mouth_left);

    float current_mouth_right = MouthRightBar->text().toFloat();
    float new_mouth_right = applyRatioToBar(MouthRightBar, calibration_amp_ratios, "mouthRight", current_mouth_right);

    float current_tongue_left = TongueLeftBar->text().toFloat();
    float new_tongue_left = applyRatioToBar(TongueLeftBar, calibration_amp_ratios, "tongueLeft", current_tongue_left);

    float current_tongue_right = TongueRightBar->text().toFloat();
    float new_tongue_right = applyRatioToBar(TongueRightBar, calibration_amp_ratios, "tongueRight", current_tongue_right);

    float current_tongue_up = TongueUpBar->text().toFloat();
    float new_tongue_up = applyRatioToBar(TongueUpBar, calibration_amp_ratios, "tongueUp", current_tongue_up);

    float current_tongue_down = TongueDownBar->text().toFloat();
    float new_tongue_down = applyRatioToBar(TongueDownBar, calibration_amp_ratios, "tongueDown", current_tongue_down);

    // 重新连接所有输入框的信号
    connectOffsetChangeEvent();

    // 更新推理引擎的振幅映射
    inference->set_amp_map(getAmpMap());

    LOG_INFO("参数放大倍数已应用");
}

// 创建一个辅助函数来处理振幅值变化
void PaperFaceTrackerWindow::updateAmplitudeValue(const QString& paramName, float value) {
    if (parameterMap.find(paramName) != parameterMap.end()) {
        ParameterInfo& paramInfo = parameterMap[paramName];
        *(paramInfo.ampValue) = value;
    }
}

void PaperFaceTrackerWindow::onCheekPuffRightChanged() const
{
    bool ok;
    float value = CheekPuffRightBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->cheek_puff_right_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onCheekPuffLeftChanged() const
{
    bool ok;
    float value = CheekPuffLeftBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->cheek_puff_left_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onJawOpenChanged() const
{
    bool ok;
    float value = JawOpenBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->jaw_open_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onJawLeftChanged() const
{
    bool ok;
    float value = JawLeftBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->jaw_left_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onJawRightChanged() const
{
    bool ok;
    float value = JawRightBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->jaw_right_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthLeftChanged() const
{
    bool ok;
    float value = MouthLeftBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_left_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthRightChanged()
{
    bool ok;
    float value = MouthRightBar->text().toFloat(&ok);
    if (ok) {
        mouth_right_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueOutChanged()
{
    bool ok;
    float value = TongueOutBar->text().toFloat(&ok);
    if (ok) {
        tongue_out_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueLeftChanged()
{
    bool ok;
    float value = TongueLeftBar->text().toFloat(&ok);
    if (ok) {
        tongue_left_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueRightChanged()
{
    bool ok;
    float value = TongueRightBar->text().toFloat(&ok);
    if (ok) {
        tongue_right_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueUpChanged() const
{
    bool ok;
    float value = TongueUpBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->tongue_up_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onTongueDownChanged() const
{
    bool ok;
    float value = TongueDownBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->tongue_down_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthCloseChanged() const
{
    bool ok;
    float value = MouthCloseBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_close_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthFunnelChanged() const
{
    bool ok;
    float value = MouthFunnelBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_funnel_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthPuckerChanged() const
{
    bool ok;
    float value = MouthPuckerBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_pucker_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthRollUpperChanged() const
{
    bool ok;
    float value = MouthRollUpperBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_roll_upper_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthRollLowerChanged() const
{
    bool ok;
    float value = MouthRollLowerBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_roll_lower_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthShrugUpperChanged() const
{
    bool ok;
    float value = MouthShrugUpperBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_shrug_upper_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::onMouthShrugLowerChanged() const
{
    bool ok;
    float value = MouthShrugLowerBar->text().toFloat(&ok);
    if (ok) {
        const_cast<PaperFaceTrackerWindow*>(this)->mouth_shrug_lower_amp = value;
    }
    inference->set_amp_map(getAmpMap());
}

void PaperFaceTrackerWindow::clearCalibrationParameters(bool clearOffset, bool clearAmp) {
    // 遍历所有参数
    for (const auto& paramName : parameterOrder) {
        if (parameterMap.find(paramName) != parameterMap.end()) {
            auto& paramInfo = parameterMap[paramName];

            // 检查参数是否被锁定（checkbox未选中）
            bool isLocked = paramInfo.enablCheckBox && paramInfo.enablCheckBox->isChecked();

            // 如果参数未被锁定，则重置其值
            if (!isLocked) {

                if (clearOffset) {
                    // 重置偏置值输入框
                    if (paramInfo.offsetEdit) {
                        paramInfo.offsetEdit->setText("0.0000");
                    }
                    // 重置对应的变量值
                    if (paramInfo.offsetValue) {
                        *(paramInfo.offsetValue) = 0.0f;
                    }
                }
                if (clearAmp){
                    // 重置放大倍数输入框
                    if (paramInfo.ampEdit) {
                        paramInfo.ampEdit->setText("1.0000");
                    }
                    if (paramInfo.ampValue) {
                        *(paramInfo.ampValue) = 1.0f;
                    }
                }
            }
        }
    }

    // 更新推理引擎的偏置值和放大倍数
    updateOffsetsToInference();

    LOG_INFO("校准参数已重置（已锁定的参数保持不变）");
}
