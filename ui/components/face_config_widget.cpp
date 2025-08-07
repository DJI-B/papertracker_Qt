#include "components/face_config_widget.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

FaceConfigWidget::FaceConfigWidget(const QString &deviceType, QWidget *parent)
    : WidgetComponentBase(parent)
    , m_deviceType(deviceType)
{
    setupUI();
    retranslateUI();
}

void FaceConfigWidget::setupUI()
{
    setObjectName("FaceConfigContent");

    // 创建滚动区域
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("FaceConfigScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);

    // 创建内容容器
    QWidget *contentContainer = new QWidget();
    contentContainer->setObjectName("FaceConfigContentContainer");
    contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout *mainLayout = new QVBoxLayout(contentContainer);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    // 标题
    m_titleLabel = new QLabel();
    m_titleLabel->setObjectName("FaceConfigTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setFixedHeight(32);

    // 描述
    m_descLabel = new QLabel();
    m_descLabel->setObjectName("FaceConfigDesc");
    m_descLabel->setAlignment(Qt::AlignCenter);
    m_descLabel->setWordWrap(true);
    m_descLabel->setFixedHeight(40);

    // 创建主要内容布局
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // 左侧：图像预览区域
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(10);

    // 图像预览标签
    m_previewLabel = new QLabel();
    m_previewLabel->setObjectName("FacePreviewLabel");
    m_previewLabel->setFixedSize(280, 280);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet(
        "QLabel#FacePreviewLabel {"
        "    border: 2px solid #e0e0e0;"
        "    border-radius: 8px;"
        "    background-color: #f8f8f8;"
        "    color: #666;"
        "    font-size: 14px;"
        "}"
    );
    m_previewLabel->setText("Preview Image\n(280 x 280)");

    // IP显示控件
    QGroupBox *ipGroupBox = new QGroupBox();
    ipGroupBox->setObjectName("IPGroupBox");
    ipGroupBox->setTitle("Device Information");
    ipGroupBox->setFixedHeight(60);

    QHBoxLayout *ipLayout = new QHBoxLayout(ipGroupBox);
    ipLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *ipLabel = new QLabel("IP Address:");
    ipLabel->setObjectName("IPLabel");
    ipLabel->setStyleSheet("QLabel { font-size: 12px; color: #333; }");

    m_ipDisplayLabel = new QLabel("192.168.1.100");
    m_ipDisplayLabel->setObjectName("IPDisplayLabel");
    m_ipDisplayLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #0070f9;"
        "    font-weight: bold;"
        "    background-color: #f0f8ff;"
        "    padding: 4px 8px;"
        "    border-radius: 4px;"
        "    border: 1px solid #cce7ff;"
        "}"
    );

    ipLayout->addWidget(ipLabel);
    ipLayout->addWidget(m_ipDisplayLabel);
    ipLayout->addStretch();

    leftLayout->addWidget(m_previewLabel);
    leftLayout->addWidget(ipGroupBox);
    leftLayout->addStretch();

    // 右侧：控制面板
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(15);

    // 亮度调整控件
    QGroupBox *brightnessGroupBox = new QGroupBox();
    brightnessGroupBox->setObjectName("BrightnessGroupBox");
    brightnessGroupBox->setTitle("Brightness Control");
    brightnessGroupBox->setFixedHeight(80);

    QVBoxLayout *brightnessLayout = new QVBoxLayout(brightnessGroupBox);
    brightnessLayout->setContentsMargins(10, 10, 10, 10);
    brightnessLayout->setSpacing(8);

    QHBoxLayout *brightnessControlLayout = new QHBoxLayout();

    QLabel *brightnessMinLabel = new QLabel("Dark");
    brightnessMinLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setObjectName("BrightnessSlider");
    m_brightnessSlider->setRange(0, 100);
    m_brightnessSlider->setValue(50);
    m_brightnessSlider->setFixedHeight(20);

    QLabel *brightnessMaxLabel = new QLabel("Bright");
    brightnessMaxLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_brightnessValueLabel = new QLabel("50%");
    m_brightnessValueLabel->setObjectName("BrightnessValueLabel");
    m_brightnessValueLabel->setFixedWidth(35);
    m_brightnessValueLabel->setAlignment(Qt::AlignCenter);
    m_brightnessValueLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #333;"
        "    font-weight: bold;"
        "    background-color: #f0f0f0;"
        "    border-radius: 4px;"
        "    padding: 2px;"
        "}"
    );

    connect(m_brightnessSlider, &QSlider::valueChanged, this, [this](int value) {
        m_brightnessValueLabel->setText(QString("%1%").arg(value));
        emit brightnessChanged(value);
    });

    brightnessControlLayout->addWidget(brightnessMinLabel);
    brightnessControlLayout->addWidget(m_brightnessSlider);
    brightnessControlLayout->addWidget(brightnessMaxLabel);
    brightnessControlLayout->addWidget(m_brightnessValueLabel);

    brightnessLayout->addLayout(brightnessControlLayout);

    // 旋转角度调整控件
    QGroupBox *rotationGroupBox = new QGroupBox();
    rotationGroupBox->setObjectName("RotationGroupBox");
    rotationGroupBox->setTitle("Rotation Control");
    rotationGroupBox->setFixedHeight(80);

    QVBoxLayout *rotationLayout = new QVBoxLayout(rotationGroupBox);
    rotationLayout->setContentsMargins(10, 10, 10, 10);
    rotationLayout->setSpacing(8);

    QHBoxLayout *rotationControlLayout = new QHBoxLayout();

    QLabel *rotationMinLabel = new QLabel("0°");
    rotationMinLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_rotationSlider = new QSlider(Qt::Horizontal);
    m_rotationSlider->setObjectName("RotationSlider");
    m_rotationSlider->setRange(0, 360);
    m_rotationSlider->setValue(0);
    m_rotationSlider->setFixedHeight(20);

    QLabel *rotationMaxLabel = new QLabel("360°");
    rotationMaxLabel->setStyleSheet("QLabel { font-size: 10px; color: #666; }");

    m_rotationValueLabel = new QLabel("0°");
    m_rotationValueLabel->setObjectName("RotationValueLabel");
    m_rotationValueLabel->setFixedWidth(35);
    m_rotationValueLabel->setAlignment(Qt::AlignCenter);
    m_rotationValueLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #333;"
        "    font-weight: bold;"
        "    background-color: #f0f0f0;"
        "    border-radius: 4px;"
        "    padding: 2px;"
        "}"
    );

    connect(m_rotationSlider, &QSlider::valueChanged, this, [this](int value) {
        m_rotationValueLabel->setText(QString("%1°").arg(value));
        emit rotationChanged(value);
    });

    rotationControlLayout->addWidget(rotationMinLabel);
    rotationControlLayout->addWidget(m_rotationSlider);
    rotationControlLayout->addWidget(rotationMaxLabel);
    rotationControlLayout->addWidget(m_rotationValueLabel);

    rotationLayout->addLayout(rotationControlLayout);

    // 性能模式选择控件
    QGroupBox *performanceGroupBox = new QGroupBox();
    performanceGroupBox->setObjectName("PerformanceGroupBox");
    performanceGroupBox->setTitle("Performance Mode");
    performanceGroupBox->setFixedHeight(65);

    QVBoxLayout *performanceLayout = new QVBoxLayout(performanceGroupBox);
    performanceLayout->setContentsMargins(10, 10, 10, 10);

    m_performanceModeComboBox = new QComboBox();
    m_performanceModeComboBox->setObjectName("PerformanceModeComboBox");
    m_performanceModeComboBox->setFixedHeight(30);
    m_performanceModeComboBox->addItem("Normal Mode");
    m_performanceModeComboBox->addItem("Power Saving Mode");
    m_performanceModeComboBox->addItem("Performance Mode");
    m_performanceModeComboBox->setCurrentIndex(0);

    connect(m_performanceModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FaceConfigWidget::onPerformanceModeChanged);

    performanceLayout->addWidget(m_performanceModeComboBox);

    // 校准按钮组
    QGroupBox *calibrationGroupBox = new QGroupBox();
    calibrationGroupBox->setObjectName("CalibrationGroupBox");
    calibrationGroupBox->setTitle("Calibration Controls");
    calibrationGroupBox->setFixedHeight(120);

    QVBoxLayout *calibrationLayout = new QVBoxLayout(calibrationGroupBox);
    calibrationLayout->setContentsMargins(10, 10, 10, 10);
    calibrationLayout->setSpacing(8);

    // 校准模式选择
    QHBoxLayout *calibrationModeLayout = new QHBoxLayout();

    QLabel *calibrationModeLabel = new QLabel("Mode:");
    calibrationModeLabel->setStyleSheet("QLabel { font-size: 12px; color: #333; }");
    calibrationModeLabel->setFixedWidth(40);

    m_calibrationModeComboBox = new QComboBox();
    m_calibrationModeComboBox->setObjectName("CalibrationModeComboBox");
    m_calibrationModeComboBox->setFixedHeight(28);
    m_calibrationModeComboBox->addItem("Quick Calibration");
    m_calibrationModeComboBox->addItem("Standard Calibration");
    m_calibrationModeComboBox->addItem("Precision Calibration");
    m_calibrationModeComboBox->setCurrentIndex(1);

    calibrationModeLayout->addWidget(calibrationModeLabel);
    calibrationModeLayout->addWidget(m_calibrationModeComboBox);

    QHBoxLayout *calibrationButtonLayout = new QHBoxLayout();
    calibrationButtonLayout->setSpacing(8);

    m_startCalibrationButton = new QPushButton();
    m_startCalibrationButton->setObjectName("StartCalibrationButton");
    m_startCalibrationButton->setFixedHeight(32);
    m_startCalibrationButton->setText("Start");

    m_stopCalibrationButton = new QPushButton();
    m_stopCalibrationButton->setObjectName("StopCalibrationButton");
    m_stopCalibrationButton->setFixedHeight(32);
    m_stopCalibrationButton->setText("Stop");
    m_stopCalibrationButton->setEnabled(false);

    m_resetCalibrationButton = new QPushButton();
    m_resetCalibrationButton->setObjectName("ResetCalibrationButton");
    m_resetCalibrationButton->setFixedHeight(32);
    m_resetCalibrationButton->setText("Reset");

    connect(m_startCalibrationButton, &QPushButton::clicked, this, &FaceConfigWidget::onStartCalibration);
    connect(m_stopCalibrationButton, &QPushButton::clicked, this, &FaceConfigWidget::onStopCalibration);
    connect(m_resetCalibrationButton, &QPushButton::clicked, this, &FaceConfigWidget::onResetCalibration);

    calibrationButtonLayout->addWidget(m_startCalibrationButton);
    calibrationButtonLayout->addWidget(m_stopCalibrationButton);
    calibrationButtonLayout->addWidget(m_resetCalibrationButton);

    calibrationLayout->addLayout(calibrationModeLayout);
    calibrationLayout->addLayout(calibrationButtonLayout);

    // 添加所有控件到右侧布局
    rightLayout->addWidget(brightnessGroupBox);
    rightLayout->addWidget(rotationGroupBox);
    rightLayout->addWidget(performanceGroupBox);
    rightLayout->addWidget(calibrationGroupBox);
    rightLayout->addStretch();

    // 添加左右布局到内容布局
    contentLayout->addLayout(leftLayout, 1);
    contentLayout->addLayout(rightLayout, 1);

    // 添加到主布局
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(m_descLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(contentLayout);

    // 设置滚动区域
    scrollArea->setWidget(contentContainer);

    // 创建最终容器
    QVBoxLayout *finalLayout = new QVBoxLayout(this);
    finalLayout->setContentsMargins(0, 0, 0, 0);
    finalLayout->addWidget(scrollArea);
}

