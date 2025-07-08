/********************************************************************************
** Form generated from reading UI file 'eye_tracker_window.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EYE_TRACKER_WINDOW_H
#define UI_EYE_TRACKER_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PaperEyeTrackerWindow
{
public:
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

    void setupUi(QWidget *PaperEyeTrackerWindow)
    {
        if (PaperEyeTrackerWindow->objectName().isEmpty())
            PaperEyeTrackerWindow->setObjectName("PaperEyeTrackerWindow");
        PaperEyeTrackerWindow->resize(907, 614);
        stackedWidget = new QStackedWidget(PaperEyeTrackerWindow);
        stackedWidget->setObjectName("stackedWidget");
        stackedWidget->setGeometry(QRect(0, 40, 901, 571));
        page = new QWidget();
        page->setObjectName("page");
        RightEyeIPAddress = new QPlainTextEdit(page);
        RightEyeIPAddress->setObjectName("RightEyeIPAddress");
        RightEyeIPAddress->setGeometry(QRect(530, 340, 201, 41));
        label_2 = new QLabel(page);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(470, 350, 54, 16));
        LeftEyeIPAddress = new QPlainTextEdit(page);
        LeftEyeIPAddress->setObjectName("LeftEyeIPAddress");
        LeftEyeIPAddress->setGeometry(QRect(530, 290, 201, 41));
        EnergyModelBox = new QComboBox(page);
        EnergyModelBox->addItem(QString());
        EnergyModelBox->addItem(QString());
        EnergyModelBox->addItem(QString());
        EnergyModelBox->setObjectName("EnergyModelBox");
        EnergyModelBox->setGeometry(QRect(800, 300, 91, 31));
        LogText = new QPlainTextEdit(page);
        LogText->setObjectName("LogText");
        LogText->setGeometry(QRect(20, 400, 721, 151));
        LeftEyeImage = new QLabel(page);
        LeftEyeImage->setObjectName("LeftEyeImage");
        LeftEyeImage->setGeometry(QRect(10, 5, 261, 261));
        RestartButton = new QPushButton(page);
        RestartButton->setObjectName("RestartButton");
        RestartButton->setGeometry(QRect(330, 290, 111, 41));
        label = new QLabel(page);
        label->setObjectName("label");
        label->setGeometry(QRect(470, 300, 54, 16));
        PassWordInput = new QPlainTextEdit(page);
        PassWordInput->setObjectName("PassWordInput");
        PassWordInput->setGeometry(QRect(20, 340, 171, 41));
        SSIDInput = new QPlainTextEdit(page);
        SSIDInput->setObjectName("SSIDInput");
        SSIDInput->setGeometry(QRect(20, 290, 171, 41));
        RightEyeImage = new QLabel(page);
        RightEyeImage->setObjectName("RightEyeImage");
        RightEyeImage->setGeometry(QRect(280, 5, 261, 261));
        label_3 = new QLabel(page);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(740, 300, 51, 31));
        FlashButton = new QPushButton(page);
        FlashButton->setObjectName("FlashButton");
        FlashButton->setGeometry(QRect(330, 340, 111, 41));
        SendButton = new QPushButton(page);
        SendButton->setObjectName("SendButton");
        SendButton->setGeometry(QRect(210, 290, 101, 91));
        LeftBrightnessBar = new QScrollBar(page);
        LeftBrightnessBar->setObjectName("LeftBrightnessBar");
        LeftBrightnessBar->setGeometry(QRect(660, 40, 231, 21));
        LeftBrightnessBar->setOrientation(Qt::Orientation::Horizontal);
        RightBrightnessBar = new QScrollBar(page);
        RightBrightnessBar->setObjectName("RightBrightnessBar");
        RightBrightnessBar->setGeometry(QRect(660, 80, 231, 21));
        RightBrightnessBar->setOrientation(Qt::Orientation::Horizontal);
        label_4 = new QLabel(page);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(580, 35, 54, 31));
        label_5 = new QLabel(page);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(580, 80, 54, 21));
        RightEyeBatteryLabel = new QLabel(page);
        RightEyeBatteryLabel->setObjectName("RightEyeBatteryLabel");
        RightEyeBatteryLabel->setGeometry(QRect(780, 240, 101, 31));
        QFont font;
        font.setBold(true);
        font.setItalic(true);
        RightEyeBatteryLabel->setFont(font);
        LeftEyeBatteryLabel = new QLabel(page);
        LeftEyeBatteryLabel->setObjectName("LeftEyeBatteryLabel");
        LeftEyeBatteryLabel->setGeometry(QRect(640, 240, 101, 31));
        LeftEyeBatteryLabel->setFont(font);
        LeftRotateLabel = new QLabel(page);
        LeftRotateLabel->setObjectName("LeftRotateLabel");
        LeftRotateLabel->setGeometry(QRect(570, 120, 81, 20));
        RightRotateLabel = new QLabel(page);
        RightRotateLabel->setObjectName("RightRotateLabel");
        RightRotateLabel->setGeometry(QRect(570, 150, 71, 20));
        LeftRotateBar = new QScrollBar(page);
        LeftRotateBar->setObjectName("LeftRotateBar");
        LeftRotateBar->setGeometry(QRect(670, 120, 231, 16));
        LeftRotateBar->setMaximum(1080);
        LeftRotateBar->setPageStep(40);
        LeftRotateBar->setOrientation(Qt::Orientation::Horizontal);
        RightRotateBar = new QScrollBar(page);
        RightRotateBar->setObjectName("RightRotateBar");
        RightRotateBar->setGeometry(QRect(670, 150, 231, 16));
        RightRotateBar->setMaximum(1080);
        RightRotateBar->setPageStep(40);
        RightRotateBar->setOrientation(Qt::Orientation::Horizontal);
        stackedWidget->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName("page_2");
        LeftEyeTrackingLabel = new QLabel(page_2);
        LeftEyeTrackingLabel->setObjectName("LeftEyeTrackingLabel");
        LeftEyeTrackingLabel->setGeometry(QRect(30, 0, 121, 31));
        QFont font1;
        font1.setPointSize(12);
        font1.setBold(true);
        LeftEyeTrackingLabel->setFont(font1);
        RightEyeTrackingLabel = new QLabel(page_2);
        RightEyeTrackingLabel->setObjectName("RightEyeTrackingLabel");
        RightEyeTrackingLabel->setGeometry(QRect(480, 0, 121, 31));
        RightEyeTrackingLabel->setFont(font1);
        LeftEyePositionFrame = new QFrame(page_2);
        LeftEyePositionFrame->setObjectName("LeftEyePositionFrame");
        LeftEyePositionFrame->setGeometry(QRect(30, 40, 250, 250));
        LeftEyePositionFrame->setFrameShape(QFrame::Shape::StyledPanel);
        LeftEyePositionFrame->setFrameShadow(QFrame::Shadow::Raised);
        RightEyePositionFrame = new QFrame(page_2);
        RightEyePositionFrame->setObjectName("RightEyePositionFrame");
        RightEyePositionFrame->setGeometry(QRect(480, 40, 250, 250));
        RightEyePositionFrame->setFrameShape(QFrame::Shape::StyledPanel);
        RightEyePositionFrame->setFrameShadow(QFrame::Shadow::Raised);
        LeftEyeOpennessBar = new QProgressBar(page_2);
        LeftEyeOpennessBar->setObjectName("LeftEyeOpennessBar");
        LeftEyeOpennessBar->setGeometry(QRect(30, 330, 250, 25));
        LeftEyeOpennessBar->setValue(0);
        RightEyeOpennessBar = new QProgressBar(page_2);
        RightEyeOpennessBar->setObjectName("RightEyeOpennessBar");
        RightEyeOpennessBar->setGeometry(QRect(480, 330, 250, 25));
        RightEyeOpennessBar->setValue(0);
        LeftEyeOpennessLabel = new QLabel(page_2);
        LeftEyeOpennessLabel->setObjectName("LeftEyeOpennessLabel");
        LeftEyeOpennessLabel->setGeometry(QRect(40, 300, 121, 16));
        RightEyeOpennessLabel = new QLabel(page_2);
        RightEyeOpennessLabel->setObjectName("RightEyeOpennessLabel");
        RightEyeOpennessLabel->setGeometry(QRect(480, 300, 121, 16));
        settingsCalibrateButton = new QPushButton(page_2);
        settingsCalibrateButton->setObjectName("settingsCalibrateButton");
        settingsCalibrateButton->setGeometry(QRect(310, 100, 131, 51));
        settingsCenterButton = new QPushButton(page_2);
        settingsCenterButton->setObjectName("settingsCenterButton");
        settingsCenterButton->setGeometry(QRect(310, 160, 131, 51));
        settingsEyeOpenButton = new QPushButton(page_2);
        settingsEyeOpenButton->setObjectName("settingsEyeOpenButton");
        settingsEyeOpenButton->setGeometry(QRect(310, 220, 131, 51));
        settingsEyeCloseButton = new QPushButton(page_2);
        settingsEyeCloseButton->setObjectName("settingsEyeCloseButton");
        settingsEyeCloseButton->setGeometry(QRect(310, 280, 131, 51));
        stackedWidget->addWidget(page_2);
        MainPageButton = new QPushButton(PaperEyeTrackerWindow);
        MainPageButton->setObjectName("MainPageButton");
        MainPageButton->setGeometry(QRect(10, 10, 75, 24));
        SettingButton = new QPushButton(PaperEyeTrackerWindow);
        SettingButton->setObjectName("SettingButton");
        SettingButton->setGeometry(QRect(120, 10, 75, 24));
        RightEyeWifiStatus = new QLabel(PaperEyeTrackerWindow);
        RightEyeWifiStatus->setObjectName("RightEyeWifiStatus");
        RightEyeWifiStatus->setGeometry(QRect(680, 10, 161, 31));
        RightEyeWifiStatus->setFont(font);
        EyeWindowSerialStatus = new QLabel(PaperEyeTrackerWindow);
        EyeWindowSerialStatus->setObjectName("EyeWindowSerialStatus");
        EyeWindowSerialStatus->setGeometry(QRect(300, 15, 161, 21));
        EyeWindowSerialStatus->setFont(font);
        LeftEyeWifiStatus = new QLabel(PaperEyeTrackerWindow);
        LeftEyeWifiStatus->setObjectName("LeftEyeWifiStatus");
        LeftEyeWifiStatus->setGeometry(QRect(490, 10, 161, 31));
        LeftEyeWifiStatus->setFont(font);

        stackedWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(PaperEyeTrackerWindow);
    } // setupUi


};

namespace Ui {
    class PaperEyeTrackerWindow: public Ui_PaperEyeTrackerWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EYE_TRACKER_WINDOW_H
