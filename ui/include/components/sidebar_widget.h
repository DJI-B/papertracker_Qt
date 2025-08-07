#ifndef SIDEBAR_WIDGET_H
#define SIDEBAR_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QList>
#include <QString>
#include <QMap>

/**
 * @brief 侧边栏组件
 * 负责管理侧边栏项目、设备标签页等
 */
class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);

    void createSidebarItem(const QString &iconPath, const QString &text);
    void createSidebarSeparator();
    
    // 设备管理方法
    void addDeviceTab(const QString &deviceName, const QString &deviceType);
    void removeDeviceTab(const QString &deviceName);
    void clearAllDeviceTabs();
    void updateDeviceType(const QString &deviceName, const QString &deviceType);
    
    void setSelectedItem(QWidget *selectedItem);

signals:
    void itemClicked(const QString &itemText);
    void deviceTabClicked(const QString &deviceName);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    QString getDeviceIcon(const QString &deviceType) const;

    // UI 组件
    QVBoxLayout *sidebarLayout = nullptr;
    
    // 数据存储
    QList<QWidget*> sidebarItems = {};
    QMap<QString, QWidget*> deviceTabs = {}; // 存储设备名称到侧边栏tab的映射
};

#endif // SIDEBAR_WIDGET_H
