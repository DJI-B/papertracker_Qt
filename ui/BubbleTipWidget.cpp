// BubbleTipWidget.cpp
#include "BubbleTipWidget.h"
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QStyleOption>
#include <QLayout>

// 气泡样式表
const char* BubbleTipWidget::bubbleStyle = R"(
    QWidget {
        background-color: transparent;
        border: none;
    }
    QLabel {
        color: #333333;
    }
    QTextEdit {
        border: none;
        background: transparent;
        color: #666666;
        font-size: 12px;
    }
    QPushButton {
        background-color: transparent;
        border: none;
        font-size: 16px;
        font-weight: bold;
        color: #999999;
    }
    QPushButton:hover {
        color: #FF0000;
    }
)";

BubbleTipWidget::BubbleTipWidget(QWidget *parent)
    : QWidget(parent)
    , hoverToShow(false)
    , arrowDirection(BottomRight)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    initUI();
    setupAnimations();

    // 初始化定时器
    autoHideTimer = new QTimer(this);
    autoHideTimer->setSingleShot(true);
    connect(autoHideTimer, &QTimer::timeout, this, &BubbleTipWidget::onAutoHideTimeout);

    // 设置样式
    setStyleSheet(bubbleStyle);

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(15);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    shadowEffect->setOffset(0, 0);
    setGraphicsEffect(shadowEffect);
}

void BubbleTipWidget::initUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20); // 增加边距以容纳箭头
    mainLayout->setSpacing(10);

    // 顶部标题栏
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(10, 10, 10, 0);
    titleLayout->setSpacing(5);

    titleLabel = new QLabel(this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    closeButton = new QPushButton("×", this);
    closeButton->setFixedSize(24, 24);
    titleLayout->addWidget(closeButton);

    connect(closeButton, &QPushButton::clicked, this, &BubbleTipWidget::onCloseButtonClicked);

    mainLayout->addLayout(titleLayout);

    // 内容区域
    contentTextEdit = new QTextEdit(this);
    contentTextEdit->setReadOnly(true);
    contentTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    contentTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    contentTextEdit->setMaximumHeight(100);
    contentTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(10, 0, 10, 10);
    contentLayout->addWidget(contentTextEdit);
    mainLayout->addLayout(contentLayout);

    // 图片显示区域
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel->hide();

    QHBoxLayout *imageLayout = new QHBoxLayout();
    imageLayout->setContentsMargins(10, 0, 10, 10);
    imageLayout->addWidget(imageLabel);
    mainLayout->addLayout(imageLayout);

    setLayout(mainLayout);

    // 设置默认大小
    resize(300, 200);
}

