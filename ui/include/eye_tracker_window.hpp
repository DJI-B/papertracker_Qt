//
// Created by JellyfishKnight on 25-3-11.
//

#ifndef PAPER_EYE_TRACKER_WINDOW_HPP
#define PAPER_EYE_TRACKER_WINDOW_HPP

#include <eye_inference.hpp>
#include <face_tracker_window.hpp>

#include "serial.hpp"
#include "image_downloader.hpp"
#include "osc.hpp"
#include "logger.hpp"
#include <QTimer>
#include "config_writer.hpp"
#include "osc.hpp"
#include "face_inference.hpp"
#include <list>

#include <QPainter>
#include <QApplication>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>
#include <QMessageBox>

// 下位机发送的version tag
#define FACE_VERSION 1
#define LEFT_VERSION 2
#define RIGHT_VERSION 3

// 上位机用于索引的tag
#define LEFT_TAG 0
#define RIGHT_TAG 1
#define EYE_NUM 2

// #define CALIBRATION_SAMPLES 150


struct PaperEyeTrackerConfig
{
    std::string left_ip;
    std::string right_ip;
    int left_brightness;
    int right_brightness;
    int energy_mode;
    Rect left_roi;
    Rect right_roi;

    // 左眼校准数据
    double left_calib_XMIN = 0;
    double left_calib_XMAX = 0;
    double left_calib_YMIN = 0;
    double left_calib_YMAX = 0;
    double left_calib_XOFF = 0;
    double left_calib_YOFF = 0;
    bool left_has_calibration = false;

    // 右眼校准数据
    double right_calib_XMIN = 0;
    double right_calib_XMAX = 0;
    double right_calib_YMIN = 0;
    double right_calib_YMAX = 0;
    double right_calib_XOFF = 0;
    double right_calib_YOFF = 0;
    bool right_has_calibration = false;

    // 轴翻转设置
    bool left_flip_x = false;
    bool right_flip_x = true;
    bool flip_y = false;
    int left_rotate_angle = 0;
    int right_rotate_angle = 0;

    double left_eye_fully_open = 30.0;
    double left_eye_fully_closed = 10.0;
    double right_eye_fully_open = 30;
    double right_eye_fully_closed = 10.0;
    int eye_sync_mode = 0; // 默认为双眼独立控制
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PaperEyeTrackerConfig, left_ip, right_ip, left_brightness,
    right_brightness, energy_mode, left_roi, right_roi,
    left_calib_XMIN, left_calib_XMAX, left_calib_YMIN, left_calib_YMAX,
    left_calib_XOFF, left_calib_YOFF, left_has_calibration,
    right_calib_XMIN, right_calib_XMAX, right_calib_YMIN, right_calib_YMAX,
    right_calib_XOFF, right_calib_YOFF, right_has_calibration,
    left_flip_x, right_flip_x, flip_y, left_rotate_angle, right_rotate_angle,
    left_eye_fully_open, left_eye_fully_closed, right_eye_fully_open, right_eye_fully_closed,
    eye_sync_mode);
};

class PaperEyeTrackerWindow : public QWidget {
public:
    explicit PaperEyeTrackerWindow(QWidget *parent = nullptr);
    ~PaperEyeTrackerWindow() override;

    std::string getSSID() const;
    std::string getPassword() const;

    int get_max_fps() const;
    bool is_running() const;
    void updateBatteryStatus(int version);
    void set_config();
    void setIPText(int version, const QString& text) const;
    void start_image_download(int version) const;
    void setSerialStatusLabel(const QString& text) const;
    void setWifiStatusLabel(int version, const QString& text) const;

    void setVideoImage(int version, const cv::Mat& image);
    void updateWifiLabel(int version) ;
    void updateSerialLabel(int version);
    cv::Mat getVideoImage(int version) const;

    Rect getRoiRect(int version);
    float getRotateAngle(int version) const;
    void startCalibration();
    void centerCalibration();
    PaperEyeTrackerConfig generate_config() const;
private slots:
    void onSendButtonClicked();
    void onRestartButtonClicked();
    void onFlashButtonClicked();
    void onEyeSyncModeChanged(int index);
    void onEnergyModeChanged(int index);
    void onSendBrightnessValue();
    void calibrateEyeOpen();
    void calibrateEyeClose();
    void onLeftBrightnessChanged(int value);
    void onRightBrightnessChanged(int value);
    void onLeftRotateAngleChanged(int value);
    void onRightRotateAngleChanged(int value);
    void onLeftEyeValueIncrease();
    void onLeftEyeValueDecrease();
    void onRightEyeValueIncrease();
    void onRightEyeValueDecrease();
private:
    void initUI();
    void initLayout();
    void retranslateUI();
    void connect_callbacks();

