//
// Created by JellyfishKnight on 25-3-1.
//
/*
 * PaperTracker - 面部追踪应用程序
 * Copyright (C) 2025 PAPER TRACKER
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file contains code from projectbabble:
 * Copyright 2023 Sameer Suri
 * Licensed under the Apache License, Version 2.0
 */
#include "roi_event.hpp"

#include <QLabel>
#include <QMouseEvent>
#include <utility>
#include <QRect>

ROIEventFilter::ROIEventFilter(std::function<void(QRect rect, bool is_end, int tag)> func, QObject *parent, int tag)
    : QObject(parent), selecting(false), tag_(tag)
{
    this->func = std::move(func);
}

bool ROIEventFilter::eventFilter(QObject *obj, QEvent *event) {
    auto *label = qobject_cast<QLabel*>(obj);
    if (!label)
        return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            selecting = true;
            selectionStart = mouseEvent->pos();
            selectionRect = QRect(selectionStart, QSize());
            return true;  // 拦截事件
        }
    } else if (event->type() == QEvent::MouseMove && selecting) {
        auto *mouseEvent = dynamic_cast<QMouseEvent*>(event);
        selectionRect = QRect(selectionStart, mouseEvent->pos()).normalized();
        func(selectionRect, false, tag_);
        label->update();  // 触发重绘，绘制选区
        return true;
    } else if (event->type() == QEvent::MouseButtonRelease && selecting) {
        auto *mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            selecting = false;
            selectionRect = QRect(selectionStart, mouseEvent->pos()).normalized();
            func(selectionRect, true, tag_);
            label->update();
            return true;
        }
    } else if (event->type() == QEvent::Paint) {
        // 首先让 QLabel 完成它自己的绘制
        bool ret = QObject::eventFilter(obj, event);
        // 然后在其上面绘制 ROI 选区
        QPainter painter(label);
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        if (!selectionRect.isNull()) {
            painter.drawRect(selectionRect);
        }
        return ret;
    }
    return QObject::eventFilter(obj, event);
}
