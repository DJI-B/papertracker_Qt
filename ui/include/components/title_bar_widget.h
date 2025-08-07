#ifndef TITLE_BAR_WIDGET_H
#define TITLE_BAR_WIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class CustomMenu;

/**
 * @brief 自定义标题栏组件
 * 负责窗口标题显示、窗口控制按钮（最小化、关闭）、菜单按钮等
 */
class TitleBarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBarWidget(QWidget *parent = nullptr);
    ~TitleBarWidget() override;

    void setTitle(const QString &title);
    void setupTitleBar();

signals:
    void minimizeRequested();
    void closeRequested();
    void menuRequested();
    void notificationRequested();

private slots:
    void onMinimizeButtonClicked();
    void onCloseButtonClicked();
    void onBarsButtonClicked();
    void onBellButtonClicked();
    void createCustomMenu();
    void destroyCustomMenu();

private:
    void setupButtons();
    void setupMenuItems();
    void addMenuItem(const QString &iconPath, const QString &text, const QString &rightIconPath = QString());
    void resetMenuItemsStyle();

    // UI 组件
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
};

/**
 * @brief 自定义菜单组件
 */
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
    QVBoxLayout *menuLayout = nullptr;
};

#endif // TITLE_BAR_WIDGET_H
