//
// Created by JellyfishKnight on 25-3-11.
//

#ifndef PAPER_FACE_TRACKER_WINDOW_HPP
#define PAPER_FACE_TRACKER_WINDOW_HPP

#include <QProcess>
#include <thread>
#include <future>
#include <atomic>
#include <algorithm>
#include <config_writer.hpp>
#include <image_downloader.hpp>
#include <osc.hpp>
#include <QTimer>
#include <QLineEdit>  // 确保包含该头文件
#include "ui_face_tracker_window.h"
#include "face_inference.hpp"
#include "serial.hpp"
#include "logger.hpp"
#include "updater.hpp"
#include "translator_manager.h"
#include <QGridLayout>
struct Rect
{
public:
    Rect() = default;

    Rect(int x, int y, int width, int height) :
        rect(x, y, width, height), x(x), y(y), width(width), height(height) {}

    // delete operate = to avoid copy
    Rect& operator=(const Rect& other)
    {
        this->rect = cv::Rect(other.x, other.y, other.width, other.height);
        this->x = other.x;
        this->y = other.y;
        this->width = other.width;
        this->height = other.height;
        return *this;
    }

    Rect(cv::Rect rect) : rect(rect), x(rect.x), y(rect.y), width(rect.width), height(rect.height) {}

    cv::Rect rect;
    bool is_roi_end = true;

    int x{0};
    int y{0};
    int width{0};
    int height{0};

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Rect, x, y, width, height);
};

struct PaperFaceTrackerConfig {
    int brightness;
    int rotate_angle;
    int energy_mode;
    std::string wifi_ip;
    bool use_filter;
    float cheek_puff_left_offset = 0.0f;
    float cheek_puff_right_offset = 0.0f;
    float jaw_open_offset = 0.0f;
    float tongue_out_offset = 0.0f;
    float mouth_close_offset = 0.0f;
    float mouth_funnel_offset = 0.0f;
    float mouth_pucker_offset = 0.0f;
    float mouth_roll_upper_offset = 0.0f;
    float mouth_roll_lower_offset = 0.0f;
    float mouth_shrug_upper_offset = 0.0f;
    float mouth_shrug_lower_offset = 0.0f;
    // 卡尔曼滤波参数
    float dt = 0.02f;
    float q_factor = 1.5f;
    float r_factor = 0.0003f;
    std::unordered_map<std::string, int> amp_map;
    Rect rect;


    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PaperFaceTrackerConfig, brightness, rotate_angle, energy_mode, wifi_ip, use_filter, amp_map, rect, cheek_puff_left_offset, cheek_puff_right_offset,
        jaw_open_offset, tongue_out_offset, mouth_close_offset, mouth_funnel_offset, mouth_pucker_offset,
        mouth_roll_upper_offset, mouth_roll_lower_offset, mouth_shrug_upper_offset, mouth_shrug_lower_offset, dt, q_factor, r_factor);

};

class PaperFaceTrackerWindow final : public QWidget {
private:
public:
    explicit PaperFaceTrackerWindow(QWidget *parent = nullptr);
    ~PaperFaceTrackerWindow() override;

    void setSerialStatusLabel(const QString& text) const;
    void setWifiStatusLabel(const QString& text) const;
    void setIPText(const QString& text) const;

    QPlainTextEdit* getLogText() const;
    Rect getRoiRect();
    float getRotateAngle() const;
    std::string getSSID() const;
    std::string getPassword() const;

    void setVideoImage(const cv::Mat& image);
    // 根据模型输出更新校准页面的进度条
    void updateCalibrationProgressBars(
        const std::vector<float>& output,
        const std::unordered_map<std::string, size_t>& blendShapeIndexMap
    );

    using FuncWithoutArgs = std::function<void()>;
    using FuncWithVal = std::function<void(int)>;
    // let user decide what to do with these action
    void setOnUseFilterClickedFunc(FuncWithVal func);
    void set_osc_send_thead(FuncWithoutArgs func);

    bool is_running() const;

    void stop();
    int get_max_fps() const;

    PaperFaceTrackerConfig generate_config() const;

    void set_config();

    std::unordered_map<std::string, int> getAmpMap() const;

    void updateWifiLabel() const;
    void updateBatteryStatus() const;
    void updateSerialLabel() const;

