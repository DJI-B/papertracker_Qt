//
// Created by JellyfishKnight on 25-3-1.
//

#ifndef ROI_EVENT_HPP
#define ROI_EVENT_HPP

#include <QObject>
#include <QPainter>


class ROIEventFilter final : public QObject {
public:
    explicit ROIEventFilter(std::function<void(QRect rect, bool is_end, int tag)> func, QObject *parent = nullptr, int tag = 1);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    QRect selectionRect;
    QPoint selectionStart;
    bool selecting;
    int tag_;

    std::function<void(QRect rect, bool is_end, int tag)> func;
};


#endif //ROI_EVENT_HPP