void FaceConfigWidget::retranslateUI()
{
    m_titleLabel->setText(tr("Face Tracker Configuration"));
    m_descLabel->setText(tr("Configure your face tracking device settings"));
}

void FaceConfigWidget::updatePreviewImage(const QPixmap &pixmap)
{
    if (m_previewLabel && !pixmap.isNull()) {
        QPixmap scaledPixmap = pixmap.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_previewLabel->setPixmap(scaledPixmap);
    }
}

void FaceConfigWidget::updateDeviceIP(const QString &ipAddress)
{
    if (m_ipDisplayLabel) {
        m_ipDisplayLabel->setText(ipAddress);
    }
}

void FaceConfigWidget::onBrightnessChanged(int value)
{
    emit brightnessChanged(value);
}

void FaceConfigWidget::onRotationChanged(int value)
{
    emit rotationChanged(value);
}

void FaceConfigWidget::onPerformanceModeChanged(int index)
{
    emit performanceModeChanged(index);
}

void FaceConfigWidget::onStartCalibration()
{
    m_startCalibrationButton->setEnabled(false);
    m_stopCalibrationButton->setEnabled(true);
    m_calibrationModeComboBox->setEnabled(false);
    emit calibrationStarted();
}

void FaceConfigWidget::onStopCalibration()
{
    m_startCalibrationButton->setEnabled(true);
    m_stopCalibrationButton->setEnabled(false);
    m_calibrationModeComboBox->setEnabled(true);
    emit calibrationStopped();
}

void FaceConfigWidget::onResetCalibration()
{
    emit calibrationReset();
}