    double eye_fully_open[EYE_NUM] = {30.0, 30.0};    // 默认值
    double eye_fully_closed[EYE_NUM] = {10.0, 10.0};  // 默认值
    void create_sub_thread();
    void launchETVR();
    void setFixedWidthBasedONLongestText(QWidget* widget, const QStringList& texts);
    enum EyeSyncMode {
        NO_SYNC = 0,       // 双眼独立控制
        LEFT_CONTROLS = 1, // 左眼控制双眼
        RIGHT_CONTROLS = 2 // 右眼控制双眼
    };
    EyeSyncMode eyeSyncMode = NO_SYNC;
    QComboBox* eyeSyncComboBox = nullptr;
    void bound_pages();
    // 记录校准状态
    bool calibrated_position = false;
    bool calibrated_center = false;
    bool calibrated_eye_open = false;
    bool calibrated_eye_close = false;

    // 更新按钮状态的方法
    void updateCalibrationButtonStates();

    std::shared_ptr<ESP32VideoStream> image_stream[EYE_NUM];
    std::shared_ptr<SerialPortManager> serial_port_;
    std::shared_ptr<OscManager> osc_manager;
    std::shared_ptr<EyeInference> inference_[EYE_NUM];

    // 2 is left, 3 is right
    int current_esp32_version = 0;

    inline static PaperEyeTrackerWindow* instance = nullptr;

    std::thread update_ui_thread[EYE_NUM];
    std::thread osc_send_thread;
    std::thread inference_thread[EYE_NUM];
    bool app_is_running = true;
    int max_fps = 38;

    std::string current_ip[EYE_NUM];

    int brightness[EYE_NUM];
    std::string current_firmware_type = "face_tracker";

    std::shared_ptr<ConfigWriter> config_writer;
    PaperEyeTrackerConfig config;

    std::shared_ptr<QTimer> brightness_timer[EYE_NUM];

    Rect roi_rect[EYE_NUM] = {
        Rect {0, 0, 261, 261},
        Rect {0, 0, 261, 261}
    };
    int current_rotate_angle[EYE_NUM] = {0};

    std::vector<float> outputs[EYE_NUM] = {};
    std::mutex outputs_mutex[EYE_NUM];
    std::mutex results_mutex[EYE_NUM];

    float calibration_percentile_90[EYE_NUM];
    float calibration_percentile_2[EYE_NUM];
    std::vector<double> open_list[EYE_NUM];

    bool calibrated[EYE_NUM] = {false, false};

    double eye_open[EYE_NUM];
    double last_eye_open[EYE_NUM] = {};
    cv::Point2f pupil[EYE_NUM];

    std::vector<double> out_y[EYE_NUM];

    const std::vector<std::string> left_blend_shapes = {
        "EyeLidLeft",
        "EyeLeftX",
        //"EyeY",
    };
    const std::vector<std::string> right_blend_shapes = {
        "EyeLidRight",
        "EyeRightX",
        // "EyeY",
    };
    const std::vector<std::vector<std::string>> blend_shapes = {
        left_blend_shapes,
        right_blend_shapes
    };
    struct EyeCalibData {
        double calib_XMIN = 9999999;
        double calib_XMAX = -9999999;
        double calib_YMIN = 9999999;
        double calib_YMAX = -9999999;
        double calib_XOFF = 0;
        double calib_YOFF = 0;
        bool has_calibration = false;
    };
    EyeCalibData eye_calib_data[EYE_NUM];
    bool flip_x_axis[EYE_NUM] = {false, false};  // 控制X轴翻转
    bool flip_y_axis = false;  // 控制Y轴翻转
    bool is_calibrating = false;
    int calibration_frame_count = 0;
    int calibration_eye_index = -1; // 当前校准的眼睛索引
    const int CALIBRATION_FRAME_MAX = 2000;  // 收集100帧用于校准
    // 用于声音提示的 QSoundEffect 对象
    // QSoundEffect* startSound;
    // QSoundEffect* endSound;
    QTimer* auto_save_timer= nullptr;
    // 用于绘制眼睛位置的自定义小部件
    class EyePositionWidget : public QWidget {
    public:
        explicit EyePositionWidget(QWidget* parent = nullptr) : QWidget(parent), x(0.5), y(0.5), openness(1.0) {}

