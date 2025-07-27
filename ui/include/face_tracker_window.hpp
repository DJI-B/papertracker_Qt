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
#include "face_inference.hpp"
#include "serial.hpp"
#include "logger.hpp"
#include "updater.hpp"
#include "translator_manager.h"
#include <QGridLayout>
#include <QStackedWidget>
#include <QScrollArea>
#include <QLabel>
#include <QApplication>
#include <QScrollBar>
#include <QCheckBox>
#include <QComboBox>
#include <QProgressBar>
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
    float jaw_left_offset = 0.0f;
    float jaw_right_offset = 0.0f;
    float mouth_left_offset = 0.0f;
    float mouth_right_offset = 0.0f;
    float tongue_left_offset = 0.0f;
    float tongue_right_offset = 0.0f;
    float tongue_up_offset = 0.0f;
    float tongue_down_offset = 0.0f;

    // 卡尔曼滤波参数
    float dt = 0.02f;
    float q_factor = 1.5f;
    float r_factor = 0.0003f;
    std::unordered_map<std::string, float> amp_map;
    Rect rect;


    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PaperFaceTrackerConfig, brightness, rotate_angle, energy_mode, wifi_ip, use_filter, amp_map, rect, cheek_puff_left_offset, cheek_puff_right_offset,
        jaw_open_offset, tongue_out_offset, mouth_close_offset, mouth_funnel_offset, mouth_pucker_offset,
        mouth_roll_upper_offset, mouth_roll_lower_offset, mouth_shrug_upper_offset, mouth_shrug_lower_offset, jaw_left_offset, jaw_right_offset, mouth_left_offset, mouth_right_offset, tongue_left_offset, tongue_right_offset, tongue_up_offset, tongue_down_offset, dt, q_factor, r_factor);
};

class PaperFaceTrackerWindow final : public QWidget {
public:

    struct ParameterInfo {
        float* offsetValue;              // 偏置值变量指针
        float* ampValue;                 // 放大倍数值变量指针
        QLineEdit* offsetEdit;           // 偏置值输入框
        QLineEdit* ampEdit;              // 放大倍数输入框
        QProgressBar* progressBar;       // 进度条
        QLabel* label;                   // 标签
        QCheckBox* enablCheckBox;        // 启用/禁用控制 CheckBox（新增）


        // 更新构造函数
        ParameterInfo(float* offsetValue = nullptr, float* ampValue = nullptr,
                      QLineEdit* offsetEdit = nullptr, QLineEdit* ampEdit = nullptr,
                      QProgressBar* progressBar = nullptr, QLabel* label = nullptr,
                      QCheckBox* enablCheckBox = nullptr)
            : offsetValue(offsetValue), ampValue(ampValue),
              offsetEdit(offsetEdit), ampEdit(ampEdit),
              progressBar(progressBar), label(label),
              enablCheckBox(enablCheckBox) {}
    };
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

    void setAmplitudeValuesFromConfig();

    std::unordered_map<std::string, float> getAmpMap() const;

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
    void onCheekPuffLeftChanged() const;
    void onCheekPuffRightChanged() const;
    void onJawOpenChanged() const;
    void onJawLeftChanged() const;
    void onJawRightChanged() const;
    void onMouthLeftChanged() const;
    void onMouthRightChanged();
    void onTongueOutChanged();
    void onTongueLeftChanged();
    void onTongueRightChanged();
    void onTongueUpChanged() const;
    void onTongueDownChanged() const;
    // 在既有的private slots:部分后面添加以下内容
    void onMouthCloseChanged() const;
    void onMouthFunnelChanged() const;
    void onMouthPuckerChanged() const;
    void onMouthRollUpperChanged() const;
    void onMouthRollLowerChanged() const;
    void onMouthShrugUpperChanged() const;
    void onMouthShrugLowerChanged() const;

    void clearCalibrationParameters(bool clearOffset = true, bool clearAmp = true);

