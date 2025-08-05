//
// Created by colorful on 25-7-28.
//

#ifndef MAIN_WINDOW_NEW_H
#define MAIN_WINDOW_NEW_H

#include <QMainWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QList>
#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QEvent>
#include "eye_tracker_window.hpp"
#include "face_tracker_window.hpp"
#include "updater.hpp"
#include "translator_manager.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QListWidget>

class CustomMenu;
class GuideWidget;
// 在 main_window.h 中添加新的布局结构
class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QWidget *centralWidget;
    QHBoxLayout *mainLayout = nullptr;

    // 侧边栏相关
    QWidget *sidebarWidget = nullptr;
    QVBoxLayout *sidebarLayout = nullptr;

    // 主内容区域
    QStackedWidget *contentStack = nullptr;

    // 侧边栏项列表
    QList<QWidget*> sidebarItems = {};

    // 自定义标题栏相关
    QWidget *titleBarWidget = nullptr;
    QHBoxLayout *titleBarLayout = nullptr;
    QWidget *titleLeftArea = nullptr;
    QWidget *titleRightArea = nullptr;
    QHBoxLayout *titleRightLayout = nullptr;
    QPushButton *minimizeButton = nullptr;
    QPushButton *barsButton = nullptr;
    QPushButton *bellButton = nullptr;
    QPushButton *closeButton = nullptr;
    QLabel *titleLabel = nullptr;
    CustomMenu *customMenu = nullptr;

    // 引导界面相关
    GuideWidget *faceGuideWidget = nullptr;
    GuideWidget *eyeGuideWidget = nullptr;
    QWidget *defaultContentWidget = nullptr;

    // 开始页面控件成员变量
    QLabel *welcomeLabel = nullptr;
    QLabel *descLabel = nullptr;
    QWidget *buttonContainer = nullptr;
    QHBoxLayout *buttonLayout = nullptr;
    QPushButton *faceTrackerButton = nullptr;
    QPushButton *eyeTrackerButton = nullptr;
    QWidget *faceButtonWidget = nullptr;
    QWidget *eyeButtonWidget = nullptr;
    QLabel *faceIcon = nullptr;
    QLabel *faceText = nullptr;
    QLabel *faceDesc = nullptr;
    QLabel *eyeIcon = nullptr;
    QLabel *eyeText = nullptr;
    QLabel *eyeDesc = nullptr;

    // 在 MainWindow 类中添加新的成员变量声明（需要在头文件中添加）
    QWidget *deviceListWidget = nullptr;
    QWidget *usbStatusWidget = nullptr;
    QListWidget *connectedDevicesList = nullptr;
    QLabel *usbStatusLabel = nullptr;
    QLabel *usbStatusIcon = nullptr;

    QWidget *wifiContentWidget = nullptr;
    QScrollArea *wifiScrollArea = nullptr;
    QWidget *wifiContentContainer = nullptr;
    QLabel *wifiTitleLabel = nullptr;
    QLabel *wifiDescLabel = nullptr;
    QGroupBox *wifiGroupBox = nullptr;
    QGroupBox *historyGroupBox = nullptr;
    QLineEdit *wifiNameEdit = nullptr;
    QLineEdit *wifiPasswordEdit = nullptr;
    QCheckBox *showPasswordCheckBox = nullptr;
    QLabel *wifiStatusLabel = nullptr;
    QPushButton *clearButton = nullptr;
    QPushButton *sendConfigButton = nullptr;
    QListWidget *historyListWidget = nullptr;
    QPushButton *clearHistoryButton = nullptr;
    QLabel *deviceStatusLabel = nullptr;
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setupUI();
    void createSidebarItem(const QString &iconPath, const QString &text);

    bool eventFilter(QObject *obj, QEvent *event);

protected:
    void setSelectedItem(QWidget *selectedItem);

    // 处理窗口移动
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QPoint dragPosition;

    // 初始化自定义标题栏
    void setupTitleBar();

        // 创建内容页面
    void createContentPages();
    void createDefaultContent();
     // 创建 WiFi 设置步骤的方法
    QWidget* createWifiSetupStep(const QString &deviceType);
    QWidget* createFaceStep2Content();
    void updateFacePreviewImage(const QPixmap &pixmap);
    QWidget* createFaceConfigStep(const QString &deviceType);

    void createSidebarSeparator();
    void createDeviceStatusSection();
    void createConnectedDevicesSection();
    void addConnectedDevice(const QString &deviceName, const QString &status, bool isConnected);
    void updateDeviceCount();
    void updateUSBStatus(bool connected);
    void checkUSBConnection();
    void onWiFiConfigurationSuccess(const QString &deviceType, const QString &wifiName);

