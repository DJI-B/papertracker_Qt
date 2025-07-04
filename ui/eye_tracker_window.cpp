//
// Created by JellyfishKnight on 25-3-11.
//
#include <opencv2/imgproc.hpp>
#include "eye_tracker_window.hpp"

#include <eye_inference.hpp>
#include <face_tracker_window.hpp>
#include <QMessageBox>
#include <roi_event.hpp>

#include "ui_eye_tracker_window.h"
#include <QInputDialog>
#include "tools.hpp"
#include <algorithm>
// 生成测试数据的函数
struct TestEyeData {
    float eyeLidLeft;
    float eyeLidRight;
    float eyeLeftX;
    float eyeLeftY;
    float eyeRightX;
    float eyeRightY;
    float pupilDilation;
};

// 基于官方参数文档的修复版本
TestEyeData generateTestData(double time) {
    TestEyeData data;

    // 将20秒分为不同的测试阶段
    double cycle_time = fmod(time, 20.0);

    if (cycle_time < 5.0) {
        // 阶段1 (0-5秒): 测试X轴完整范围 (-1.0 到 1.0)
        double progress = cycle_time / 5.0;

        // 重要修复：眼球位置应该是 -1.0 到 1.0，不是 0.0 到 1.0
        double eyeX = -1.0 + (progress * 2.0);  // 从-1.0到1.0

        data.eyeLeftX = static_cast<float>(eyeX);
        data.eyeLeftY = 0.0f;                    // Y轴保持中心（0.0，不是0.5）
        data.eyeRightX = static_cast<float>(eyeX);
        data.eyeRightY = 0.0f;

        // 修复：眼睑正常范围是0.0-0.75，不发送1.0避免睁大效果
        data.eyeLidLeft = 0.75f;                 // 正常最大开度
        data.eyeLidRight = 0.75f;
        data.pupilDilation = 0.5f;

        static int x_debug_count = 0;
        if (x_debug_count % 60 == 0) {
            LOG_INFO("X轴测试: progress={:.3f}, eyeX={:.3f}, 预期: 从-1.0（最左）移动到1.0（最右）", progress, eyeX);
        }
        x_debug_count++;

    } else if (cycle_time < 10.0) {
        // 阶段2 (5-10秒): 测试Y轴完整范围 (-1.0 到 1.0)
        double progress = (cycle_time - 5.0) / 5.0;

        // 重要修复：Y轴也应该是 -1.0 到 1.0
        double eyeY = -1.0 + (progress * 2.0);  // 从-1.0（向下看）到1.0（向上看）

        data.eyeLeftX = 0.0f;                    // X轴保持中心
        data.eyeLeftY = static_cast<float>(eyeY);
        data.eyeRightX = 0.0f;
        data.eyeRightY = static_cast<float>(eyeY);

        data.eyeLidLeft = 0.75f;                 // 保持正常开度
        data.eyeLidRight = 0.75f;
        data.pupilDilation = 0.5f;

        static int y_debug_count = 0;
        if (y_debug_count % 60 == 0) {
            LOG_INFO("Y轴测试: progress={:.3f}, eyeY={:.3f}, 预期: 从-1.0（向下）移动到1.0（向上）", progress, eyeY);
        }
        y_debug_count++;

    } else if (cycle_time < 15.0) {
        // 阶段3 (10-15秒): 测试眼睑范围 (0.0 到 0.75，然后0.75到1.0)
        double progress = (cycle_time - 10.0) / 5.0;

        data.eyeLeftX = 0.0f;                    // 眼球保持中心
        data.eyeLeftY = 0.0f;
        data.eyeRightX = 0.0f;
        data.eyeRightY = 0.0f;

        // 修复：测试完整的眼睑范围，包括睁大效果
        float eyeLidValue;
        if (progress < 0.75) {
            // 前75%时间：测试正常开合度 (0.0 到 0.75)
            eyeLidValue = static_cast<float>(progress);
        } else {
            // 后25%时间：测试睁大效果 (0.75 到 1.0)
            float widenProgress = (progress - 0.75f) / 0.25f;
            eyeLidValue = 0.75f + (widenProgress * 0.25f);
        }

        data.eyeLidLeft = eyeLidValue;
        data.eyeLidRight = eyeLidValue;
        data.pupilDilation = 0.5f;

        static int lid_debug_count = 0;
        if (lid_debug_count % 60 == 0) {
            if (progress < 0.75) {
                LOG_INFO("眼睑测试: progress={:.3f}, eyeLid={:.3f}, 阶段: 正常开合度（0.0闭合→0.75张开）", progress, eyeLidValue);
            } else {
                LOG_INFO("眼睑测试: progress={:.3f}, eyeLid={:.3f}, 阶段: 睁大效果（0.75→1.0睁大）", progress, eyeLidValue);
            }
        }
        lid_debug_count++;

    } else {
        // 阶段4 (15-20秒): 测试瞳孔扩张范围 (0.0 到 1.0)
        double progress = (cycle_time - 15.0) / 5.0;

        data.eyeLeftX = 0.0f;                    // 眼球保持中心
        data.eyeLeftY = 0.0f;
        data.eyeRightX = 0.0f;
        data.eyeRightY = 0.0f;

        data.eyeLidLeft = 0.75f;                 // 眼睛保持正常张开
        data.eyeLidRight = 0.75f;
        data.pupilDilation = static_cast<float>(progress); // 0.0到1.0

        static int pupil_debug_count = 0;
        if (pupil_debug_count % 60 == 0) {
            LOG_INFO("瞳孔测试: progress={:.3f}, pupil={:.3f}, 预期: 从0.0（收缩）到1.0（扩张）", progress, data.pupilDilation);
        }
        pupil_debug_count++;
    }

    return data;
}
PaperEyeTrackerWindow::PaperEyeTrackerWindow(QWidget* parent) :
    QWidget(parent) {
    if (instance == nullptr)
        instance = this;
    else
        throw std::exception("当前已经打开了眼追窗口，请不要重复打开");
    ui.setupUi(this);
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
    setFixedSize(907, 614);
    append_log_window(ui.LogText);
    // 初始化眼睛位置显示控件
    leftEyePositionWidget = new EyePositionWidget(ui.LeftEyePositionFrame);
    leftEyePositionWidget->setGeometry(0, 0, ui.LeftEyePositionFrame->width(), ui.LeftEyePositionFrame->height());
    rightEyePositionWidget = new EyePositionWidget(ui.RightEyePositionFrame);
    rightEyePositionWidget->setGeometry(0, 0, ui.RightEyePositionFrame->width(), ui.RightEyePositionFrame->height());
    // 创建眼睛同步控制下拉框
    eyeSyncComboBox = new QComboBox(ui.page_2);
    eyeSyncComboBox->setGeometry(QRect(740, 190, 150, 31));
    eyeSyncComboBox->addItem("双眼眼皮独立控制");
    eyeSyncComboBox->addItem("左眼眼皮控制双眼眼皮");
    eyeSyncComboBox->addItem("右眼眼皮控制双眼眼皮");
    eyeSyncComboBox->setObjectName("eyeSyncComboBox");
    for (int i = 0; i < EYE_NUM; i++) {
        compensated_eye_openness[i] = 0.0;
    }

    // 连接信号槽
    connect(eyeSyncComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperEyeTrackerWindow::onEyeSyncModeChanged);
    // 初始化声音提示
    // startSound = new QSoundEffect(this);
    // endSound = new QSoundEffect(this);

    // QString appDir = QCoreApplication::applicationDirPath();
    // startSound->setSource(QUrl::fromLocalFile(appDir + "/sounds/calibration_start.wav"));
    // endSound->setSource(QUrl::fromLocalFile(appDir + "/sounds/calibration_end.wav"));

    connect_callbacks();
    bound_pages();

    // 添加ROI事件
    QLabel* images_labels[2] = {
        ui.LeftEyeImage,
        ui.RightEyeImage
    };
    for (int i = 0; i < EYE_NUM; i++) {
        auto* roiFilter = new ROIEventFilter([this](QRect rect, bool isEnd, int tag) {
            int x = rect.x();
            int y = rect.y();
            int width = rect.width();
            int height = rect.height();

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
            if (x + width > 261) {
                width = 261 - x;
            }
            if (y + height > 261) {
                height = 261 - y;
            }
            // 确保最终的宽度和高度为正值
            width = max(0, width);
            height = max(0, height);

            // 更新 roi_rect
            roi_rect[tag].is_roi_end = isEnd;
            roi_rect[tag] = Rect(x, y, width, height);
            }, images_labels[i], i);
        images_labels[i]->installEventFilter(roiFilter);
    }

    // 添加输入框焦点事件处理
    ui.SSIDInput->installEventFilter(this);
    ui.PassWordInput->installEventFilter(this);
    // 允许Tab键在输入框之间跳转
    ui.SSIDInput->setTabChangesFocus(true);
    ui.PassWordInput->setTabChangesFocus(true);
    setFocus();
    // QPushButton* calibrateButton = new QPushButton("开始校准", ui.page);
    //    // calibrateButton->setGeometry(QRect(750, 340, 111, 41));
    //    // connect(calibrateButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::startCalibration);
    //    // QPushButton* centerButton = new QPushButton("标定中心", ui.page);
    //    // centerButton->setGeometry(QRect(750, 390, 111, 41)); // 位置在校准按钮下方
    //    // connect(centerButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::centerCalibration);
    //    // // 添加以下代码：
    //    // QPushButton* eyeOpenButton = new QPushButton("眼睛完全张开", ui.page);
    //    // eyeOpenButton->setGeometry(QRect(750, 440, 111, 41)); // 位置在标定中心按钮下方
    //    // connect(eyeOpenButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::calibrateEyeOpen);
    //    //
    //    // QPushButton* eyeCloseButton = new QPushButton("眼睛完全闭合", ui.page);
    //    // eyeCloseButton->setGeometry(QRect(750, 490, 111, 41)); // 位置在眼睛张开按钮下方
    //    // connect(eyeCloseButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::calibrateEyeClose);
    // 连接按钮信号到槽
    connect(ui.settingsCalibrateButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::startCalibration);
    connect(ui.settingsCenterButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::centerCalibration);
    connect(ui.settingsEyeOpenButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::calibrateEyeOpen);
    connect(ui.settingsEyeCloseButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::calibrateEyeClose);
    osc_manager = std::make_shared<OscManager>();
    config_writer = std::make_shared<ConfigWriter>("./eye_config.json");
    config = config_writer->get_config<PaperEyeTrackerConfig>();

    set_config();

    LOG_INFO("正在加载模型...")
        for (int i = 0; i < EYE_NUM; i++) {
            inference_[i] = std::make_shared<EyeInference>();
            inference_[i]->load_model("");
        }
    LOG_INFO("模型加载完成");
    LOG_INFO("正在初始化OSC...");
    if (osc_manager->init("127.0.0.1", 8889)) {
        osc_manager->setLocationPrefix("");
        LOG_INFO("OSC初始化成功");
    }
    else {
        LOG_ERROR("OSC初始化失败，请检查网络连接");
    }
    for (int i = 0; i < EYE_NUM; i++) {
        // 设置默认校准值
        eye_calib_data[i].calib_XOFF = 261 / 2.0;  // 假设ROI宽度为261
        eye_calib_data[i].calib_YOFF = 261 / 2.0;  // 假设ROI高度为261
        eye_calib_data[i].calib_XMIN = 0;
        eye_calib_data[i].calib_XMAX = 261;
        eye_calib_data[i].calib_YMIN = 0;
        eye_calib_data[i].calib_YMAX = 261;

        // 如果已有校准数据（例如从配置文件加载），可以在这里设置has_calibration = true
        eye_calib_data[i].has_calibration = false;
    }

    // 设置轴翻转配置，可根据需要调整
    flip_x_axis[LEFT_TAG] = false;
    flip_x_axis[RIGHT_TAG] = true;  // 右眼可能需要翻转X轴
    flip_y_axis = false;  // 根据您的坐标系决定是否需要翻转Y轴
    // 在构造函数中初始化校准数据
    for (int i = 0; i < EYE_NUM; i++) {
        eye_calib_data[i].calib_XMIN = 9999999;
        eye_calib_data[i].calib_XMAX = -9999999;
        eye_calib_data[i].calib_YMIN = 9999999;
        eye_calib_data[i].calib_YMAX = -9999999;
        eye_calib_data[i].has_calibration = false;
    }

    // 初始化串口和wifi
    for (int i = 0; i < EYE_NUM; i++) {
        image_stream[i] = std::make_shared<ESP32VideoStream>();
    }
    serial_port_ = std::make_shared<SerialPortManager>();

    serial_port_->init();
    // init serial port manager
    serial_port_->registerCallback(
        PACKET_DEVICE_STATUS,
        [this](const std::string& ip, int brightness, int power, int version) {
            current_esp32_version = version;
            if (version != LEFT_VERSION && version != RIGHT_VERSION) {
                static bool version_warning = false;
                if (!version_warning) {
                    QMessageBox msgBox;
                    msgBox.setWindowIcon(this->windowIcon());
                    msgBox.setText(tr("检测到面捕设备，请打开面捕界面进行设置"));
                    msgBox.exec();
                    version_warning = true;
                }
                return;
            }
            // 使用Qt的线程安全方式更新UI
            QMetaObject::invokeMethod(this, [ip, brightness, power, version = version - 2, this]() {
                if (current_ip[version] != "http://" + ip) {
                    current_ip[version] = "http://" + ip;
                    // 更新IP地址显示，添加 http:// 前缀
                    this->setIPText(version, QString::fromStdString(current_ip[version]));
                    LOG_INFO("IP地址已更新: {}", current_ip[version]);
                    start_image_download(version);
                }
                // 可以添加其他状态更新的日志，如果需要的话
                }, Qt::QueuedConnection);
        }
    );


    while (serial_port_->status() == SerialStatus::CLOSED) {}

    if (serial_port_->status() == SerialStatus::FAILED) {
        LOG_WARN("没有检测到眼追设备，尝试从配置文件中读取地址...");
        if (!config.left_ip.empty()) {
            LOG_INFO("从配置文件中读取左眼地址成功");
            current_ip[LEFT_TAG] = config.left_ip;
            start_image_download(LEFT_TAG);
        }
        else {
            QMessageBox msgBox;
            msgBox.setWindowIcon(this->windowIcon());
            msgBox.setText(QApplication::translate("PaperTrackerMainWindow","未找到左眼配置文件信息，请将设备通过数据线连接到电脑进行首次配置"));
            msgBox.exec();
        }
        if (!config.right_ip.empty()) {
            LOG_INFO("从配置文件中读取右眼地址成功");
            current_ip[RIGHT_TAG] = config.right_ip;
            start_image_download(RIGHT_TAG);
        }
        else {
            QMessageBox msgBox;
            msgBox.setWindowIcon(this->windowIcon());
            msgBox.setText(QApplication::translate("PaperTrackerMainWindow","未找到右眼配置文件信息，请将设备通过数据线连接到电脑进行首次配置"));
            msgBox.exec();
        }
    }
    else {
        if (current_ip[LEFT_TAG].empty() && !config.left_ip.empty()) {
            current_ip[LEFT_TAG] = config.left_ip;
            start_image_download(LEFT_TAG);
        }
        else {
            if (config.left_ip.empty()) {
                QMessageBox msgBox;
                msgBox.setWindowIcon(this->windowIcon());
                msgBox.setText(QApplication::translate("PaperTrackerMainWindow","未找到左眼配置文件信息，请将设备通过数据线连接到电脑进行首次配置"));
                msgBox.exec();
            }
        }
        if (current_ip[RIGHT_TAG].empty() && !config.right_ip.empty()) {
            current_ip[RIGHT_TAG] = config.right_ip;
            start_image_download(RIGHT_TAG);
        }
        else {
            if (config.right_ip.empty()) {
                QMessageBox msgBox;
                msgBox.setWindowIcon(this->windowIcon());
                msgBox.setText(QApplication::translate("PaperTrackerMainWindow","未找到右眼配置文件信息，请将设备通过数据线连接到电脑进行首次配置"));
                msgBox.exec();
            }
        }
        LOG_INFO("有线模式眼追连接成功");
        setSerialStatusLabel("有线模式眼追连接成功");
    }
    // 将下面的代码添加到 PaperEyeTrackerWindow 构造函数中，在设置页面初始化之后

    // 添加视频流显示到设置页面
    QLabel* leftEyeVideoLabel = new QLabel(ui.page_2);
    leftEyeVideoLabel->setGeometry(QRect(30, 400, 200, 200));
    leftEyeVideoLabel->setText("左眼视频流");
    leftEyeVideoLabel->setFrameShape(QFrame::StyledPanel);
    leftEyeVideoLabel->setObjectName("leftEyeVideoLabel");

    QLabel* rightEyeVideoLabel = new QLabel(ui.page_2);
    rightEyeVideoLabel->setGeometry(QRect(480, 400, 200, 200));
    rightEyeVideoLabel->setText("右眼视频流");
    rightEyeVideoLabel->setFrameShape(QFrame::StyledPanel);
    rightEyeVideoLabel->setObjectName("rightEyeVideoLabel");
    // 为左眼添加调整按钮
    QPushButton* leftIncButton = new QPushButton("+", ui.page_2);
    leftIncButton->setGeometry(QRect(330, 330, 30, 30));
    leftIncButton->setObjectName("leftIncButton");

    QPushButton* leftDecButton = new QPushButton("-", ui.page_2);
    leftDecButton->setGeometry(QRect(370, 330, 30, 30));
    leftDecButton->setObjectName("leftDecButton");

    // 为右眼添加调整按钮
    QPushButton* rightIncButton = new QPushButton("+", ui.page_2);
    rightIncButton->setGeometry(QRect(780, 330, 30, 30));
    rightIncButton->setObjectName("rightIncButton");

    QPushButton* rightDecButton = new QPushButton("-", ui.page_2);
    rightDecButton->setGeometry(QRect(820, 330, 30, 30));
    rightDecButton->setObjectName("rightDecButton");

    // 添加标签
    QLabel* leftAdjustLabel = new QLabel(ui.page_2);
    leftAdjustLabel->setGeometry(QRect(290, 330, 40, 30));
    leftAdjustLabel->setText(QApplication::translate("PaperTrackerMainWindow", "调整:"));

    QLabel* rightAdjustLabel = new QLabel(ui.page_2);
    rightAdjustLabel->setGeometry(QRect(740, 330, 40, 30));
    rightAdjustLabel->setText(QApplication::translate("PaperTrackerMainWindow", "调整:"));

    // 连接信号和槽
    connect(leftIncButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::onLeftEyeValueIncrease);
    connect(leftDecButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::onLeftEyeValueDecrease);
    connect(rightIncButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::onRightEyeValueIncrease);
    connect(rightDecButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::onRightEyeValueDecrease);
    // 初始化滚动条的值
    ui.LeftRotateBar->setValue(current_rotate_angle[LEFT_TAG]);
    ui.RightRotateBar->setValue(current_rotate_angle[RIGHT_TAG]);

    // 连接滚动条信号到槽
    connect(ui.LeftRotateBar, &QScrollBar::valueChanged, this, &PaperEyeTrackerWindow::onLeftRotateAngleChanged);
    connect(ui.RightRotateBar, &QScrollBar::valueChanged, this, &PaperEyeTrackerWindow::onRightRotateAngleChanged);
    create_sub_thread();
    // 创建自动保存配置的定时器
    auto_save_timer = new QTimer(this);
    connect(auto_save_timer, &QTimer::timeout, this, [this]() {
        config = generate_config();
        config_writer->write_config(config);
        LOG_DEBUG("眼追配置已自动保存");
    });
    auto_save_timer->start(10000); // 10000毫秒 = 10秒
    // 根据配置文件设置校准状态
    // calibrated_position = config.left_has_calibration || config.right_has_calibration;
    // calibrated_center = calibrated_position; // 如果有校准数据，则中心点也已经校准
    // calibrated_eye_open = config.left_eye_fully_open > 0 || config.right_eye_fully_open > 0;
    // calibrated_eye_close = config.left_eye_fully_closed > 0 || config.right_eye_fully_closed > 0;

    // 更新按钮状态
    updateCalibrationButtonStates();
    retranslateUI();
    connect(&TranslatorManager::instance(), &TranslatorManager::languageSwitched,
    this, &PaperEyeTrackerWindow::retranslateUI);
}