    void onMouthCloseOffsetChanged();
    void onMouthFunnelOffsetChanged();
    void onMouthPuckerOffsetChanged();
    void onMouthRollUpperOffsetChanged();
    void onMouthRollLowerOffsetChanged();
    void onMouthShrugUpperOffsetChanged();
    void onMouthShrugLowerOffsetChanged();
    void onJawLeftOffsetChanged();
    void onJawRightOffsetChanged();
    void onMouthLeftOffsetChanged();
    void onMouthRightOffsetChanged();
    void onTongueUpOffsetChanged();
    void onTongueDownOffsetChanged();
    void onTongueLeftOffsetChanged();
    void onTongueRightOffsetChanged();

    // 卡尔曼滤波参数调整函数
    void onDtEditingFinished();
    void onQFactorEditingFinished();
    void onRFactorEditingFinished();

    // 设置卡尔曼滤波参数控制UI
    void setupKalmanFilterControls();
    void onCalibrationStartClicked();

    void onCalibrationTimeout();

    void onCalibrationStopClicked();

private:
    void initUi();
    void initializeParameters();
    void initLayout();
    //翻译槽函数
    void retranslateUI();
    void updatePageWidth();
    void collectData(const std::vector<float>& output, const std::unordered_map<std::string, size_t>& blendShapeIndexMap);
    void calculateCalibrationOffsets();
    void applyCalibrationOffsets();
    void calculateCalibrationAMP();
    void applyCalibrationAMP();

    void updateAmplitudeValue(const QString &paramName, float value);

    void addCalibrationParam(QGridLayout* layout, const QString& name, const ParameterInfo& param, int row = 0);

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
    int current_rotate_angle = 50;

    std::string current_ip_;

    void bound_pages();

    void connectOffsetChangeEvent();
    void disconnectOffsetChangeEvent();

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
    float mouth_close_offset = 0.0f;
    float mouth_funnel_offset = 0.0f;
    float mouth_pucker_offset = 0.0f;
    float mouth_roll_upper_offset = 0.0f;
    float mouth_roll_lower_offset = 0.0f;
    float mouth_shrug_upper_offset = 0.0f;
    float mouth_shrug_lower_offset = 0.0f;
    float jaw_left_offset = 0.0f;
    float jaw_right_offset = 0.0f;
    float mouth_left_offset = 0.0f;
    float mouth_right_offset = 0.0f;
    float tongue_left_offset = 0.0f;
    float tongue_right_offset = 0.0f;
    float tongue_up_offset = 0.0f;
    float tongue_down_offset = 0.0f;

    float cheek_puff_left_amp = 1.0f;
    float cheek_puff_right_amp = 1.0f;
    float jaw_open_amp = 1.0f;
    float tongue_out_amp = 1.0f;
    float mouth_close_amp = 1.0f;
    float mouth_funnel_amp = 1.0f;
    float mouth_pucker_amp = 1.0f;
    float mouth_roll_upper_amp = 1.0f;
    float mouth_roll_lower_amp = 1.0f;
    float mouth_shrug_upper_amp = 1.0f;
    float mouth_shrug_lower_amp = 1.0f;
    float jaw_left_amp = 1.0f;
    float jaw_right_amp = 1.0f;
    float mouth_left_amp = 1.0f;
    float mouth_right_amp = 1.0f;
    float tongue_left_amp = 1.0f;
    float tongue_right_amp = 1.0f;
    float tongue_up_amp = 1.0f;
    float tongue_down_amp = 1.0f;

    // 卡尔曼滤波参数控制
    QLabel* dtLabel = nullptr;
    QLineEdit* dtLineEdit = nullptr;

    QLabel* qFactorLabel = nullptr;
    QLineEdit* qFactorLineEdit = nullptr;