    cv::Mat getVideoImage() const;
    std::string getFirmwareVersion() const;
    SerialStatus getSerialStatus() const;

private slots:
    void onCheekPuffLeftOffsetChanged();
    void onCheekPuffRightOffsetChanged();
    void onJawOpenOffsetChanged();
    void onTongueOutOffsetChanged();
    void onBrightnessChanged(int value);
    void onSendBrightnessValue() const;
    void onRotateAngleChanged(int value);
    void onSendButtonClicked();
    void onRestartButtonClicked();
    void onUseFilterClicked(int value) const;
    void onFlashButtonClicked();
    void onEnergyModeChanged(int value);
    void onShowSerialDataButtonClicked();
    void onCheekPuffLeftChanged(int value) const;
    void onCheekPuffRightChanged(int value) const;
    void onJawOpenChanged(int value) const;
    void onJawLeftChanged(int value) const;
    void onJawRightChanged(int value) const;
    void onMouthLeftChanged(int value) const;
    void onMouthRightChanged(int value);
    void onTongueOutChanged(int value);
    void onTongueLeftChanged(int value);
    void onTongueRightChanged(int value);
    void onTongueUpChanged(int value) const;
    void onTongueDownChanged(int value) const;
    // 在既有的private slots:部分后面添加以下内容
    void onMouthCloseChanged(int value) const;
    void onMouthFunnelChanged(int value) const;
    void onMouthPuckerChanged(int value) const;
    void onMouthRollUpperChanged(int value) const;
    void onMouthRollLowerChanged(int value) const;
    void onMouthShrugUpperChanged(int value) const;
    void onMouthShrugLowerChanged(int value) const;

    void onMouthCloseOffsetChanged();
    void onMouthFunnelOffsetChanged();
    void onMouthPuckerOffsetChanged();
    void onMouthRollUpperOffsetChanged();
    void onMouthRollLowerOffsetChanged();
    void onMouthShrugUpperOffsetChanged();
    void onMouthShrugLowerOffsetChanged();
    // 卡尔曼滤波参数调整函数
    void onDtEditingFinished();
    void onQFactorEditingFinished();
    void onRFactorEditingFinished();

    // 设置卡尔曼滤波参数控制UI
    void setupKalmanFilterControls();

    //翻译槽函数
    void retranslateUI();
private:
    void InitUi();
    void InitLayout();
    void addCalibrationParam(QGridLayout* layout, QLabel* label = nullptr, QLineEdit* offsetEdit =nullptr, QScrollBar* bar = nullptr, QProgressBar* value = nullptr, int row = 0);

    void start_image_download() const;
    std::vector<std::string> serialRawDataLog;
    bool showSerialData = false;
    QLabel* roiStatusLabel = nullptr;
    FuncWithVal onRotateAngleChangedFunc;
    FuncWithVal onUseFilterClickedFunc;
    FuncWithoutArgs onSaveConfigButtonClickedFunc;
    FuncWithoutArgs onAmpMapChangedFunc;
    std::shared_ptr<QTimer> brightness_timer;
    std::shared_ptr<HttpServer> http_server;
    int current_brightness;
    int current_rotate_angle = 540;

    std::string current_ip_;
    void bound_pages();

    void connect_callbacks();

    void create_sub_threads();

    // 定义所有ARKit模型输出名称
    const std::vector<std::string> blend_shapes =  {
        "/cheekPuffLeft", "/cheekPuffRight",
        "/cheekSuckLeft", "/cheekSuckRight",
        "/jawOpen", "/jawForward", "/jawLeft", "/jawRight",
        "/noseSneerLeft", "/noseSneerRight",
        "/mouthFunnel", "/mouthPucker",
        "/mouthLeft", "/mouthRight",
        "/mouthRollUpper", "/mouthRollLower",
        "/mouthShrugUpper", "/mouthShrugLower",
        "/mouthClose",
        "/mouthSmileLeft", "/mouthSmileRight",
        "/mouthFrownLeft", "/mouthFrownRight",
        "/mouthDimpleLeft", "/mouthDimpleRight",
        "/mouthUpperUpLeft", "/mouthUpperUpRight",
        "/mouthLowerDownLeft", "/mouthLowerDownRight",
        "/mouthPressLeft", "/mouthPressRight",
        "/mouthStretchLeft", "/mouthStretchRight",
        "/tongueOut", "/tongueUp", "/tongueDown",
        "/tongueLeft", "/tongueRight",
        "/tongueRoll", "/tongueBendDown",
        "/tongueCurlUp", "/tongueSquish",
        "/tongueFlat", "/tongueTwistLeft",
        "/tongueTwistRight"
    };

    Rect roi_rect;
    void updateOffsetsToInference();
    std::thread update_thread;
    std::thread inference_thread;
    std::thread osc_send_thread;
    bool app_is_running = true;
    int max_fps = 38;

    std::shared_ptr<SerialPortManager> serial_port_manager;
    std::shared_ptr<ESP32VideoStream> image_downloader;
    std::shared_ptr<FaceInference> inference;
    std::shared_ptr<OscManager> osc_manager;
    std::shared_ptr<ConfigWriter> config_writer;

    PaperFaceTrackerConfig config;

    std::string firmware_version;
    float cheek_puff_left_offset = 0.0f;
    float cheek_puff_right_offset = 0.0f;
    float jaw_open_offset = 0.0f;
    float tongue_out_offset = 0.0f;
    // 卡尔曼滤波参数控制
    QLabel* dtLabel = nullptr;
    QLineEdit* dtLineEdit = nullptr;

    QLabel* qFactorLabel = nullptr;
    QLineEdit* qFactorLineEdit = nullptr;

