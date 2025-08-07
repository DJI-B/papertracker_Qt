#ifndef WIDGET_COMPONENT_BASE_H
#define WIDGET_COMPONENT_BASE_H

#include <QWidget>
#include <memory>

class SerialPortManager;

/**
 * @brief 组件基类，提供通用的组件功能
 */
class WidgetComponentBase : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetComponentBase(QWidget *parent = nullptr);
    virtual ~WidgetComponentBase() = default;

    // 纯虚函数，子类必须实现
    virtual void setupUI() = 0;
    virtual void retranslateUI() = 0;

protected:
    // 通用的样式和布局辅助方法
    void applyDefaultStyle();
    void setupScrollArea(QWidget *content);

signals:
    void statusChanged(const QString &message);
    void errorOccurred(const QString &error);
};

#endif // WIDGET_COMPONENT_BASE_H