void PaperEyeTrackerWindow::setVideoImage(int version, const cv::Mat& image) {
    auto image_label = version == LEFT_TAG ? ui.LeftEyeImage : ui.RightEyeImage;
    auto setting_image_label = version == LEFT_TAG ?
        ui.page_2->findChild<QLabel*>("leftEyeVideoLabel") :
        ui.page_2->findChild<QLabel*>("rightEyeVideoLabel");

    if (image.empty()) {
        QMetaObject::invokeMethod(this, [this, image_label, setting_image_label]() {
            image_label->clear(); // 清除图片
            image_label->setText(QApplication::translate("PaperTrackerMainWindow", "没有图像输入"));

            if (setting_image_label) {
                setting_image_label->clear();
                setting_image_label->setText(QApplication::translate("PaperTrackerMainWindow", "没有图像输入"));
            }
            }, Qt::QueuedConnection);
        return;
    }

    QMetaObject::invokeMethod(this, [this, image = image.clone(), image_label, setting_image_label]() {
        auto qimage = QImage(image.data, image.cols, image.rows, image.step, QImage::Format_RGB888);
        auto pix_map = QPixmap::fromImage(qimage);

        // 更新主页面视频流
        image_label->setPixmap(pix_map);
        image_label->setScaledContents(true);
        image_label->update();

        // 更新设置页面视频流
        if (setting_image_label) {
            setting_image_label->setPixmap(pix_map);
            setting_image_label->setScaledContents(true);
            setting_image_label->update();
        }
        }, Qt::QueuedConnection);
}

