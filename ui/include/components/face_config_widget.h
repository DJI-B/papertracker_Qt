#ifndef FACE_CONFIG_WIDGET_H
#define FACE_CONFIG_WIDGET_H

#include "components/widget_component_base.h"
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>

/**
 * @brief 面部追踪配置组件
 * 负责面部追踪设备的配置界面，包括预览、亮度调节、旋转控制、性能模式、校准等
 */
class FaceConfigWidget : public WidgetComponentBase
{
    Q_OBJECT

public:
    explicit FaceConfigWidget(const QString &deviceType, QWidget *parent = nullptr);

    void setupUI() override;
    void retranslateUI() override;
    
    void updatePreviewImage(const QPixmap &pixmap);
    void updateDeviceIP(const QString &ipAddress);

signals:
    void brightnessChanged(int value);
    void rotationChanged(int value);
    void performanceModeChanged(int mode);
    void calibrationStarted();
    void calibrationStopped();
    void calibrationReset();

private slots:
    void onBrightnessChanged(int value);
    void onRotationChanged(int value);
    void onPerformanceModeChanged(int index);
    void onStartCalibration();
    void onStopCalibration();
    void onResetCalibration();

private:
    QString m_deviceType;
    
    // UI组件
    QLabel *m_titleLabel = nullptr;
    QLabel *m_descLabel = nullptr;
    
    // 图像预览
    QLabel *m_previewLabel = nullptr;
    
    // IP显示
    QLabel *m_ipDisplayLabel = nullptr;
    
    // 亮度调整
    QSlider *m_brightnessSlider = nullptr;
    QLabel *m_brightnessValueLabel = nullptr;
    
    // 旋转角度调整
    QSlider *m_rotationSlider = nullptr;
    QLabel *m_rotationValueLabel = nullptr;
    
    // 性能模式选择
    QComboBox *m_performanceModeComboBox = nullptr;
    
    // 校准控件
    QComboBox *m_calibrationModeComboBox = nullptr;
    QPushButton *m_startCalibrationButton = nullptr;
    QPushButton *m_stopCalibrationButton = nullptr;
    QPushButton *m_resetCalibrationButton = nullptr;
};

#endif // FACE_CONFIG_WIDGET_H