        void setPosition(double newX, double newY) {
            x = max(0.0, min(1.0, newX));
            y = max(0.0, min(1.0, newY));
            update();
        }

        void setOpenness(double newOpenness) {
            openness = max(0.0, min(1.0, newOpenness));
            update();
        }

    protected:
        void paintEvent(QPaintEvent*) override {
            QPainter painter(this);

            // 绘制背景
            painter.fillRect(rect(), Qt::lightGray);

            // 绘制中心十字线
            painter.setPen(Qt::gray);
            painter.drawLine(width()/2, 0, width()/2, height());
            painter.drawLine(0, height()/2, width(), height()/2);

            // 计算眼球位置
            int eyeX = static_cast<int>(x * width());
            int eyeY = static_cast<int>(y * height());

            // 计算眼球大小和颜色（基于开合度）
            int eyeSize = 20;
            QColor eyeColor = QColor(0, 0, 255, static_cast<int>(openness * 255));

            // 绘制眼球
            painter.setBrush(eyeColor);
            painter.setPen(Qt::black);
            painter.drawEllipse(QPoint(eyeX, eyeY), eyeSize, eyeSize);
        }

    private:
        double x, y;     // 眼球位置 (0-1 范围)
        double openness; // 眼睛开合度 (0-1 范围)
    };

    // 左右眼位置显示控件
    EyePositionWidget* leftEyePositionWidget;
    EyePositionWidget* rightEyePositionWidget;

    // 校准相关函数
    void calibrateEye(int eyeIndex);
    void centerEye(int eyeIndex);
    void updateEyePosition(int eyeIndex);
    void finishCalibration(int eyeIndex, QProgressBar* progressBar, QTimer* updateTimer);
    void updatePageWidth();

    QStackedWidget *stackedWidget;
    QWidget *page;
    QPlainTextEdit *RightEyeIPAddress;
    QLabel *label_2;
    QPlainTextEdit *LeftEyeIPAddress;
    QComboBox *EnergyModelBox;
    QPlainTextEdit *LogText;
    QLabel *LeftEyeImage;
    QPushButton *RestartButton;
    QLabel *label;
    QPlainTextEdit *PassWordInput;
    QPlainTextEdit *SSIDInput;
    QLabel *RightEyeImage;
    QLabel *label_3;
    QPushButton *FlashButton;
    QPushButton *SendButton;
    QScrollBar *LeftBrightnessBar;
    QScrollBar *RightBrightnessBar;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *RightEyeBatteryLabel;
    QLabel *LeftEyeBatteryLabel;
    QLabel *LeftRotateLabel;
    QLabel *RightRotateLabel;
    QScrollBar *LeftRotateBar;
    QScrollBar *RightRotateBar;
    QWidget *page_2;
    QLabel *LeftEyeTrackingLabel;
    QLabel *RightEyeTrackingLabel;
    QFrame *LeftEyePositionFrame;
    QFrame *RightEyePositionFrame;
    QProgressBar *LeftEyeOpennessBar;
    QProgressBar *RightEyeOpennessBar;
    QLabel *LeftEyeOpennessLabel;
    QLabel *RightEyeOpennessLabel;
    QPushButton *settingsCalibrateButton;
    QPushButton *settingsCenterButton;
    QPushButton *settingsEyeOpenButton;
    QPushButton *settingsEyeCloseButton;
    QPushButton *MainPageButton;
    QPushButton *SettingButton;
    QLabel *RightEyeWifiStatus;
    QLabel *EyeWindowSerialStatus;
    QLabel *LeftEyeWifiStatus;
    QLabel *leftEyeVideoStreamLabel;
    QLabel *rightEyeVideoStreamLabel;
    QLabel *leftAdjustLabel;
    QLabel *rightAdjustLabel;
    QPushButton *leftIncButton;
    QPushButton *leftDecButton;
    QPushButton *rightIncButton;
    QPushButton *rightDecButton;
};


#endif //PAPER_EYE_TRACKER_WINDOW_HPP