void PaperEyeTrackerWindow::create_sub_thread() {
    for (int i = 0; i < EYE_NUM; i++) {
        update_ui_thread[i] = std::thread([this, version = i]() {
            auto last_time = std::chrono::high_resolution_clock::now();
            double fps_total = 0;
            double fps_count = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            while (is_running()) {
                updateWifiLabel(version);
                updateBatteryStatus(version);
                updateSerialLabel(current_esp32_version);
                auto start_time = std::chrono::high_resolution_clock::now();
                try {
                    if (fps_total > 1000) {
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
                    fps = fps_total / fps_count;
                    cv::Mat frame = getVideoImage(version);
                    // draw rect on frame
                    cv::Mat show_image;
                    if (!frame.empty()) {
                        cv::resize(frame, frame, cv::Size(ui.LeftEyeImage->size().width(), ui.LeftEyeImage->size().height()), cv::INTER_NEAREST);
                        {
                            // 添加旋转处理
                            auto rotate_angle = getRotateAngle(version);
                            if (rotate_angle != 0) {
                                int y = frame.rows / 2;
                                int x = frame.cols / 2;
                                auto rotate_matrix = cv::getRotationMatrix2D(cv::Point(x, y), rotate_angle, 1);
                                cv::warpAffine(frame, frame, rotate_matrix, frame.size(), cv::INTER_NEAREST);
                            }
                            std::lock_guard<std::mutex> lock_guard(outputs_mutex[version]);
                            if (!outputs[version].empty()) {
                                for (int j = 0; j < EYE_OUTPUT_SIZE; j += 2) {
                                    cv::Point2f point;
                                    point.x = outputs[version][j];
                                    point.y = outputs[version][j + 1];
                                    if (j == EYE_OUTPUT_SIZE - 2) {
                                        cv::circle(frame, point, 2, cv::Scalar(0, 255, 0), 2);
                                    }
                                    else {
                                        // draw eye points
                                        cv::circle(frame, point, 2, cv::Scalar(0, 0, 255), 2);
                                    }
                                }
                                for (int j = 0; j < EYE_OUTPUT_SIZE - 2; j += 2) {
                                    cv::Point2f point;
                                    point.x = outputs[version][j];
                                    point.y = outputs[version][j + 1];
                                    cv::Point2f next_point;
                                    next_point.x = outputs[version][(j + 2) % (EYE_OUTPUT_SIZE - 2)];
                                    next_point.y = outputs[version][(j + 3) % (EYE_OUTPUT_SIZE - 2)];
                                    cv::line(frame, point, next_point, cv::Scalar(0, 255, 0), 2);
                                }
                            }
                        }
                        cv::rectangle(frame, roi_rect[version].rect, cv::Scalar(0, 255, 0), 2);
                        show_image = frame;
                    }
                    setVideoImage(version, show_image);
                    // 控制帧率
                }
                catch (const std::exception& e) {
                    // 使用Qt方式记录日志，而不是minilog
                    QMetaObject::invokeMethod(this, [&e]() {
                        LOG_ERROR("错误, 视频处理异常: {}", e.what());
                        }, Qt::QueuedConnection);
                }
                auto end_time = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                int delay_ms = max(0, static_cast<int>(1000.0 / min(get_max_fps() + 30, 50) - elapsed));
                // LOG_DEBUG("UIFPS:" +  std::to_string(min(get_max_fps() + 30, 60)));
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
            });

        inference_thread[i] = std::thread([this, version = i]() {
            auto last_time = std::chrono::high_resolution_clock::now();
            double fps_total = 0;
            double fps_count = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            while (is_running()) {
                if (fps_total > 1000) {
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
                fps = fps_total / fps_count;
                // LOG_DEBUG("模型FPS： {}", fps);

                auto start_time = std::chrono::high_resolution_clock::now();
                // 设置时间序列
                inference_[version]->set_dt(duration.count() / 1000.0);

                auto frame = getVideoImage(version);
                // 推理处理
                if (!frame.empty()) {
                    auto rotate_angle = getRotateAngle(version);
                    cv::resize(frame, frame, cv::Size(280, 280), cv::INTER_NEAREST);
                    int y = frame.rows / 2;
                    int x = frame.cols / 2;
                    auto rotate_matrix = cv::getRotationMatrix2D(cv::Point(x, y), rotate_angle, 1);
                    cv::warpAffine(frame, frame, rotate_matrix, frame.size(), cv::INTER_NEAREST);
                    cv::Mat infer_frame;
                    infer_frame = frame.clone();
                    auto roi_rect = getRoiRect(version);
                    if (!roi_rect.rect.empty() && roi_rect.is_roi_end) {
                        infer_frame = infer_frame(roi_rect.rect);
                    }
                    if (version == LEFT_TAG) {
                    // 水平翻转图像（沿y轴对称）
                    cv::flip(infer_frame, infer_frame, 1);  // 参数1表示水平翻转
                    }
                    inference_[version]->inference(infer_frame);
                    auto temp = inference_[version]->get_output();
                    if (temp.empty()) {
                        continue;
                    }
                    if (version == LEFT_TAG) {
                        // 对每个坐标点进行处理
                        for (int j = 0; j < temp.size(); j += 2) {
                            // 只调整x坐标 (水平翻转)
                            temp[j] = 1.0f - temp[j];  // 图像宽度减去x坐标值
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock_guard(outputs_mutex[version]);
                        outputs[version] = temp;
                        for (int j = 0; j < EYE_OUTPUT_SIZE; j += 2) {
                            outputs[version][j] = outputs[version][j] * roi_rect.rect.width + roi_rect.rect.x;
                            outputs[version][j + 1] = outputs[version][j + 1] * roi_rect.rect.height + roi_rect.rect.y;
                        }
                    }
                    // 后处理逻辑
                    double dist_1 = cv::norm(outputs[version][1] - outputs[version][3]);
                    double dist_2 = cv::norm(outputs[version][2] - outputs[version][4]);
                    double dist = (dist_1 + dist_2) / 2;
                    {
                    std::lock_guard<std::mutex> lock_guard(results_mutex[version]);

                        // 原始眼睛开合度值，不再使用百分位计算
                        eye_open[version] = dist;

                        // 处理瞳孔位置
                        pupil[version].x = outputs[version][EYE_OUTPUT_SIZE - 2];
                        pupil[version].y = outputs[version][EYE_OUTPUT_SIZE - 1];

                        // 记录校准数据
                        if (is_calibrating) {
                            // 只更新位置校准数据
                            eye_calib_data[version].calib_XMIN = min(eye_calib_data[version].calib_XMIN, pupil[version].x);
                            eye_calib_data[version].calib_XMAX = max(eye_calib_data[version].calib_XMAX, pupil[version].x);
                            eye_calib_data[version].calib_YMIN = min(eye_calib_data[version].calib_YMIN, pupil[version].y);
                            eye_calib_data[version].calib_YMAX = max(eye_calib_data[version].calib_YMAX, pupil[version].y);
                        }


                    // 同时更新坐标校准数据，使用pupil[version]而不是pupil_point
                    eye_calib_data[version].calib_XMIN = min(eye_calib_data[version].calib_XMIN, pupil[version].x);
                    eye_calib_data[version].calib_XMAX = max(eye_calib_data[version].calib_XMAX, pupil[version].x);
                    eye_calib_data[version].calib_YMIN = min(eye_calib_data[version].calib_YMIN, pupil[version].y);
                    eye_calib_data[version].calib_YMAX = max(eye_calib_data[version].calib_YMAX, pupil[version].y);

                    // 如果是首个校准样本，设置中心点
                    if (open_list[version].size() == 1) {
                        eye_calib_data[version].calib_XOFF = pupil[version].x;
                        eye_calib_data[version].calib_YOFF = pupil[version].y;
                    }

                        // if (open_list[version].size() > CALIBRATION_SAMPLES) {
                        //     if (is_calibrating) {
                        //         calibrated[version] = true;
                        //         eye_calib_data[version].has_calibration = true;
                        //         LOG_INFO("眼睛{}校准完成：已收集足够样本数据 ({} 个样本点)",
                        //                 version == LEFT_TAG ? "左" : "右",
                        //                 open_list[version].size());
                        //         is_calibrating = false; // 结束校准状态
                        //     }
                        // }
                }

                        pupil[version].x = outputs[version][EYE_OUTPUT_SIZE - 2];
                        pupil[version].y = outputs[version][EYE_OUTPUT_SIZE - 1];

                    if (is_calibrating) {
                    // 对当前处理的眼睛进行校准数据收集，由于左右眼是两个不同的线程，这里不会有冲突
                    std::lock_guard<std::mutex> lock(results_mutex[version]);

                    // 只在有效瞳孔位置时更新校准数据
                    if (pupil[version].x > 0 && pupil[version].y > 0) {
                        // 更新坐标范围
                        eye_calib_data[version].calib_XMIN = min(eye_calib_data[version].calib_XMIN, pupil[version].x);
                        eye_calib_data[version].calib_XMAX = max(eye_calib_data[version].calib_XMAX, pupil[version].x);
                        eye_calib_data[version].calib_YMIN = min(eye_calib_data[version].calib_YMIN, pupil[version].y);
                        eye_calib_data[version].calib_YMAX = max(eye_calib_data[version].calib_YMAX, pupil[version].y);

                        // 如果是首个校准样本，设置中心点
                        if (open_list[version].empty()) {
                            eye_calib_data[version].calib_XOFF = pupil[version].x;
                            eye_calib_data[version].calib_YOFF = pupil[version].y;
                        }
                    }
                }
                }
                auto end_time = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                int delay_ms = max(0, static_cast<int>(1000.0 / get_max_fps() - elapsed));
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
            });
    }

   osc_send_thread = std::thread([this] ()
{
    int debug_counter = 0;

    // 从60Hz计算帧间隔时间，毫秒
    const int frame_interval_ms = 1000 / 60;

    // 插帧相关变量
    const int interpolation_frames = 3; // 每两个真实数据帧之间插入的帧数
    int current_interp_frame = 0;

    // 存储插值用的目标状态

    bool hasValidStartState = false;

    // 用于卡尔曼滤波的状态变量
    struct EyeState {
        double eyeLidValue = 0.0;
        double eyeXValue = 0.0;
        double eyeYValue = 0.0;
        double eyeLidVelocity = 0.0;
        double eyeXVelocity = 0.0;
        double eyeYVelocity = 0.0;
        double pupilDilation = 0.5; // 新增瞳孔扩张参数

        // 用于平滑过渡
        void update(double targetLid, double targetX, double targetY, double dt, double targetDilation = 0.5) {
            // 简单的一阶卡尔曼滤波（这里简化为低通滤波）
            const double smoothFactor = 0.0;  // 平滑系数，值越大越平滑但越滞后

            // 计算速度（增量/时间）
            eyeLidVelocity = (targetLid - eyeLidValue) / dt;
            eyeXVelocity = (targetX - eyeXValue) / dt;
            eyeYVelocity = (targetY - eyeYValue) / dt;

            // 平滑更新位置
            eyeLidValue += (targetLid - eyeLidValue) * (1.0 - smoothFactor);
            eyeXValue += (targetX - eyeXValue) * (1.0 - smoothFactor);
            eyeYValue += (targetY - eyeYValue) * (1.0 - smoothFactor);

            // 平滑更新瞳孔扩张
            pupilDilation += (targetDilation - pupilDilation) * (1.0 - smoothFactor);
        }

        // 线性插值方法
        void interpolate(const EyeState& start, const EyeState& end, float t) {
            eyeLidValue = start.eyeLidValue + t * (end.eyeLidValue - start.eyeLidValue);
            eyeXValue = start.eyeXValue + t * (end.eyeXValue - start.eyeXValue);
            eyeYValue = start.eyeYValue + t * (end.eyeYValue - start.eyeYValue);
            pupilDilation = start.pupilDilation + t * (end.pupilDilation - start.pupilDilation);

            // 速度也可以插值，但通常不需要
            eyeLidVelocity = start.eyeLidVelocity + t * (end.eyeLidVelocity - start.eyeLidVelocity);
            eyeXVelocity = start.eyeXVelocity + t * (end.eyeXVelocity - start.eyeXVelocity);
            eyeYVelocity = start.eyeYVelocity + t * (end.eyeYVelocity - start.eyeYVelocity);
        }
    };
    EyeState targetLeftEyeState, targetRightEyeState;
    EyeState startLeftEyeState, startRightEyeState;
    auto calculateEyeOpenness = [this](double currentValue, int eyeIndex) {
        // 获取完全张开和完全闭合的校准值
        double fullyOpen = eye_fully_open[eyeIndex];
        double fullyClosed = eye_fully_closed[eyeIndex];

        // 确保值位于有效范围内
        if (std::abs(fullyOpen - fullyClosed) < 0.0001) {
            return 1.0; // 防止除以零
        }

        double range = fullyOpen - fullyClosed;
        double normalized = (currentValue - fullyClosed) / range;

        // 限制在0-1范围内
        return max(0.0, min(1.0, normalized));
    };


    // 记录上次发送的值，用于瞳孔扩张计算
    double lastLeftPupilDilation = 0.5;
    double lastRightPupilDilation = 0.5;

    while (is_running())
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        if (0) {
                test_time += 1.0 / 60.0;  // 假设60FPS，每帧增加1/60秒

                TestEyeData testData = generateTestData(test_time);

                // 发送测试数据
                osc_manager->sendModelOutput({testData.eyeLidLeft}, {"/avatar/parameters/v2/EyeLidLeft"});
                osc_manager->sendModelOutput({testData.eyeLidRight}, {"/avatar/parameters/v2/EyeLidRight"});
                osc_manager->sendModelOutput({testData.eyeLeftX}, {"/avatar/parameters/v2/EyeLeftX"});
                osc_manager->sendModelOutput({testData.eyeLeftY}, {"/avatar/parameters/v2/EyeLeftY"});
                osc_manager->sendModelOutput({testData.eyeRightX}, {"/avatar/parameters/v2/EyeRightX"});
                osc_manager->sendModelOutput({testData.eyeRightY}, {"/avatar/parameters/v2/EyeRightY"});
                osc_manager->sendModelOutput({testData.pupilDilation}, {"/avatar/parameters/v2/PupilDilation"});

                // 日志输出（每秒输出一次）
                static int log_counter = 0;
                if (log_counter % 60 == 0) {
                    LOG_INFO("测试模式 - 时间:{:.2f}s, 眼球位置:({:.2f},{:.2f}), 眼睑:{:.2f}, 瞳孔:{:.2f}",
                             test_time, testData.eyeLeftX, testData.eyeLeftY, testData.eyeLidLeft, testData.pupilDilation);
                }
                log_counter++;

                // 控制帧率
                auto end_time = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
                int delay_ms = max(0, frame_interval_ms - static_cast<int>(elapsed));
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                continue;  // 跳过后面的正常处理
            }
        if (is_calibrating) {
            // 发送固定的居中(0,0)位置和0.75开度值
            // 左眼眼睑值固定为0.75
            osc_manager->sendModelOutput(
                { 0.75f },
                { "/avatar/parameters/v2/EyeLidLeft" }
            );

            // 左眼X轴值固定为0.0
            osc_manager->sendModelOutput(
                { 0.0f },
                { "/avatar/parameters/v2/EyeLeftX" }
            );

            // 右眼眼睑值固定为0.75
            osc_manager->sendModelOutput(
                { 0.75f },
                { "/avatar/parameters/v2/EyeLidRight" }
            );

            // 右眼X轴值固定为0.0
            osc_manager->sendModelOutput(
                { 0.0f },
                { "/avatar/parameters/v2/EyeRightX" }
            );

            // Y轴值固定为0.0
            osc_manager->sendModelOutput(
                { 0.0f },
                { "/avatar/parameters/v2/EyeLeftY" }
            );

            osc_manager->sendModelOutput(
                { 0.0f },
                { "/avatar/parameters/v2/EyeRightY" }
            );

            // 瞳孔扩张默认值
            osc_manager->sendModelOutput(
                { 0.5f },
                { "/avatar/parameters/v2/PupilDilation" }
            );

            // 控制循环时间
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            int delay_ms = max(0, frame_interval_ms - static_cast<int>(elapsed));
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            continue;  // 跳过后面的正常处理
        }

        // 是否是真实数据帧（每interpolation_frames+1帧中的第一帧）
        bool is_real_data_frame = (current_interp_frame == 0);

        if (is_real_data_frame) {
            // 保存当前状态作为新的起始状态
            if (hasValidStartState) {
                startLeftEyeState = targetLeftEyeState;
                startRightEyeState = targetRightEyeState;
            }

            // 处理实际数据
            double eye_data[EYE_NUM][4]; // 存储[眼睛开合度,X轴,Y轴,瞳孔扩张]
            bool eye_active[EYE_NUM] = {false, false}; // 标记哪些眼睛有数据

            // 在数据收集部分，使用校准值进行映射:
            for (int i = 0; i < EYE_NUM; i++) {
                double blink_vec;
                double eye_open_value;

                // 获取最新的眼睛状态
                {
                    std::lock_guard<std::mutex> lock_guard(results_mutex[i]);

                    // 检查这只眼睛是否有数据
                    if (pupil[i].x == 0 && pupil[i].y == 0) {
                        continue; // 没有有效数据，跳过此眼睛
                    }

                    // 使用校准值计算眼睛开合度
                    double raw_open = eye_open[i];
                    eye_open_value = calculateEyeOpenness(raw_open, i);

                    blink_vec = min(std::abs(eye_open_value - last_eye_open[i]), 1.0);
                    last_eye_open[i] = eye_open_value;

                    // 标记此眼睛有数据
                    eye_active[i] = true;
                }

                // 存储处理后的结果
                eye_data[i][0] = eye_open_value;
                // 默认输出值
                double out_x = 0.0;
                double out_y = 0.0;
                double pupil_dilation = 0.5; // 默认瞳孔扩张值

                // 按照Python代码的逻辑进行映射
                if (eye_calib_data[i].has_calibration)
                {
                    // 计算差值，避免除零
                    double calib_diff_x_MAX = eye_calib_data[i].calib_XMAX - eye_calib_data[i].calib_XOFF;
                    if (calib_diff_x_MAX == 0) calib_diff_x_MAX = 1;

                    double calib_diff_x_MIN = eye_calib_data[i].calib_XMIN - eye_calib_data[i].calib_XOFF;
                    if (calib_diff_x_MIN == 0) calib_diff_x_MIN = -1;

                    double calib_diff_y_MAX = eye_calib_data[i].calib_YMAX - eye_calib_data[i].calib_YOFF;
                    if (calib_diff_y_MAX == 0) calib_diff_y_MAX = 1;

                    double calib_diff_y_MIN = eye_calib_data[i].calib_YMIN - eye_calib_data[i].calib_YOFF;
                    if (calib_diff_y_MIN == 0) calib_diff_y_MIN = -1;

                    // 计算偏移量
                    double xl = (pupil[i].x - eye_calib_data[i].calib_XOFF) / calib_diff_x_MAX;
                    double xr = (pupil[i].x - eye_calib_data[i].calib_XOFF) / calib_diff_x_MIN;
                    double yu = (pupil[i].y - eye_calib_data[i].calib_YOFF) / calib_diff_y_MIN;
                    double yd = (pupil[i].y - eye_calib_data[i].calib_YOFF) / calib_diff_y_MAX;

                    // Y轴映射，根据flip_y_axis决定方向
                    if (flip_y_axis) {
                        if (yd >= 0)
                            out_y = max(0.0, min(1.0, yd));
                        if (yu > 0)
                            out_y = -abs(max(0.0, min(1.0, yu)));
                    }
                    else {
                        if (yd >= 0)
                            out_y = -abs(max(0.0, min(1.0, yd)));
                        if (yu > 0)
                            out_y = max(0.0, min(1.0, yu));
                    }

                    // X轴映射，根据flip_x_axis[i]决定方向
                    if (flip_x_axis[i]) {
                        if (xr >= 0)
                            out_x = -abs(max(0.0, min(1.0, xr)));
                        if (xl > 0)
                            out_x = max(0.0, min(1.0, xl));
                    }
                    else {
                        if (xr >= 0)
                            out_x = max(0.0, min(1.0, xr));
                        if (xl > 0)
                            out_x = -abs(max(0.0, min(1.0, xl)));
                    }
                    // float compensation_coefficient_y = 0.8, compensation_max = 0.65;
                    // // 向下看补偿：当眼睛向下看时(Y值为负)，增加睁眼值
                    // if (out_y < 0) {
                    //     // 根据向下看的程度逐渐增加补偿
                    //     double down_compensation = min(-out_y * compensation_coefficient_y, compensation_max);
                    //     // 确保补偿后的开合度不超过最大值0.75
                    //     eye_data[i][0] = min(eye_data[i][0] + down_compensation, 1);
                    // }

                    // 添加水平方向补偿：左眼向左看或右眼向右看时增加睁眼值
                    float compensation_coefficient_x = 0.4, compensation_max_x = 0.55; // 可以根据需要调整这些参数
                    if (i == LEFT_TAG && out_x < 0) { // 左眼向左看 (注意坐标系)
                        // 根据向左看的程度逐渐增加补偿
                        double left_compensation = min(-out_x * compensation_coefficient_x, compensation_max_x);
                        // 确保补偿后的开合度不超过最大值1.0
                        eye_data[i][0] = min(eye_data[i][0] + left_compensation, 1);
                    } else if (i == RIGHT_TAG && out_x > 0) { // 右眼向右看 (注意坐标系)
                        // 根据向右看的程度逐渐增加补偿
                        double right_compensation = min(out_x * compensation_coefficient_x, compensation_max_x);
                        // 确保补偿后的开合度不超过最大值1.0
                        eye_data[i][0] = min(eye_data[i][0] + right_compensation, 1);
                    }
                    // 计算瞳孔扩张值 - 基于眼睛开合度的反比例
                    // 眼睛越闭，瞳孔越小；眼睛越开，瞳孔越大
                    pupil_dilation = min(1.0, max(0.3, eye_data[i][0] / 1));
                }
                else
                {
                    // 如果没有校准数据，使用默认瞳孔扩张值
                    pupil_dilation = 0.5;
                }

                // 限制在-1到1范围内
                out_x = max(-1.0, min(1.0, out_x));
                out_y = max(-1.0, min(1.0, out_y));

                eye_data[i][1] = out_x;
                eye_data[i][2] = out_y;
                eye_data[i][3] = pupil_dilation;
            }

            // 第二步：数据共享 - 如果一只眼睛没有数据，使用另一只眼的数据
            if (eye_active[LEFT_TAG] && !eye_active[RIGHT_TAG]) {
                // 右眼没有数据，使用左眼数据
                eye_data[RIGHT_TAG][0] = eye_data[LEFT_TAG][0]; // 眼睛开合度
                eye_data[RIGHT_TAG][1] = -eye_data[LEFT_TAG][1]; // X轴反向
                eye_data[RIGHT_TAG][2] = eye_data[LEFT_TAG][2]; // Y轴
                eye_data[RIGHT_TAG][3] = eye_data[LEFT_TAG][3]; // 瞳孔扩张
                eye_active[RIGHT_TAG] = true;

                // 更新补偿后的值
                {
                    std::lock_guard<std::mutex> lock(compensated_data_mutex[RIGHT_TAG]);
                    compensated_eye_openness[RIGHT_TAG] = eye_data[RIGHT_TAG][0];
                }
            }
            else if (!eye_active[LEFT_TAG] && eye_active[RIGHT_TAG]) {
                // 左眼没有数据，使用右眼数据
                eye_data[LEFT_TAG][0] = eye_data[RIGHT_TAG][0]; // 眼睛开合度
                eye_data[LEFT_TAG][1] = -eye_data[RIGHT_TAG][1]; // X轴反向
                eye_data[LEFT_TAG][2] = eye_data[RIGHT_TAG][2]; // Y轴
                eye_data[LEFT_TAG][3] = eye_data[RIGHT_TAG][3]; // 瞳孔扩张
                eye_active[LEFT_TAG] = true;

                // 更新补偿后的值
                {
                    std::lock_guard<std::mutex> lock(compensated_data_mutex[LEFT_TAG]);
                    compensated_eye_openness[LEFT_TAG] = eye_data[LEFT_TAG][0];
                }
            }

            // *** 新增：第二点五步：眼睛开合度智能平均化处理 ***
            if (eye_active[LEFT_TAG] && eye_active[RIGHT_TAG]) {
                // 当两只眼睛都有数据时，进行智能平均化处理

                // 配置参数
                const double averaging_threshold = 0.25; // 差异阈值，超过此值则不进行平均化
                const double min_averaging_ratio = 0.1;  // 最小平均化比例
                const double max_averaging_ratio = 0.8;  // 最大平均化比例

                // 计算两眼开合度差异
                double left_openness = eye_data[LEFT_TAG][0];
                double right_openness = eye_data[RIGHT_TAG][0];
                double openness_diff = std::abs(left_openness - right_openness);

                // 计算平均化比例：差异越小，平均化程度越高
                double averaging_ratio;
                if (openness_diff >= averaging_threshold) {
                    // 差异过大，不进行平均化（或使用最小平均化比例）
                    averaging_ratio = min_averaging_ratio;
                } else {
                    // 根据差异大小线性插值计算平均化比例
                    // 差异为0时：最大平均化比例
                    // 差异为threshold时：最小平均化比例
                    averaging_ratio = max_averaging_ratio -
                                     (openness_diff / averaging_threshold) *
                                     (max_averaging_ratio - min_averaging_ratio);

                    // 确保在有效范围内
                    averaging_ratio = max(min_averaging_ratio,
                                             min(max_averaging_ratio, averaging_ratio));
                }

                // 计算平均值
                double average_openness = (left_openness + right_openness) / 2.0;

                // 应用平滑平均化：原值 + (平均值 - 原值) * 平均化比例
                double new_left_openness = left_openness + (average_openness - left_openness) * averaging_ratio;
                double new_right_openness = right_openness + (average_openness - right_openness) * averaging_ratio;

                // 更新开合度值
                eye_data[LEFT_TAG][0] = new_left_openness;
                eye_data[RIGHT_TAG][0] = new_right_openness;

                // 更新补偿后的值以供UI显示
                {
                    std::lock_guard<std::mutex> lock_left(compensated_data_mutex[LEFT_TAG]);
                    std::lock_guard<std::mutex> lock_right(compensated_data_mutex[RIGHT_TAG]);
                    compensated_eye_openness[LEFT_TAG] = new_left_openness;
                    compensated_eye_openness[RIGHT_TAG] = new_right_openness;
                }

                // 调试日志（每60帧输出一次，避免日志过多）
                static int averaging_debug_counter = 0;
                if (averaging_debug_counter % 60 == 0) {
                    if (openness_diff >= averaging_threshold) {
                        LOG_DEBUG("眼睛开合度：差异过大({:.3f})，不进行平均化。左眼:{:.3f} 右眼:{:.3f}",
                                 openness_diff, left_openness, right_openness);
                    } else {
                        LOG_DEBUG("眼睛开合度平均化：差异:{:.3f} 比例:{:.2f} 原值(L:{:.3f} R:{:.3f}) 新值(L:{:.3f} R:{:.3f})",
                                 openness_diff, averaging_ratio,
                                 left_openness, right_openness,
                                 new_left_openness, new_right_openness);
                    }
                }
                averaging_debug_counter++;
            }

            // 继续原有的第三步：处理眼睛闭眼值平均化，并检测wink状态
            // 注意：这里的wink检测现在使用的是经过智能平均化处理后的开合度值
            if (eye_active[LEFT_TAG] && eye_active[RIGHT_TAG]) {
                // 根据同步模式处理
                if (eyeSyncMode == LEFT_CONTROLS) {
                    // 左眼控制双眼 - 直接复制左眼开合度到右眼
                    eye_data[RIGHT_TAG][0] = eye_data[LEFT_TAG][0];

                    // 更新补偿后的值
                    {
                        std::lock_guard<std::mutex> lock_right(compensated_data_mutex[RIGHT_TAG]);
                        compensated_eye_openness[RIGHT_TAG] = eye_data[RIGHT_TAG][0];
                    }
                }
                else if (eyeSyncMode == RIGHT_CONTROLS) {
                    // 右眼控制双眼 - 直接复制右眼开合度到左眼
                    eye_data[LEFT_TAG][0] = eye_data[RIGHT_TAG][0];

                    // 更新补偿后的值
                    {
                        std::lock_guard<std::mutex> lock_left(compensated_data_mutex[LEFT_TAG]);
                        compensated_eye_openness[LEFT_TAG] = eye_data[LEFT_TAG][0];
                    }
                }
                else {
                    // 在独立控制模式下，继续检测wink状态
                    const double wink_threshold = 0.3; // 眨眼阈值，可调整
                    const double wink_enhancement = 0.0; // Wink增强系数，闭眼更闭，开眼更开

                    // 左眼和右眼的开合度差距（现在使用经过平均化处理的值）
                    double eye_openness_diff = eye_data[LEFT_TAG][0] - eye_data[RIGHT_TAG][0];

                    if (std::abs(eye_openness_diff) >= wink_threshold) {
                        // 存在wink状态 - 在wink状态下，不应该进行平均化
                        if (eye_openness_diff > 0) {
                            // 左眼更开，右眼在眨眼
                            // 使用左眼数据覆盖右眼的xy坐标
                            eye_data[RIGHT_TAG][1] = -eye_data[LEFT_TAG][1]; // X轴反向
                            eye_data[RIGHT_TAG][2] = eye_data[LEFT_TAG][2];  // Y轴保持不变

                            // 增强wink效果：让左眼更开，右眼更闭
                            eye_data[LEFT_TAG][0] = min(1.0, eye_data[LEFT_TAG][0] + wink_enhancement); // 左眼更开
                            eye_data[RIGHT_TAG][0] = max(0.0, eye_data[RIGHT_TAG][0] - wink_enhancement); // 右眼更闭
                        } else {
                            // 右眼更开，左眼在眨眼
                            // 使用右眼数据覆盖左眼的xy坐标
                            eye_data[LEFT_TAG][1] = -eye_data[RIGHT_TAG][1]; // X轴反向
                            eye_data[LEFT_TAG][2] = eye_data[RIGHT_TAG][2];  // Y轴保持不变

                            // 增强wink效果：让右眼更开，左眼更闭
                            eye_data[RIGHT_TAG][0] = min(1.0, eye_data[RIGHT_TAG][0] + wink_enhancement); // 右眼更开
                            eye_data[LEFT_TAG][0] = max(0.0, eye_data[LEFT_TAG][0] - wink_enhancement); // 左眼更闭
                        }

                        // 在wink状态下更新补偿后的值
                        {
                            std::lock_guard<std::mutex> lock_left(compensated_data_mutex[LEFT_TAG]);
                            std::lock_guard<std::mutex> lock_right(compensated_data_mutex[RIGHT_TAG]);
                            compensated_eye_openness[LEFT_TAG] = eye_data[LEFT_TAG][0];
                            compensated_eye_openness[RIGHT_TAG] = eye_data[RIGHT_TAG][0];
                        }
                    }
                    // 如果不是wink状态，则保持之前智能平均化的结果
                }
            }

            // 第四步：仅在非wink状态下，基于X轴视线方向的权重计算
            if (eye_active[LEFT_TAG] && eye_active[RIGHT_TAG]) {
                // 检查是否是wink状态
                const double wink_threshold = 0.4;
                double eye_openness_diff = eye_data[LEFT_TAG][0] - eye_data[RIGHT_TAG][0];

                // 只在非wink状态下应用权重计算
                if (std::abs(eye_openness_diff) < wink_threshold) {
                    // 取出两只眼睛的X数据来确定视线方向
                    double leftEyeX = eye_data[LEFT_TAG][1];
                    double rightEyeX = -eye_data[RIGHT_TAG][1];  // 注意这里取反，使坐标系一致

                    // 计算平均视线方向
                    double avgGazeX = (leftEyeX + rightEyeX) / 2.0;

                    // 基于视线方向计算权重
                    // 向左看(avgGazeX < 0)，左眼权重增加；向右看(avgGazeX > 0)，右眼权重增加
                    double leftWeight = 0.5 - avgGazeX * 0.25;  // 向左看时增加到0.75，向右看时减少到0.25
                    double rightWeight = 0.5 + avgGazeX * 0.25; // 向右看时增加到0.75，向左看时减少到0.25

                    // 确保权重在合理范围内(0.25-0.75)，避免一只眼睛完全不起作用
                    leftWeight = max(0.25, min(0.75, leftWeight));
                    rightWeight = max(0.25, min(0.75, rightWeight));

                    // 归一化权重，确保总和为1
                    double totalWeight = leftWeight + rightWeight;
                    leftWeight /= totalWeight;
                    rightWeight /= totalWeight;

                    // 应用权重计算加权平均的X值
                    double weightedX = leftEyeX * leftWeight + rightEyeX * rightWeight;

                    // 更新左右眼的X值，但要保持原来的坐标系方向
                    eye_data[LEFT_TAG][1] = weightedX;
                    eye_data[RIGHT_TAG][1] = -weightedX;  // 注意这里再次取反，保持右眼原有的坐标系

                    // Y方向保持原样，不进行权重调整
                }
                // 在wink状态下，不进行权重计算，保持第三步的处理结果
            }

            // 更新目标状态值
            if (eye_active[LEFT_TAG]) {
                targetLeftEyeState.eyeLidValue = eye_data[LEFT_TAG][0];
                targetLeftEyeState.eyeXValue = eye_data[LEFT_TAG][1];
                targetLeftEyeState.eyeYValue = eye_data[LEFT_TAG][2];
                targetLeftEyeState.pupilDilation = eye_data[LEFT_TAG][3];
            }

            if (eye_active[RIGHT_TAG]) {
                targetRightEyeState.eyeLidValue = eye_data[RIGHT_TAG][0];
                targetRightEyeState.eyeXValue = -eye_data[RIGHT_TAG][1];
                targetRightEyeState.eyeYValue = eye_data[RIGHT_TAG][2];
                targetRightEyeState.pupilDilation = eye_data[RIGHT_TAG][3];
            }

            // 如果这是第一帧，初始化起始状态等于目标状态
            if (!hasValidStartState) {
                startLeftEyeState = targetLeftEyeState;
                startRightEyeState = targetRightEyeState;
                hasValidStartState = true;
            }
        }

        // 计算当前插值位置（0.0 - 1.0）
        float t = static_cast<float>(current_interp_frame) / (interpolation_frames + 1);

        // 创建当前插值状态
        EyeState currentLeftEyeState, currentRightEyeState;
        currentLeftEyeState.interpolate(startLeftEyeState, targetLeftEyeState, t);
        currentRightEyeState.interpolate(startRightEyeState, targetRightEyeState, t);

        // 左眼数据发送
        osc_manager->sendModelOutput(
            { static_cast<float>(currentLeftEyeState.eyeLidValue) },
            { "/avatar/parameters/v2/EyeLidLeft" }
        );

        osc_manager->sendModelOutput(
            { static_cast<float>(currentLeftEyeState.eyeXValue) },
            { "/avatar/parameters/v2/EyeLeftX" }
        );

        osc_manager->sendModelOutput(
            { static_cast<float>(currentLeftEyeState.eyeYValue) },
            { "/avatar/parameters/v2/EyeLeftY" }
        );

        lastLeftPupilDilation = currentLeftEyeState.pupilDilation;

        // 右眼数据发送
        osc_manager->sendModelOutput(
            { static_cast<float>(currentRightEyeState.eyeLidValue) },
            { "/avatar/parameters/v2/EyeLidRight" }
        );

        osc_manager->sendModelOutput(
            { static_cast<float>(currentRightEyeState.eyeXValue) },
            { "/avatar/parameters/v2/EyeRightX" }
        );

        osc_manager->sendModelOutput(
            { static_cast<float>(currentRightEyeState.eyeYValue) },
            { "/avatar/parameters/v2/EyeRightY" }
        );

        lastRightPupilDilation = currentRightEyeState.pupilDilation;

        // 发送瞳孔扩张数据（使用两眼平均值）
        float pupilDilationValue = (lastLeftPupilDilation + lastRightPupilDilation) / 2.0f;
        osc_manager->sendModelOutput(
            { pupilDilationValue },
            { "/avatar/parameters/v2/PupilDilation" }
        );

        debug_counter++;

        // 更新眼睛位置显示
        updateEyePosition(LEFT_TAG);
        updateEyePosition(RIGHT_TAG);

        // 更新插帧计数器
        current_interp_frame = (current_interp_frame + 1) % (interpolation_frames + 1);

        // 控制循环时间，确保60Hz的发送帧率
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        int delay_ms = max(0, frame_interval_ms - static_cast<int>(elapsed));
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }
});
}

PaperEyeTrackerWindow::~PaperEyeTrackerWindow() {
    LOG_INFO("正在关闭系统...");
    instance = nullptr;
    app_is_running = false;
    if (auto_save_timer) {
        auto_save_timer->stop();
        delete auto_save_timer;
        auto_save_timer = nullptr;
    }
    // join threads
    for (int i = 0; i < EYE_NUM; i++) {
        if (update_ui_thread[i].joinable()) {
            update_ui_thread[i].join();
        }
        if (inference_thread[i].joinable()) {
            inference_thread[i].join();
        }
        if (image_stream[i]->isStreaming()) {
            image_stream[i]->stop();
        }
    }
    if (osc_send_thread.joinable()) {
        osc_send_thread.join();
    }
    // clean resources
    if (serial_port_->status() == SerialStatus::OPENED) {
        serial_port_->stop();
    }
    osc_manager->close();
    config = generate_config();
    config_writer->write_config(config);
    LOG_INFO("系统已安全关闭");
    remove_log_window(ui.LogText);
    instance = nullptr;
}

void PaperEyeTrackerWindow::onSendButtonClicked() {
    auto ssid = getSSID();
    auto password = getPassword();

    // 输入验证
    if (ssid == "请输入WIFI名字（仅支持2.4ghz）" || ssid.empty()) {
        QMessageBox::warning(this, tr("输入错误"), tr("请输入有效的WIFI名字"));
        return;
    }

    if (password == "请输入WIFI密码" || password.empty()) {
        QMessageBox::warning(this, tr("输入错误"), tr("请输入有效的密码"));
        return;
    }

    // 构建并发送数据包
    LOG_INFO("已发送WiFi配置: SSID = {}, PWD = {}", ssid, password);
    LOG_INFO("等待数据被发送后开始自动重启ESP32...");
    serial_port_->sendWiFiConfig(ssid, password);

    QTimer::singleShot(3000, this, [this] {
        // 3秒后自动重启ESP32
        onRestartButtonClicked();
        });
}

void PaperEyeTrackerWindow::onRestartButtonClicked() {
    if (current_esp32_version == FACE_VERSION) {
        QMessageBox::information(this, "错误", "插入的是面捕设备，请到面捕界面操作");
        return;
    }
    serial_port_->stop_heartbeat_timer();
    for (int i = 0; i < EYE_NUM; i++) {
        image_stream[i]->stop_heartbeat_timer();
    }
    serial_port_->restartESP32(this);
    serial_port_->start_heartbeat_timer();
    for (int i = 0; i < EYE_NUM; i++) {
        image_stream[i]->stop();
        image_stream[i]->start();
        image_stream[i]->start_heartbeat_timer();
    }
}

void PaperEyeTrackerWindow::onFlashButtonClicked() {
    if (current_esp32_version == FACE_VERSION) {
        QMessageBox::information(this, "错误", "插入的是面捕设备，请到面捕界面操作");
        return;
    }

    // 弹出固件选择对话框
    QStringList firmwareTypes;
    firmwareTypes << "左眼固件 (left_eye.bin)" << "右眼固件 (right_eye.bin)"
                  << "轻薄板左眼固件 (light_left_eye.bin)" << "轻薄板右眼固件 (light_right_eye.bin)"
                  << "面捕固件 (face_tracker.bin)";

    bool ok;
    QString selectedType = QInputDialog::getItem(this, "选择固件类型",
        "请选择要烧录的固件类型:",
        firmwareTypes, 0, false, &ok);
    if (!ok || selectedType.isEmpty()) {
        // 用户取消操作
        return;
    }

    // 修改为:
    std::string firmwareType;
    if (selectedType.contains("左眼固件") && !selectedType.contains("轻薄板")) {
        firmwareType = "left_eye";
    }
    else if (selectedType.contains("右眼固件") && !selectedType.contains("轻薄板")) {
        firmwareType = "right_eye";
    }
    else if (selectedType.contains("轻薄板左眼固件")) {
        firmwareType = "light_left_eye";
    }
    else if (selectedType.contains("轻薄板右眼固件")) {
        firmwareType = "light_right_eye";
    }
    else {
        firmwareType = "face_tracker";
    }

    LOG_INFO("用户选择烧录固件类型: {}", firmwareType);

    serial_port_->stop_heartbeat_timer();
    for (int i = 0; i < EYE_NUM; i++) {
        image_stream[i]->stop_heartbeat_timer();
    }

    // 传递选择的固件类型
    serial_port_->flashESP32(this, firmwareType);

    serial_port_->start_heartbeat_timer();
    for (int i = 0; i < EYE_NUM; i++) {
        image_stream[i]->stop();
        image_stream[i]->start();
        image_stream[i]->start_heartbeat_timer();
    }
}

void PaperEyeTrackerWindow::retranslateUI() {
    ui.LeftEyeTrackingLabel->setText(QApplication::translate("PaperTrackerMainWindow", "左眼跟踪"));
    ui.RightEyeTrackingLabel->setText(QApplication::translate("PaperTrackerMainWindow", "右眼跟踪"));
    ui.MainPageButton->setText(QApplication::translate("PaperTrackerMainWindow", "主页面"));
    ui.SettingButton->setText(QApplication::translate("PaperTrackerMainWindow", "设置"));
    ui.settingsCalibrateButton->setText(QApplication::translate("PaperTrackerMainWindow", "开始眼球位置校准"));
    ui.settingsCenterButton->setText(QApplication::translate("PaperTrackerMainWindow", "标定眼球中心"));
    ui.settingsEyeOpenButton->setText(QApplication::translate("PaperTrackerMainWindow", "标定眼睛完全张开"));
    ui.settingsEyeCloseButton->setText(QApplication::translate("PaperTrackerMainWindow", "标定眼睛完全闭合"));
    ui.LeftEyeOpennessLabel->setText(QApplication::translate("PaperTrackerMainWindow", "左眼开合度"));
    ui.RightEyeOpennessLabel->setText(QApplication::translate("PaperTrackerMainWindow", "右眼开合度"));
    ui.LeftEyeImage->setText(QApplication::translate("PaperTrackerMainWindow", "没有图像输入"));
    ui.RightEyeImage->setText(QApplication::translate("PaperTrackerMainWindow", "没有图像输入"));
    ui.label_4->setText(QApplication::translate("PaperTrackerMainWindow", "左眼补光"));
    ui.label_5->setText(QApplication::translate("PaperTrackerMainWindow", "右眼补光"));
    ui.LeftRotateLabel->setText(QApplication::translate("PaperTrackerMainWindow", "左眼旋转角度"));
    ui.RightRotateLabel->setText(QApplication::translate("PaperTrackerMainWindow", "右眼旋转角度"));
    ui.SendButton->setText(QApplication::translate("PaperTrackerMainWindow", "发送"));
    ui.RestartButton->setText(QApplication::translate("PaperTrackerMainWindow", "重启"));
    ui.FlashButton->setText(QApplication::translate("PaperTrackerMainWindow", "刷写固件"));
    ui.label_3->setText(QApplication::translate("PaperTrackerMainWindow", "模式选择"));
    ui.label->setText(QApplication::translate("PaperTrackerMainWindow", "左眼IP"));
    ui.label_2->setText(QApplication::translate("PaperTrackerMainWindow", "右眼IP"));
    ui.EnergyModelBox->setItemText(0, QApplication::translate("PaperTrackerMainWindow", "普通模式"));
    ui.EnergyModelBox->setItemText(1, QApplication::translate("PaperTrackerMainWindow", "节能模式"));
    ui.EnergyModelBox->setItemText(2, QApplication::translate("PaperTrackerMainWindow", "性能模式"));

    eyeSyncComboBox->setItemText(0, QApplication::translate("PaperTrackerMainWindow", "双眼眼皮独立控制"));
    eyeSyncComboBox->setItemText(1, QApplication::translate("PaperTrackerMainWindow", "左眼眼皮控制双眼眼皮"));
    eyeSyncComboBox->setItemText(2, QApplication::translate("PaperTrackerMainWindow", "右眼眼皮控制双眼眼皮"));
}

void PaperEyeTrackerWindow::connect_callbacks() {
    for (int i = 0; i < EYE_NUM; i++) {
        brightness_timer[i] = std::make_shared<QTimer>();
        brightness_timer[i]->setSingleShot(true);
        connect(brightness_timer[i].get(), &QTimer::timeout, this, &PaperEyeTrackerWindow::onSendBrightnessValue);
    }
    connect(ui.SendButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::onSendButtonClicked);
    connect(ui.RestartButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::onRestartButtonClicked);
    connect(ui.FlashButton, &QPushButton::clicked, this, &PaperEyeTrackerWindow::onFlashButtonClicked);
    connect(ui.EnergyModelBox, &QComboBox::currentIndexChanged, this, &PaperEyeTrackerWindow::onEnergyModeChanged);
    connect(ui.LeftBrightnessBar, &QScrollBar::valueChanged, this, &PaperEyeTrackerWindow::onLeftBrightnessChanged);
    connect(ui.RightBrightnessBar, &QScrollBar::valueChanged, this, &PaperEyeTrackerWindow::onRightBrightnessChanged);
    connect(ui.LeftRotateBar, &QScrollBar::valueChanged, this, &PaperEyeTrackerWindow::onLeftRotateAngleChanged);
    connect(ui.RightRotateBar, &QScrollBar::valueChanged, this, &PaperEyeTrackerWindow::onRightRotateAngleChanged);
}

void PaperEyeTrackerWindow::onLeftBrightnessChanged(int value) {
    brightness[LEFT_TAG] = value;
    // 更新值后可以安全返回，即使系统未准备好也先保存用户设置
    if (!serial_port_ || !brightness_timer[LEFT_TAG]) {
        return;
    }

    // 只在设备正确连接状态下才发送命令
    if (serial_port_->status() == SerialStatus::OPENED && current_esp32_version == LEFT_VERSION) {
        brightness_timer[LEFT_TAG]->start();
    }
    else {
        QMessageBox::warning(this, "警告", "左眼设备未连接，请先连接设备");
    }

}

void PaperEyeTrackerWindow::onRightBrightnessChanged(int value) {
    brightness[RIGHT_TAG] = value;
    brightness_timer[RIGHT_TAG]->start();
    // 更新值后可以安全返回，即使系统未准备好也先保存用户设置
    if (!serial_port_ || !brightness_timer[RIGHT_TAG]) {
        return;
    }

    // 只在设备正确连接状态下才发送命令
    if (serial_port_->status() == SerialStatus::OPENED && current_esp32_version == RIGHT_VERSION) {
        brightness_timer[RIGHT_TAG]->start();
    }
    else {
        QMessageBox::warning(this, "警告", "右眼设备未连接，请先连接设备");
    }

}

void PaperEyeTrackerWindow::onSendBrightnessValue() {
    if (serial_port_->status() != SerialStatus::OPENED || current_esp32_version == 0) {
        //QMessageBox::warning(this, "修改失败", "请等待设备响应后调整亮度");
        return;
    }
    // 发送亮度控制命令 - 确保亮度值为三位数字
    std::string brightness_str = std::to_string(brightness[current_esp32_version - 2]);
    // 补齐三位数字，前面加0
    while (brightness_str.length() < 3) {
        brightness_str = std::string("0") + brightness_str;
    }
    std::string packet = "A6" + brightness_str + "B6";
    serial_port_->write_data(packet);
    // 记录操作
    LOG_INFO("已设置亮度: {}", brightness_str);
}

std::string PaperEyeTrackerWindow::getSSID() const {
    return ui.SSIDInput->toPlainText().toStdString();
}

std::string PaperEyeTrackerWindow::getPassword() const {
    return ui.PassWordInput->toPlainText().toStdString();
}

int PaperEyeTrackerWindow::get_max_fps() const {
    return max_fps;
}

bool PaperEyeTrackerWindow::is_running() const {
    return app_is_running;
}

void PaperEyeTrackerWindow::set_config() {
    ui.RightEyeIPAddress->setPlainText(QString::fromStdString(config.right_ip));
    ui.LeftEyeIPAddress->setPlainText(QString::fromStdString(config.left_ip));
    ui.LeftBrightnessBar->setValue(config.left_brightness);
    ui.RightBrightnessBar->setValue(config.right_brightness);
    ui.EnergyModelBox->setCurrentIndex(config.energy_mode);
    roi_rect[LEFT_TAG] = config.left_roi;
    roi_rect[RIGHT_TAG] = config.right_roi;

    // 加载校准数据
    eye_calib_data[LEFT_TAG].calib_XMIN = config.left_calib_XMIN;
    eye_calib_data[LEFT_TAG].calib_XMAX = config.left_calib_XMAX;
    eye_calib_data[LEFT_TAG].calib_YMIN = config.left_calib_YMIN;
    eye_calib_data[LEFT_TAG].calib_YMAX = config.left_calib_YMAX;
    eye_calib_data[LEFT_TAG].calib_XOFF = config.left_calib_XOFF;
    eye_calib_data[LEFT_TAG].calib_YOFF = config.left_calib_YOFF;
    eye_calib_data[LEFT_TAG].has_calibration = config.left_has_calibration;

    eye_calib_data[RIGHT_TAG].calib_XMIN = config.right_calib_XMIN;
    eye_calib_data[RIGHT_TAG].calib_XMAX = config.right_calib_XMAX;
    eye_calib_data[RIGHT_TAG].calib_YMIN = config.right_calib_YMIN;
    eye_calib_data[RIGHT_TAG].calib_YMAX = config.right_calib_YMAX;
    eye_calib_data[RIGHT_TAG].calib_XOFF = config.right_calib_XOFF;
    eye_calib_data[RIGHT_TAG].calib_YOFF = config.right_calib_YOFF;
    eye_calib_data[RIGHT_TAG].has_calibration = config.right_has_calibration;

    // 加载轴翻转设置
    flip_x_axis[LEFT_TAG] = config.left_flip_x;
    flip_x_axis[RIGHT_TAG] = config.right_flip_x;
    flip_y_axis = config.flip_y;

    // 加载旋转角度
    ui.LeftRotateBar->setValue(config.left_rotate_angle);
    ui.RightRotateBar->setValue(config.right_rotate_angle);
    current_rotate_angle[LEFT_TAG] = config.left_rotate_angle;
    current_rotate_angle[RIGHT_TAG] = config.right_rotate_angle;
    // 加载眼睛开合度校准数据
    eye_fully_open[LEFT_TAG] = config.left_eye_fully_open;
    eye_fully_closed[LEFT_TAG] = config.left_eye_fully_closed;
    eye_fully_open[RIGHT_TAG] = config.right_eye_fully_open;
    eye_fully_closed[RIGHT_TAG] = config.right_eye_fully_closed;
    // 设置页面上旋转条也需要更新
    if (ui.page_2) {
        QScrollBar* leftRotateBar = ui.page_2->findChild<QScrollBar*>("settingLeftRotateBar");
        QScrollBar* rightRotateBar = ui.page_2->findChild<QScrollBar*>("settingRightRotateBar");

        if (leftRotateBar) leftRotateBar->setValue(config.left_rotate_angle);
        if (rightRotateBar) rightRotateBar->setValue(config.right_rotate_angle);
    }
    if (eyeSyncComboBox) {
        eyeSyncComboBox->setCurrentIndex(config.eye_sync_mode);
        eyeSyncMode = static_cast<EyeSyncMode>(config.eye_sync_mode);
    }
}

void PaperEyeTrackerWindow::setIPText(int version, const QString& text) const {
    if (version == LEFT_TAG) {
        ui.LeftEyeIPAddress->setPlainText(tr(text.toUtf8().constData()));
    }
    else {
        ui.RightEyeIPAddress->setPlainText(tr(text.toUtf8().constData()));
    }
}


void PaperEyeTrackerWindow::start_image_download(int version) const {
    for (int i = 0; i < EYE_NUM; i++) {
        if (image_stream[i]->isStreaming()) {
            image_stream[i]->stop();
        }

        // 检查URL是否为空
        const std::string& url = current_ip[i];
        if (url.empty()) {
            LOG_INFO("跳过眼睛 {} 的WebSocket连接：URL为空", i == LEFT_TAG ? "左" : "右");
            continue;  // 跳过空URL的连接初始化
        }
        int deviceType = (i == LEFT_TAG) ? DEVICE_TYPE_LEFT_EYE : DEVICE_TYPE_RIGHT_EYE;
        // 开始下载图片 - 修改为支持WebSocket协议
        // 检查URL格式
        if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://" ||
            url.substr(0, 5) == "ws://" || url.substr(0, 6) == "wss://") {
            // URL已经包含协议前缀，直接使用
            image_stream[i]->init(url, deviceType);
            }
        else {
            // 添加默认ws://前缀
            image_stream[i]->init("ws://" + url, deviceType);
        }
        image_stream[i]->start();
    }
}

void PaperEyeTrackerWindow::updateWifiLabel(int version) const {
    if (image_stream[version]->isStreaming()) {
        setWifiStatusLabel(version, "Wifi已连接");
    }
    else {
        setWifiStatusLabel(version, "Wifi连接失败");
    }
}

void PaperEyeTrackerWindow::updateSerialLabel(int version) const {
    if (serial_port_->status() == SerialStatus::OPENED) {
        if (version == LEFT_VERSION) {
            setSerialStatusLabel("左眼设备已连接");
        }
        else if (version == RIGHT_VERSION) {
            setSerialStatusLabel("右眼设备已连接");
        }
    }
    else {
        setSerialStatusLabel("没有设备连接");
    }
}

cv::Mat PaperEyeTrackerWindow::getVideoImage(int version) const {
    return std::move(image_stream[version]->getLatestFrame());
}

void PaperEyeTrackerWindow::setSerialStatusLabel(const QString& text) const {
    ui.EyeWindowSerialStatus->setText(QApplication::translate("PaperTrackerMainWindow",text.toUtf8().constData()));
}

void PaperEyeTrackerWindow::setWifiStatusLabel(int version, const QString& text) const {
    if (version == LEFT_TAG) {
        ui.LeftEyeWifiStatus->setText(QApplication::translate("PaperTrackerMainWindow",text.toUtf8().constData()));
    }
    else if (version == RIGHT_TAG) {
        ui.RightEyeWifiStatus->setText(QApplication::translate("PaperTrackerMainWindow",text.toUtf8().constData()));
    }
}

PaperEyeTrackerConfig PaperEyeTrackerWindow::generate_config() const {
    PaperEyeTrackerConfig res_config;
    res_config.left_ip = ui.LeftEyeIPAddress->toPlainText().toStdString();
    res_config.right_ip = ui.RightEyeIPAddress->toPlainText().toStdString();
    res_config.left_brightness = brightness[LEFT_TAG];
    res_config.right_brightness = brightness[RIGHT_TAG];
    res_config.energy_mode = ui.EnergyModelBox->currentIndex();
    res_config.left_roi = roi_rect[LEFT_TAG];
    res_config.right_roi = roi_rect[RIGHT_TAG];

    // 保存校准数据
    res_config.left_calib_XMIN = eye_calib_data[LEFT_TAG].calib_XMIN;
    res_config.left_calib_XMAX = eye_calib_data[LEFT_TAG].calib_XMAX;
    res_config.left_calib_YMIN = eye_calib_data[LEFT_TAG].calib_YMIN;
    res_config.left_calib_YMAX = eye_calib_data[LEFT_TAG].calib_YMAX;
    res_config.left_calib_XOFF = eye_calib_data[LEFT_TAG].calib_XOFF;
    res_config.left_calib_YOFF = eye_calib_data[LEFT_TAG].calib_YOFF;
    res_config.left_has_calibration = eye_calib_data[LEFT_TAG].has_calibration;

    res_config.right_calib_XMIN = eye_calib_data[RIGHT_TAG].calib_XMIN;
    res_config.right_calib_XMAX = eye_calib_data[RIGHT_TAG].calib_XMAX;
    res_config.right_calib_YMIN = eye_calib_data[RIGHT_TAG].calib_YMIN;
    res_config.right_calib_YMAX = eye_calib_data[RIGHT_TAG].calib_YMAX;
    res_config.right_calib_XOFF = eye_calib_data[RIGHT_TAG].calib_XOFF;
    res_config.right_calib_YOFF = eye_calib_data[RIGHT_TAG].calib_YOFF;
    res_config.right_has_calibration = eye_calib_data[RIGHT_TAG].has_calibration;
    // 保存眼睛开合度校准数据
    res_config.left_eye_fully_open = eye_fully_open[LEFT_TAG];
    res_config.left_eye_fully_closed = eye_fully_closed[LEFT_TAG];
    res_config.right_eye_fully_open = eye_fully_open[RIGHT_TAG];
    res_config.right_eye_fully_closed = eye_fully_closed[RIGHT_TAG];
    // 保存旋转角度
    res_config.left_rotate_angle = current_rotate_angle[LEFT_TAG];
    res_config.right_rotate_angle = current_rotate_angle[RIGHT_TAG];

    // 保存轴翻转设置
    res_config.left_flip_x = flip_x_axis[LEFT_TAG];
    res_config.right_flip_x = flip_x_axis[RIGHT_TAG];
    res_config.flip_y = flip_y_axis;
    res_config.eye_sync_mode = eyeSyncMode;
    return res_config;
}

void PaperEyeTrackerWindow::onEnergyModeChanged(int index) {
    if (index == 0) {
        max_fps = 38;
    }
    else if (index == 1) {
        max_fps = 15;
    }
    else if (index == 2) {
        max_fps = 70;
    }
}

void PaperEyeTrackerWindow::bound_pages() {
    // 页面导航逻辑
    connect(ui.MainPageButton, &QPushButton::clicked, [this] {
        ui.stackedWidget->setCurrentIndex(0);
        });
    connect(ui.SettingButton, &QPushButton::clicked, [this] {
        ui.stackedWidget->setCurrentIndex(1);
        });
}

float PaperEyeTrackerWindow::getRotateAngle(int version) const {
    // 将0-1080范围转换为0-360度
    auto rotate_angle = static_cast<float>(current_rotate_angle[version]);
    rotate_angle = rotate_angle / 1080.0f * 360.0f;
    return rotate_angle;
}

Rect PaperEyeTrackerWindow::getRoiRect(int version) {
    return roi_rect[version];
}
void PaperEyeTrackerWindow::startCalibration() {
    if (is_calibrating) {
        LOG_INFO("已有校准进行中，请等待当前校准完成");
        QMessageBox::information(this, "校准进行中", "请等待当前校准完成后再开始新的校准");
        return;
    }

    // 检查哪些眼睛连接了
    bool left_connected = !current_ip[LEFT_TAG].empty() && image_stream[LEFT_TAG]->isStreaming();
    bool right_connected = !current_ip[RIGHT_TAG].empty() && image_stream[RIGHT_TAG]->isStreaming();

    if (!left_connected && !right_connected) {
        LOG_INFO("没有检测到连接的眼睛");
        QMessageBox::information(this, "没有连接", "没有检测到任何连接的眼睛，请先连接眼睛设备");
        return;
    }

    // 记录要校准的眼睛
    std::vector<int> eyes_to_calibrate;
    if (left_connected) {
        eyes_to_calibrate.push_back(LEFT_TAG);
        LOG_INFO("将校准左眼位置");
    }
    if (right_connected) {
        eyes_to_calibrate.push_back(RIGHT_TAG);
        LOG_INFO("将校准右眼位置");
    }

    // 初始化校准数据 - 只初始化位置相关数据
    for (int eye_idx : eyes_to_calibrate) {
        eye_calib_data[eye_idx].calib_XMIN = 9999999;
        eye_calib_data[eye_idx].calib_XMAX = -9999999;
        eye_calib_data[eye_idx].calib_YMIN = 9999999;
        eye_calib_data[eye_idx].calib_YMAX = -9999999;
        eye_calib_data[eye_idx].has_calibration = false;
    }

    // 开始校准过程
    is_calibrating = true;
    calibration_frame_count = 0;

    // 提示用户
    //LOG_INFO("位置校准开始：请在10秒内环顾四周，尽量转动眼球覆盖最大视野范围。");

    // 使用 QTimer 延迟执行结束校准流程
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this, eyes_to_calibrate, timer]() {
    bool was_calibrating = is_calibrating;
    is_calibrating = false;

    // 为每只校准的眼睛检查状态
    for (int eye_idx : eyes_to_calibrate) {
        // 确保有足够的校准数据
        if (pupil[eye_idx].x > 0 && pupil[eye_idx].y > 0) {
            eye_calib_data[eye_idx].has_calibration = true;

            // 只有在之前仍处于校准状态时才输出时间限制信息
            if (was_calibrating) {
                LOG_INFO("眼睛{}位置校准完成：时间限制到达 (已收集 {} 个样本点, 校准时限 20 秒)",
                    eye_idx == LEFT_TAG ? "左" : "右",
                    open_list[eye_idx].size());
            }

            LOG_INFO("眼睛{}位置校准数据: X范围({:.1f}-{:.1f}), Y范围({:.1f}-{:.1f}), 中心({:.1f}, {:.1f})",
                eye_idx == LEFT_TAG ? "左" : "右",
                eye_calib_data[eye_idx].calib_XMIN,
                eye_calib_data[eye_idx].calib_XMAX,
                eye_calib_data[eye_idx].calib_YMIN,
                eye_calib_data[eye_idx].calib_YMAX,
                eye_calib_data[eye_idx].calib_XOFF,
                eye_calib_data[eye_idx].calib_YOFF);
        } else {
            LOG_WARN("眼睛{}位置校准失败: 未收集到足够数据", eye_idx == LEFT_TAG ? "左" : "右");
        }
    }

    if (was_calibrating) {
        LOG_INFO("位置校准过程结束：时间限制到达");
    }

    timer->deleteLater();
});

    timer->setSingleShot(true);
    timer->start(5000); // 5秒后完成校准
    calibrated_position = true;
    updateCalibrationButtonStates();
}

void PaperEyeTrackerWindow::centerCalibration() {
    if (is_calibrating) {
        LOG_INFO("校准进行中，请等待校准完成后再标定中心点");
        QMessageBox::information(this, "校准进行中", "请等待当前校准完成后再标定中心点");
        return;
    }

    // 检查哪些眼睛连接了
    bool left_connected = !current_ip[LEFT_TAG].empty() && image_stream[LEFT_TAG]->isStreaming();
    bool right_connected = !current_ip[RIGHT_TAG].empty() && image_stream[RIGHT_TAG]->isStreaming();

    if (!left_connected && !right_connected) {
        LOG_INFO("没有检测到连接的眼睛");
        QMessageBox::information(this, "没有连接", "没有检测到任何连接的眼睛，请先连接眼睛设备");
        return;
    }

    LOG_INFO("开始标定中心点...");
    if (left_connected) LOG_INFO("左眼中心标定中...");
    if (right_connected) LOG_INFO("右眼中心标定中...");

    // 延迟1秒执行，给用户时间调整视线
    QTimer::singleShot(100, this, [this, left_connected, right_connected]() {
        // 标定左眼中心
        if (left_connected) {
            std::lock_guard<std::mutex> lock(results_mutex[LEFT_TAG]);
            eye_calib_data[LEFT_TAG].calib_XOFF = pupil[LEFT_TAG].x;
            eye_calib_data[LEFT_TAG].calib_YOFF = pupil[LEFT_TAG].y;

            // 如果之前没有校准过，设置默认范围
            if (!eye_calib_data[LEFT_TAG].has_calibration) {
                double width = roi_rect[LEFT_TAG].width;
                double height = roi_rect[LEFT_TAG].height;
                eye_calib_data[LEFT_TAG].calib_XMIN = pupil[LEFT_TAG].x - width/4;
                eye_calib_data[LEFT_TAG].calib_XMAX = pupil[LEFT_TAG].x + width/4;
                eye_calib_data[LEFT_TAG].calib_YMIN = pupil[LEFT_TAG].y - height/4;
                eye_calib_data[LEFT_TAG].calib_YMAX = pupil[LEFT_TAG].y + height/4;
                eye_calib_data[LEFT_TAG].has_calibration = true;
            }

            LOG_INFO("左眼中心点标定完成: 中心({:.1f}, {:.1f})",
                eye_calib_data[LEFT_TAG].calib_XOFF,
                eye_calib_data[LEFT_TAG].calib_YOFF);
        }

        // 标定右眼中心
        if (right_connected) {
            std::lock_guard<std::mutex> lock(results_mutex[RIGHT_TAG]);
            eye_calib_data[RIGHT_TAG].calib_XOFF = pupil[RIGHT_TAG].x;
            eye_calib_data[RIGHT_TAG].calib_YOFF = pupil[RIGHT_TAG].y;

            // 如果之前没有校准过，设置默认范围
            if (!eye_calib_data[RIGHT_TAG].has_calibration) {
                double width = roi_rect[RIGHT_TAG].width;
                double height = roi_rect[RIGHT_TAG].height;
                eye_calib_data[RIGHT_TAG].calib_XMIN = pupil[RIGHT_TAG].x - width/4;
                eye_calib_data[RIGHT_TAG].calib_XMAX = pupil[RIGHT_TAG].x + width/4;
                eye_calib_data[RIGHT_TAG].calib_YMIN = pupil[RIGHT_TAG].y - height/4;
                eye_calib_data[RIGHT_TAG].calib_YMAX = pupil[RIGHT_TAG].y + height/4;
                eye_calib_data[RIGHT_TAG].has_calibration = true;
            }

            LOG_INFO("右眼中心点标定完成: 中心({:.1f}, {:.1f})",
                eye_calib_data[RIGHT_TAG].calib_XOFF,
                eye_calib_data[RIGHT_TAG].calib_YOFF);
        }
        calibrated_center = true;
        updateCalibrationButtonStates();
        LOG_INFO("中心点标定完成！");
    });
}
void PaperEyeTrackerWindow::onLeftRotateAngleChanged(int value) {
    current_rotate_angle[LEFT_TAG] = value;
}

void PaperEyeTrackerWindow::onRightRotateAngleChanged(int value) {
    current_rotate_angle[RIGHT_TAG] = value;
}
void PaperEyeTrackerWindow::calibrateEye(int eyeIndex) {
    std::string eyeName = (eyeIndex == LEFT_TAG) ? "左眼" : "右眼";
    LOG_INFO("开始校准{}...", eyeName);

    // 播放开始音效
    //startSound->play();

    // 初始化校准数据
    eye_calib_data[eyeIndex].calib_XMIN = 9999999;
    eye_calib_data[eyeIndex].calib_XMAX = -9999999;
    eye_calib_data[eyeIndex].calib_YMIN = 9999999;
    eye_calib_data[eyeIndex].calib_YMAX = -9999999;
    eye_calib_data[eyeIndex].has_calibration = false;

    // 开始校准过程
    is_calibrating = true;
    calibration_frame_count = 0;

    // 提示用户
    LOG_INFO("校准开始：请在10秒内环顾四周，尽量转动眼球覆盖最大视野范围。");

    // 使用 QTimer 延迟执行结束校准流程
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this, eyeIndex, timer, eyeName]() {
        is_calibrating = false;

        // 播放结束音效
        //endSound->play();

        // 设置校准完成标志
        eye_calib_data[eyeIndex].has_calibration = true;

        LOG_INFO("{}校准完成: X范围({:.1f}-{:.1f}), Y范围({:.1f}-{:.1f}), 中心({:.1f}, {:.1f})",
            eyeName,
            eye_calib_data[eyeIndex].calib_XMIN,
            eye_calib_data[eyeIndex].calib_XMAX,
            eye_calib_data[eyeIndex].calib_YMIN,
            eye_calib_data[eyeIndex].calib_YMAX,
            eye_calib_data[eyeIndex].calib_XOFF,
            eye_calib_data[eyeIndex].calib_YOFF);

        LOG_INFO("校准完成！");
        timer->deleteLater();
        });

    timer->setSingleShot(true);
    timer->start(3000); // 10秒后完成校准
}