void BubbleTipWidget::setupAnimations()
{
    // 显示动画
    showAnimation = new QPropertyAnimation(this, "windowOpacity");
    showAnimation->setDuration(300);
    showAnimation->setStartValue(0.0);
    showAnimation->setEndValue(1.0);

    // 隐藏动画
    hideAnimation = new QPropertyAnimation(this, "windowOpacity");
    hideAnimation->setDuration(300);
    hideAnimation->setStartValue(1.0);
    hideAnimation->setEndValue(0.0);
    connect(hideAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
}

void BubbleTipWidget::setTitle(const QString &title)
{
    titleLabel->setText(title);
}

void BubbleTipWidget::setText(const QString &text)
{
    contentTextEdit->setPlainText(text);
}

void BubbleTipWidget::setImage(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        imageLabel->hide();
    } else {
        imageLabel->setPixmap(pixmap.scaled(200, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imageLabel->show();
    }
}

void BubbleTipWidget::setPosition(int x, int y) {
    customX = x;
    customY = y;
}

void BubbleTipWidget::setArrowDirection(ArrowDirection direction) {
    arrowDirection = direction;
    update();
}

void BubbleTipWidget::setAutoHideDelay(int delayMs)
{
    if (delayMs > 0) {
        autoHideTimer->start(delayMs);
    } else {
        autoHideTimer->stop();
    }
}

void BubbleTipWidget::setHoverToHide(bool enable)
{
    hoverToShow = enable;
}

void BubbleTipWidget::showBubble()
{
    move(customX, customY);
    showAnimation->start();
    show();
}

void BubbleTipWidget::hideBubble()
{
    hideAnimation->start();
}

void BubbleTipWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取窗口矩形（减去边距）
    QRect rect = this->rect();
    int margin = 20;
    QRect bubbleRect = rect.adjusted(margin, margin, -margin, -margin);

    // 创建气泡路径
    QPainterPath bubblePath;

    // 根据箭头方向调整气泡形状
    QPoint arrowPoint;
    switch (arrowDirection) {
    case TopLeft:
        arrowPoint = QPoint(bubbleRect.left() + 20, bubbleRect.top());
        break;
    case TopRight:
        arrowPoint = QPoint(bubbleRect.right() - 20, bubbleRect.top());
        break;
    case BottomLeft:
        arrowPoint = QPoint(bubbleRect.left() + 20, bubbleRect.bottom());
        break;
    case BottomRight:
        arrowPoint = QPoint(bubbleRect.right() - 20, bubbleRect.bottom());
        break;
    case LeftTop:
        arrowPoint = QPoint(bubbleRect.left(), bubbleRect.top() + 20);
        break;
    case LeftBottom:
        arrowPoint = QPoint(bubbleRect.left(), bubbleRect.bottom() - 20);
        break;
    case RightTop:
        arrowPoint = QPoint(bubbleRect.right(), bubbleRect.top() + 20);
        break;
    case RightBottom:
        arrowPoint = QPoint(bubbleRect.right(), bubbleRect.bottom() - 20);
        break;
    }

    // 绘制气泡主体（带圆角矩形）
    bubblePath.addRoundedRect(bubbleRect, 10, 10);

    // 添加箭头
    QPainterPath arrowPath;
    QPoint p1, p2, p3;

    switch (arrowDirection) {
    case TopLeft:
    case TopRight:
        p1 = QPoint(arrowPoint.x() - 10, arrowPoint.y());
        p2 = QPoint(arrowPoint.x() + 10, arrowPoint.y());
        p3 = QPoint(arrowPoint.x(), arrowPoint.y() - 10);
        break;
    case BottomLeft:
    case BottomRight:
        p1 = QPoint(arrowPoint.x() - 10, arrowPoint.y());
        p2 = QPoint(arrowPoint.x() + 10, arrowPoint.y());
        p3 = QPoint(arrowPoint.x(), arrowPoint.y() + 10);
        break;
    case LeftTop:
    case LeftBottom:
        p1 = QPoint(arrowPoint.x(), arrowPoint.y() - 10);
        p2 = QPoint(arrowPoint.x(), arrowPoint.y() + 10);
        p3 = QPoint(arrowPoint.x() - 10, arrowPoint.y());
        break;
    case RightTop:
    case RightBottom:
        p1 = QPoint(arrowPoint.x(), arrowPoint.y() - 10);
        p2 = QPoint(arrowPoint.x(), arrowPoint.y() + 10);
        p3 = QPoint(arrowPoint.x() + 10, arrowPoint.y());
        break;
    }

    arrowPath.moveTo(p1);
    arrowPath.lineTo(p2);
    arrowPath.lineTo(p3);
    arrowPath.closeSubpath();

    // 合并气泡主体和箭头
    bubblePath = bubblePath.united(arrowPath);

    // 绘制气泡
    painter.fillPath(bubblePath, QBrush(QColor(255, 255, 255)));

    // 绘制边框
    painter.strokePath(bubblePath, QPen(QColor(204, 204, 204), 1));
}

QSize BubbleTipWidget::sizeHint() const
{
    QSize size = QWidget::sizeHint();
    // 增加边距以容纳箭头
    size.setWidth(size.width() + 40);
    size.setHeight(size.height() + 40);
    return size;
}

void BubbleTipWidget::enterEvent(QEnterEvent *event)
{
    if (hoverToShow) {
        hideBubble();
    }
    QWidget::enterEvent(event);
}

void BubbleTipWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
}

void BubbleTipWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void BubbleTipWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - dragPosition);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void BubbleTipWidget::onCloseButtonClicked()
{
    hideBubble();
}

void BubbleTipWidget::onAutoHideTimeout()
{
    hideBubble();
}