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
class DropdownMenu;
// 在 main_window.h 中添加新的布局结构
class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QWidget *centralWidget;
    QHBoxLayout *mainLayout;

    // 侧边栏相关
    QWidget *sidebarWidget;
    QVBoxLayout *sidebarLayout;

    // 主内容区域
    QStackedWidget *contentStack;

    // 侧边栏项列表
    QList<QWidget*> sidebarItems;

    // 自定义标题栏相关
    QWidget *titleBarWidget;
    QHBoxLayout *titleBarLayout;
    QWidget *titleLeftArea;
    QWidget *titleRightArea;
    QHBoxLayout *titleRightLayout;
    QPushButton *minimizeButton;
    QPushButton *barsButton;
    QPushButton *bellButton;
    QPushButton *closeButton;
    QLabel *titleLabel;
    DropdownMenu *dropdownMenu;

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

private slots:
    void onMinimizeButtonClicked();
    void onCloseButtonClicked();

    void setupDropdownMenu();

    void addMenuItem(const QString &iconPath, const QString &text, const QString &arrowPath);

    void onBarsButtonClicked();
    void onBellButtonClicked();
};

class DropdownMenu : public QWidget
{
    Q_OBJECT

public:
    explicit DropdownMenu(QWidget *parent = nullptr);
    void addItem(QWidget *item);
protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVBoxLayout *menuLayout;
};

#endif //MAIN_WINDOW_NEW_H