void PaperEyeTrackerWindow::centerEye(int eyeIndex) {
    std::string eyeName = (eyeIndex == LEFT_TAG) ? "左眼" : "右眼";
    LOG_INFO("标定{}中心点...", eyeName);

    // 延迟1秒执行，给用户时间调整视线
    QTimer::singleShot(1000, this, [this, eyeIndex, eyeName]() {
        // 获取当前眼睛位置作为中心点
        std::lock_guard<std::mutex> lock(results_mutex[eyeIndex]);
        eye_calib_data[eyeIndex].calib_XOFF = pupil[eyeIndex].x;
        eye_calib_data[eyeIndex].calib_YOFF = pupil[eyeIndex].y;

        // 如果之前没有校准过，设置默认范围
        if (!eye_calib_data[eyeIndex].has_calibration) {
            double width = roi_rect[eyeIndex].width;
            double height = roi_rect[eyeIndex].height;
            eye_calib_data[eyeIndex].calib_XMIN = pupil[eyeIndex].x - width / 4;
            eye_calib_data[eyeIndex].calib_XMAX = pupil[eyeIndex].x + width / 4;
            eye_calib_data[eyeIndex].calib_YMIN = pupil[eyeIndex].y - height / 4;
            eye_calib_data[eyeIndex].calib_YMAX = pupil[eyeIndex].y + height / 4;
            eye_calib_data[eyeIndex].has_calibration = true;
        }

        LOG_INFO("{}中心点标定完成: 中心({:.1f}, {:.1f})",
            eyeName,
            eye_calib_data[eyeIndex].calib_XOFF,
            eye_calib_data[eyeIndex].calib_YOFF);
        });
}
void PaperEyeTrackerWindow::updateEyePosition(int eyeIndex)
{
    // 这个函数会被 osc_send_thread 线程调用，用来更新眼睛位置显示
    if (!app_is_running)
        return;

    // 获取当前眼球坐标和开合度
    double eyeX = 0.5, eyeY = 0.5, opennessPct = 0.0;

    {
        std::lock_guard<std::mutex> lock(results_mutex[eyeIndex]);

        if (eye_calib_data[eyeIndex].has_calibration) {
            // 计算相对于中心点的偏移量
            double offsetX = pupil[eyeIndex].x - eye_calib_data[eyeIndex].calib_XOFF;
            double offsetY = pupil[eyeIndex].y - eye_calib_data[eyeIndex].calib_YOFF;

            // 归一化偏移量
            double maxOffsetX = max(
                std::abs(eye_calib_data[eyeIndex].calib_XMAX - eye_calib_data[eyeIndex].calib_XOFF),
                std::abs(eye_calib_data[eyeIndex].calib_XMIN - eye_calib_data[eyeIndex].calib_XOFF)
            );

            double maxOffsetY = max(
                std::abs(eye_calib_data[eyeIndex].calib_YMAX - eye_calib_data[eyeIndex].calib_YOFF),
                std::abs(eye_calib_data[eyeIndex].calib_YMIN - eye_calib_data[eyeIndex].calib_YOFF)
            );

            // 避免除以零
            if (maxOffsetX > 0.0001) {
                eyeX = 0.5 - (offsetX / (2.0 * maxOffsetX));
            }

            if (maxOffsetY > 0.0001) {
                eyeY = 0.5 + (offsetY / (2.0 * maxOffsetY));
            }

            // 限制范围在0-1之间
            eyeX = max(0.0, min(1.0, eyeX));
            eyeY = max(0.0, min(1.0, eyeY));
        }

        // *** 修改部分：使用补偿后的开合度值进行UI显示 ***
        // 删除原有的校准计算代码：
        /*
        // 计算眼睛开合度 (0.0-1.0)
        // 使用校准后的值计算开合度百分比
        double fullyOpen = eye_fully_open[eyeIndex];
        double fullyClosed = eye_fully_closed[eyeIndex];

        // 确保有校准数据
        if (std::abs(fullyOpen - fullyClosed) > 0.0001) {
            opennessPct = (eye_open[eyeIndex] - fullyClosed) / (fullyOpen - fullyClosed);
        } else {
            // 如果没有校准数据，使用一个估计值
            opennessPct = eye_open[eyeIndex] / 30.0;  // 假设30是一个合理的范围
        }

        opennessPct = max(0.0, min(1.0, opennessPct));
        */

        // 新的实现：直接使用补偿后的值
        {
            std::lock_guard<std::mutex> comp_lock(compensated_data_mutex[eyeIndex]);
            opennessPct = compensated_eye_openness[eyeIndex];
        }

        // 确保范围在0-1之间（虽然补偿处理已经限制了，但这里再次确保）
        opennessPct = max(0.0, min(1.0, opennessPct));
    }

    // 更新 UI 显示 (线程安全的方式)
    QMetaObject::invokeMethod(this, [this, eyeIndex, eyeX, eyeY, opennessPct]() {
        if (eyeIndex == LEFT_TAG) {
            leftEyePositionWidget->setPosition(eyeX, eyeY);
            leftEyePositionWidget->setOpenness(opennessPct);
            if (ui.LeftEyeOpennessBar) {
                ui.LeftEyeOpennessBar->setValue(static_cast<int>(opennessPct * 100));
            }
        } else {
            rightEyePositionWidget->setPosition(eyeX, eyeY);
            rightEyePositionWidget->setOpenness(opennessPct);
            if (ui.RightEyeOpennessBar) {
                ui.RightEyeOpennessBar->setValue(static_cast<int>(opennessPct * 100));
            }
        }
    }, Qt::QueuedConnection);
}
void PaperEyeTrackerWindow::calibrateEyeOpen() {
    // 检查哪些眼睛连接了
    bool left_connected = !current_ip[LEFT_TAG].empty() && image_stream[LEFT_TAG]->isStreaming();
    bool right_connected = !current_ip[RIGHT_TAG].empty() && image_stream[RIGHT_TAG]->isStreaming();

    if (!left_connected && !right_connected) {
        LOG_INFO("没有检测到连接的眼睛");
        QMessageBox::information(this, "没有连接", "没有检测到任何连接的眼睛，请先连接眼睛设备");
        return;
    }

    LOG_INFO("请将眼睛完全张开并保持...");
    //QMessageBox msgBox;
    //msgBox.setWindowTitle("眼睛完全张开校准");
    //msgBox.setText("请将眼睛完全张开，然后点击确定按钮进行校准。");
    //msgBox.exec();

    // 直接采集当前帧的数据
    if (left_connected) {
        std::lock_guard<std::mutex> lock(results_mutex[LEFT_TAG]);
        eye_fully_open[LEFT_TAG] = eye_open[LEFT_TAG];
        LOG_INFO("左眼完全张开校准完成: 值 = {:.3f}", eye_fully_open[LEFT_TAG]);
    }

    if (right_connected) {
        std::lock_guard<std::mutex> lock(results_mutex[RIGHT_TAG]);
        eye_fully_open[RIGHT_TAG] = eye_open[RIGHT_TAG];
        LOG_INFO("右眼完全张开校准完成: 值 = {:.3f}", eye_fully_open[RIGHT_TAG]);
    }
    calibrated_eye_open = true;
    updateCalibrationButtonStates();
    //QMessageBox::information(this, "校准完成", "眼睛完全张开校准已完成！");
}

