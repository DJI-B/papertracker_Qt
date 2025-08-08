#include "components/widget_component_base.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFrame>

WidgetComponentBase::WidgetComponentBase(QWidget *parent)
    : QWidget(parent)
{
    applyDefaultStyle();
}

void WidgetComponentBase::applyDefaultStyle()
{
    // 应用基本样式，但不影响按钮等控件
    setStyleSheet(
        "WidgetComponentBase {"
        "    background-color: #ffffff;"
        "    font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 1px solid #cccccc;"
        "    border-radius: 6px;"
        "    margin-top: 10px;"
        "    padding-top: 5px;"
        "    background-color: transparent;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 5px 0 5px;"
        "}"
    );
}

void WidgetComponentBase::setupScrollArea(QWidget *content)
{
    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setWidget(content);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea);
}
