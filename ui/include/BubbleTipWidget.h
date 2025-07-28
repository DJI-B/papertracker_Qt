// BubbleTipWidget.h
#ifndef BUBBLETIPWIDGET_H
#define BUBBLETIPWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPixmap>

class BubbleTipWidget : public QWidget
{
    Q_OBJECT

public:
    enum ArrowDirection {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        LeftTop,
        LeftBottom,
        RightTop,
        RightBottom
    };

    explicit BubbleTipWidget(QWidget *parent = nullptr);

    // 设置标题
    void setTitle(const QString &title);

    // 设置提示文本
    void setText(const QString &text);

    // 设置图片
    void setImage(const QPixmap &pixmap);

    // 设置位置
    void setPosition(int x, int y);

    // 设置自动消失时间（毫秒），0表示不自动消失
    void setAutoHideDelay(int delayMs);

    // 设置是否启用hover时隐藏
    void setHoverToHide(bool enable);

    // 设置箭头方向
    void setArrowDirection(ArrowDirection direction);

    // 显示气泡
    void showBubble();

    // 隐藏气泡（带动画效果）
    void hideBubble();

protected:
    // 重写绘制事件来绘制带箭头的气泡
    void paintEvent(QPaintEvent *event) override;

    // 重写大小提示事件
    QSize sizeHint() const override;

    // 重写鼠标进入和离开事件处理hover效果
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

    // 重写鼠标按下事件处理拖拽
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onCloseButtonClicked();
    void onAutoHideTimeout();

private:
    void initUI();
    void setupAnimations();

    // UI组件
    QLabel *titleLabel;
    QTextEdit *contentTextEdit;
    QLabel *imageLabel;
    QPushButton *closeButton;

    // 功能组件
    QTimer *autoHideTimer;
    QPropertyAnimation *showAnimation;
    QPropertyAnimation *hideAnimation;

    // 状态变量
    bool hoverToShow;
    QPoint dragPosition;
    ArrowDirection arrowDirection;

    // 位置变量
    int customX;
    int customY;

    // 样式表
    static const char* bubbleStyle;
};

#endif // BUBBLETIPWIDGET_H