void PaperEyeTrackerWindow::calibrateEyeClose() {
    // 检查哪些眼睛连接了
    bool left_connected = !current_ip[LEFT_TAG].empty() && image_stream[LEFT_TAG]->isStreaming();
    bool right_connected = !current_ip[RIGHT_TAG].empty() && image_stream[RIGHT_TAG]->isStreaming();

    if (!left_connected && !right_connected) {
        LOG_INFO("没有检测到连接的眼睛");
        QMessageBox::information(this, "没有连接", "没有检测到任何连接的眼睛，请先连接眼睛设备");
        return;
    }

    LOG_INFO("请将眼睛完全闭合并保持...");
    //QMessageBox msgBox;
    //msgBox.setWindowTitle("眼睛完全闭合校准");
    //msgBox.setText("请将眼睛完全闭合，然后点击确定按钮进行校准。");
    //msgBox.exec();

    // 直接采集当前帧的数据
    if (left_connected) {
        std::lock_guard<std::mutex> lock(results_mutex[LEFT_TAG]);
        eye_fully_closed[LEFT_TAG] = eye_open[LEFT_TAG]+5 ;
        LOG_INFO("左眼完全闭合校准完成: 值 = {:.3f}", eye_fully_closed[LEFT_TAG]);
    }

    if (right_connected) {
        std::lock_guard<std::mutex> lock(results_mutex[RIGHT_TAG]);
        eye_fully_closed[RIGHT_TAG] = eye_open[RIGHT_TAG]+5;
        LOG_INFO("右眼完全闭合校准完成: 值 = {:.3f}", eye_fully_closed[RIGHT_TAG]);
    }
    calibrated_eye_close = true;
    updateCalibrationButtonStates();
    //QMessageBox::information(this, "校准完成", "眼睛完全闭合校准已完成！");
}
void PaperEyeTrackerWindow::updateBatteryStatus(int version) const
{
    if (image_stream[version] && image_stream[version]->isStreaming())
    {
        float battery = image_stream[version]->getBatteryPercentage();
        QString batteryText = QString("电池电量: %1%").arg(battery, 0, 'f', 1);

        if (version == LEFT_TAG) {
            ui.LeftEyeBatteryLabel->setText(batteryText);
        } else if (version == RIGHT_TAG) {
            ui.RightEyeBatteryLabel->setText(batteryText);
        }
    }
    else
    {
        if (version == LEFT_TAG) {
            ui.LeftEyeBatteryLabel->setText("左眼电池: 未知");
        } else if (version == RIGHT_TAG) {
            ui.RightEyeBatteryLabel->setText("右眼电池: 未知");
        }
    }
}
void PaperEyeTrackerWindow::updateCalibrationButtonStates()
{
    // 设置按钮样式
    QString uncalibratedStyle = "background-color: #ffcccc;"; // 浅红色
    QString calibratedStyle = ""; // 默认样式

    // 更新每个按钮的样式
    ui.settingsCalibrateButton->setStyleSheet(calibrated_position ? calibratedStyle : uncalibratedStyle);
    ui.settingsCenterButton->setStyleSheet(calibrated_center ? calibratedStyle : uncalibratedStyle);
    ui.settingsEyeOpenButton->setStyleSheet(calibrated_eye_open ? calibratedStyle : uncalibratedStyle);
    ui.settingsEyeCloseButton->setStyleSheet(calibrated_eye_close ? calibratedStyle : uncalibratedStyle);
}
void PaperEyeTrackerWindow::onEyeSyncModeChanged(int index) {
    eyeSyncMode = static_cast<EyeSyncMode>(index);
    LOG_INFO("眼皮同步模式已更改为: {}",
             index == 0 ? "双眼眼皮独立控制眼皮" :
             (index == 1 ? "左眼眼皮控制双眼眼皮" : "右眼眼皮控制双眼眼皮"));

    // 保存设置到配置
    config.eye_sync_mode = index;
}
// 在 eye_tracker_window.cpp 文件中实现这些函数
void PaperEyeTrackerWindow::onLeftEyeValueIncrease() {
    std::lock_guard<std::mutex> lock(results_mutex[LEFT_TAG]);
    eye_fully_closed[LEFT_TAG] += 1.0; // 增加闭合值
    LOG_INFO("左眼闭合值增加到: {:.3f}", eye_fully_closed[LEFT_TAG]);
}

