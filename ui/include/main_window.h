//
// Created by JellyfishKnight on 25-3-11.
//

#ifndef PAPER_TRACKER_MAIN_WINDOW_HPP
#define PAPER_TRACKER_MAIN_WINDOW_HPP

#include <eye_tracker_window.hpp>
#include <face_tracker_window.hpp>
#include <QWidget>
#include <updater.hpp>
#include <QVBoxLayout>
#include <translator_manager.h>
#include <QMenuBar>

class PaperTrackerMainWindow : public QWidget {
public:
    explicit PaperTrackerMainWindow(QWidget *parent = nullptr);
    ~PaperTrackerMainWindow() override;

private slots:
    void onFaceTrackerButtonClicked();
    void onEyeTrackerButtonClicked();
    void onUpdateButtonClicked();
    void onRestartVRCFTButtonClicked();
    void onSettingsButtonClicked();
    void onAboutClicked();

private:
    void initUI();
    void initLayout();
    void connect_callbacks();
    void retranslateUi();
    void updateStatusLabel();
    std::shared_ptr<Updater> updater;
    QProcess* vrcftProcess = nullptr; // 添加这一行

    QPushButton* FaceTrackerButton;
    QPushButton* EyeTrackerButton;
    QLabel* LOGOLabel;
    QPushButton* ClientUpdateButton;
    QLabel* ClientStatusLabel;
    QPushButton* RestartVRCFTButton;
    QLabel* FaceTrackerInstructionLabel;
    QLabel* EyeTrackerInstructionLabel;
    QLabel* ClientStatusLabel_2;

    QVBoxLayout* mainLayout;
    QHBoxLayout* topButtonLayout;
    QHBoxLayout* buttonLayout;
    QHBoxLayout* bottomLayout;
    QVBoxLayout* statusLayout;

    // 新增菜单栏组件
    QMenuBar* menuBar;          // 菜单栏容器
    QMenu* settingsMenu;        // 设置菜单
    QMenu* otherSettingsMenu;   // 其他设置菜单
    QMenu* helpMenu;            // 帮助菜单（可选）

    QString statusStr; // 状态标识符
    QString currentVersionTag; // 当前版本号
    QString latestVersionTag; // 最新版本号
    enum class ClientStatus {
        Unknown,
        ServerUnreachable,
        VersionUnknown,
        UpdateAvailable,
        UpToDate
    } clientStatus; // 状态枚举

    static constexpr const char* BUTTON_STYLE =
        "QPushButton {"
        "    font-size: 14pt;"
        "    padding: 10px;"
        "    border-radius: 8px;"
        "    background-color: #4CAF50;"
        "    color: white;"
        "}";

    static constexpr const char* HOVER_STYLE =
        "QPushButton:hover { background-color: #45a049; }";
};

#endif //PAPER_TRACKER_MAIN_WINDOW_HPP