    QLabel* rFactorLabel = nullptr;
    QLineEdit* rFactorLineEdit = nullptr;
    QLabel* helpLabel = nullptr;
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
    QLineEdit *MouthCloseBar;
    QProgressBar *MouthCloseValue;
    QLabel *label_mouthFunnel;
    QLineEdit *MouthFunnelOffset;
    QLineEdit *MouthFunnelBar;
    QProgressBar *MouthFunnelValue;
    QLabel *label_mouthPucker;
    QLineEdit *MouthPuckerOffset;
    QLineEdit *MouthPuckerBar;
    QProgressBar *MouthPuckerValue;
    QLabel *label_mouthRollUpper;
    QLineEdit *MouthRollUpperOffset;
    QLineEdit *MouthRollUpperBar;
    QProgressBar *MouthRollUpperValue;
    QLabel *label_mouthRollLower;
    QLineEdit *MouthRollLowerOffset;
    QLineEdit *MouthRollLowerBar;
    QProgressBar *MouthRollLowerValue;
    QLabel *label_mouthShrugUpper;
    QLineEdit *MouthShrugUpperOffset;
    QLineEdit *MouthShrugUpperBar;
    QProgressBar *MouthShrugUpperValue;
    QLabel *label_mouthShrugLower;
    QLineEdit *MouthShrugLowerOffset;
    QLineEdit *MouthShrugLowerBar;
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
    QLineEdit *CheekPuffLeftBar;
    QLineEdit *CheekPuffRightBar;
    QLineEdit *JawOpenBar;
    QLineEdit *JawLeftBar;
    QLineEdit *MouthLeftBar;
    QLineEdit *JawRightBar;
    QLineEdit *TongueOutBar;
    QLineEdit *MouthRightBar;
    QLineEdit *TongueDownBar;
    QLineEdit *TongueUpBar;
    QLineEdit *TongueRightBar;
    QLineEdit *TongueLeftBar;
    QLabel *ImageLabelCal;
    QLabel* lockLabel;
    QLabel *label_3;
    QLabel *label_20;
    QPushButton *MainPageButton;
    QPushButton *CalibrationPageButton;
    QLabel *WifiConnectLabel;
    QLabel *SerialConnectLabel;
    QLabel *BatteryStatusLabel;
    QLabel *calibrationModeLabel;
    QComboBox *calibrationModeComboBox;
    QPushButton *calibrationStartButton;
    QPushButton *calibrationStopButton;
    QPushButton *calibrationResetButton;
    QLineEdit *JawLeftOffset;
    QLineEdit *JawRightOffset;
    QLineEdit *MouthLeftOffset;
    QLineEdit *MouthRightOffset;
    QLineEdit *TongueUpOffset;
    QLineEdit *TongueDownOffset;
    QLineEdit *TongueLeftOffset;
    QLineEdit *TongueRightOffset;


    QLabel *tutorialLink;
    QHBoxLayout *calibrationPageLayout;
    QVBoxLayout *page2RightLayout;
    QWidget *topTitleWidget;

    int currentSettingPageWidth = 0;
    bool is_calibrating = false;                     // 校准状态标志
    std::unordered_map<std::string, std::vector<float>> calibration_data; // 校准数据存储
    std::unordered_map<std::string, float> calibration_offsets;           // 计算出的偏置值
    int calibration_sample_count = 0;                // 校准样本计数
    QTimer* calibrationTimer;  // 校准超时计时器

    std::unordered_map<std::string, float> calibration_amp_ratios;
    std::unordered_map<QString, ParameterInfo> parameterMap;
    std::vector<QString> parameterOrder;

    QCheckBox* CheekPuffLeftEnable;
    QCheckBox* CheekPuffRightEnable;
    QCheckBox* JawOpenEnable;
    QCheckBox* JawLeftEnable;
    QCheckBox* JawRightEnable;
    QCheckBox* MouthCloseEnable;
    QCheckBox* MouthFunnelEnable;
    QCheckBox* MouthLeftEnable;
    QCheckBox* MouthPuckerEnable;
    QCheckBox* MouthRightEnable;
    QCheckBox* MouthRollLowerEnable;
    QCheckBox* MouthRollUpperEnable;
    QCheckBox* MouthShrugLowerEnable;
    QCheckBox* MouthShrugUpperEnable;
    QCheckBox* TongueOutEnable;
    QCheckBox* TongueDownEnable;
    QCheckBox* TongueLeftEnable;
    QCheckBox* TongueRightEnable;
    QCheckBox* TongueUpEnable;

};

#endif //PAPER_FACE_TRACKER_WINDOW_HPP