void PaperEyeTrackerWindow::onLeftEyeValueDecrease() {
    std::lock_guard<std::mutex> lock(results_mutex[LEFT_TAG]);
    eye_fully_closed[LEFT_TAG] = max(5.0, eye_fully_closed[LEFT_TAG] - 1.0); // 减少闭合值，但不低于5
    LOG_INFO("左眼闭合值减少到: {:.3f}", eye_fully_closed[LEFT_TAG]);
}

void PaperEyeTrackerWindow::onRightEyeValueIncrease() {
    std::lock_guard<std::mutex> lock(results_mutex[RIGHT_TAG]);
    eye_fully_closed[RIGHT_TAG] += 1.0; // 增加闭合值
    LOG_INFO("右眼闭合值增加到: {:.3f}", eye_fully_closed[RIGHT_TAG]);
}

void PaperEyeTrackerWindow::onRightEyeValueDecrease() {
    std::lock_guard<std::mutex> lock(results_mutex[RIGHT_TAG]);
    eye_fully_closed[RIGHT_TAG] = max(5.0, eye_fully_closed[RIGHT_TAG] - 1.0); // 减少闭合值，但不低于5
    LOG_INFO("右眼闭合值减少到: {:.3f}", eye_fully_closed[RIGHT_TAG]);
}