    QLabel* rFactorLabel = nullptr;
    QLineEdit* rFactorLineEdit = nullptr;
    QLabel* helpLabel = nullptr;
    // 在private:部分的其他偏置值变量声明后添加
    float mouth_close_offset = 0.0f;
    float mouth_funnel_offset = 0.0f;
    float mouth_pucker_offset = 0.0f;
    float mouth_roll_upper_offset = 0.0f;
    float mouth_roll_lower_offset = 0.0f;
    float mouth_shrug_upper_offset = 0.0f;
    float mouth_shrug_lower_offset = 0.0f;
    float current_dt = 0.02f;
    float current_q_factor = 1.5f;
    float current_r_factor = 0.0003f;
    std::vector<float> outputs;
    std::mutex outputs_mutex;
    QTimer* auto_save_timer;
    inline static PaperFaceTrackerWindow* instance = nullptr;
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    // UI元素指针声明
    QStackedWidget *stackedWidget;
    QWidget *page;
    QLabel *ImageLabel;
    QPlainTextEdit *SSIDText;
    QPlainTextEdit *PasswordText;
    QScrollBar *BrightnessBar;
    QLabel *label;
    QPushButton *wifi_send_Button;
    QPushButton *FlashFirmwareButton;
    QPlainTextEdit *LogText;
    QLabel *label_2;
    QTextEdit *textEdit;
    QLabel *label_16;
    QPushButton *restart_Button;
    QLabel *label_17;
    QScrollBar *RotateImageBar;
    QCheckBox *UseFilterBox;
    QComboBox *EnergyModeBox;
    QLabel *label_18;
    QPushButton *ShowSerialDataButton;
    QWidget *page_2;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QLabel *label_5;
    QLineEdit *CheekPuffLeftOffset;
    QLabel *label_10;
    QLabel *label_13;
    QLabel *label_6;
    QLineEdit *CheekPuffRightOffset;
    QLabel *label_15;
    QLabel *label_7;
    QLineEdit *JawOpenOffset;
    QLabel *label_11;
    QLabel *label_9;
    QLabel *label_14;
    QLabel *label_8;
    QLabel *label_12;
    QLineEdit *TongueOutOffset;
    QLabel *label_31;
    QLabel *label_mouthClose;
    QLineEdit *MouthCloseOffset;
    QScrollBar *MouthCloseBar;
    QProgressBar *MouthCloseValue;
    QLabel *label_mouthFunnel;
    QLineEdit *MouthFunnelOffset;
    QScrollBar *MouthFunnelBar;
    QProgressBar *MouthFunnelValue;
    QLabel *label_mouthPucker;
    QLineEdit *MouthPuckerOffset;
    QScrollBar *MouthPuckerBar;
    QProgressBar *MouthPuckerValue;
    QLabel *label_mouthRollUpper;
    QLineEdit *MouthRollUpperOffset;
    QScrollBar *MouthRollUpperBar;
    QProgressBar *MouthRollUpperValue;
    QLabel *label_mouthRollLower;
    QLineEdit *MouthRollLowerOffset;
    QScrollBar *MouthRollLowerBar;
    QProgressBar *MouthRollLowerValue;
    QLabel *label_mouthShrugUpper;
    QLineEdit *MouthShrugUpperOffset;
    QScrollBar *MouthShrugUpperBar;
    QProgressBar *MouthShrugUpperValue;
    QLabel *label_mouthShrugLower;
    QLineEdit *MouthShrugLowerOffset;
    QScrollBar *MouthShrugLowerBar;
    QProgressBar *MouthShrugLowerValue;
    QProgressBar *CheekPullLeftValue;
    QProgressBar *CheekPullRightValue;
    QProgressBar *JawLeftValue;
    QProgressBar *JawOpenValue;
    QProgressBar *MouthLeftValue;
    QProgressBar *JawRightValue;
    QProgressBar *TongueOutValue;
    QProgressBar *MouthRightValue;
    QProgressBar *TongueDownValue;
    QProgressBar *TongueUpValue;
    QProgressBar *TongueRightValue;
    QProgressBar *TongueLeftValue;
    QScrollBar *CheekPuffLeftBar;
    QScrollBar *CheekPuffRightBar;
    QScrollBar *JawOpenBar;
    QScrollBar *JawLeftBar;
    QScrollBar *MouthLeftBar;
    QScrollBar *JawRightBar;
    QScrollBar *TongueOutBar;
    QScrollBar *MouthRightBar;
    QScrollBar *TongueDownBar;
    QScrollBar *TongueUpBar;
    QScrollBar *TongueRightBar;
    QScrollBar *TongueLeftBar;
    QLabel *ImageLabelCal;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_19;
    QLabel *label_20;
    QPushButton *MainPageButton;
    QPushButton *CalibrationPageButton;
    QLabel *WifiConnectLabel;
    QLabel *SerialConnectLabel;
    QLabel *BatteryStatusLabel;

    QLabel *tutorialLink;
    QHBoxLayout *calibrationPageLayout;
    QVBoxLayout *page2RightLayout;
    QWidget *topTitleWidget;
};

#endif //PAPER_FACE_TRACKER_WINDOW_HPP