private slots:
    void onMinimizeButtonClicked();
    void onCloseButtonClicked();

    void onBarsButtonClicked();
    void onBellButtonClicked();

    void createCustomMenu();
    void addMenuItem(const QString &iconPath, const QString &text, const QString &rightIconPath = QString());
    void resetMenuItemsStyle();
    void destroyCustomMenu();

        // 侧边栏点击处理
    void onSidebarItemClicked(const QString &itemText);
};

// 引导界面类
class GuideWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GuideWidget(const QString &title, int totalSteps, QWidget *parent = nullptr);
    void setCurrentStep(int step);
    void setStepContent(int step, QWidget *content);

private slots:
    void onNextButtonClicked();
    void onPrevButtonClicked();

private:
    QString guideTitle;
    int totalSteps;
    int currentStep;
    
    QVBoxLayout *mainLayout;
    QLabel *titleLabel;
    QProgressBar *progressBar;
    QStackedWidget *contentStack;
    QWidget *buttonArea;
    QPushButton *prevButton;
    QPushButton *nextButton;
    
    void setupUI();
    void updateButtons();
};

class CustomMenu : public QWidget
{
    Q_OBJECT

public:
    explicit CustomMenu(QWidget *parent = nullptr);
    void addItem(QWidget *item);

protected:
    void paintEvent(QPaintEvent *event) override;
    void hideEvent(QHideEvent *event) override;

signals:
    void menuHidden();

private:
    QVBoxLayout *menuLayout;
};

// 在头文件中添加新的WiFi设置组件类
class WiFiSetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WiFiSetupWidget(const QString &deviceType, QWidget *parent = nullptr);
    void retranslateUI();

signals:
    void configurationSuccess(const QString &deviceType, const QString &wifiName);

private slots:
    void onSendConfigClicked();
    void onClearClicked();
    void onShowPasswordToggled(bool checked);
    void onHistoryItemClicked(QListWidgetItem *item);
    void onClearHistoryClicked();

private:
    void setupUI();
    void validateInputs();
    void addToHistory(const QString &wifiName);
    bool isWifiNameInHistory(const QString &wifiName);

    QString m_deviceType;
    
    // UI组件
    QLabel *m_titleLabel;
    QLabel *m_descLabel;
    QLabel *m_deviceStatusLabel;
    QGroupBox *m_wifiGroupBox;
    QGroupBox *m_historyGroupBox;
    QLineEdit *m_wifiNameEdit;
    QLineEdit *m_wifiPasswordEdit;
    QCheckBox *m_showPasswordCheckBox;
    QLabel *m_wifiStatusLabel;
    QPushButton *m_clearButton;
    QPushButton *m_sendConfigButton;
    QListWidget *m_historyListWidget;
    QPushButton *m_clearHistoryButton;
    QLabel *m_networkNameLabel;
    QLabel *m_passwordLabel;
    QLabel *m_statusLabel;
};

class FaceConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FaceConfigWidget(const QString &deviceType, QWidget *parent = nullptr);
    void retranslateUI();

signals:

private slots:

private:
    void setupUI();
    
    QString m_deviceType;
    
    // UI组件
    QLabel *m_titleLabel;
    QLabel *m_descLabel;
    
    // 图像预览
    QLabel *m_previewLabel;
    
    // IP显示
    QLabel *m_ipDisplayLabel;
    
    // 亮度调整
    QSlider *m_brightnessSlider;
    QLabel *m_brightnessValueLabel;
    
    // 旋转角度调整
    QSlider *m_rotationSlider;
    QLabel *m_rotationValueLabel;
    
    // 性能模式选择
    QComboBox *m_performanceModeComboBox;
    
    // 校准控件
    QComboBox *m_calibrationModeComboBox;
    QPushButton *m_startCalibrationButton;
    QPushButton *m_stopCalibrationButton;
    QPushButton *m_resetCalibrationButton;
};

#endif //MAIN_WINDOW_NEW_H
