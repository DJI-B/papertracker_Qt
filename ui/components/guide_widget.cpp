#include "components/guide_widget.h"
#include <QHBoxLayout>

GuideWidget::GuideWidget(const QString &title, int totalSteps, QWidget *parent)
    : QWidget(parent)
    , m_guideTitle(title)
    , m_totalSteps(totalSteps)
    , m_currentStep(1)
{
    setupUI();
}

void GuideWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 15, 20, 20);
    m_mainLayout->setSpacing(10);

    // 标题
    m_titleLabel = new QLabel(m_guideTitle);
    m_titleLabel->setObjectName("GuideTitle");
    m_titleLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #333; margin: 5px 0; }");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setMaximumHeight(35);

    // 进度条
    m_progressBar = new QProgressBar();
    m_progressBar->setObjectName("GuideProgressBar");
    m_progressBar->setRange(0, m_totalSteps);
    m_progressBar->setValue(m_currentStep);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(16);
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #ddd;"
        "    border-radius: 8px;"
        "    background-color: #f0f0f0;"
        "    height: 16px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #0070f9;"
        "    border-radius: 7px;"
        "}"
    );

    // 内容区域
    m_contentStack = new QStackedWidget();
    m_contentStack->setObjectName("GuideContentStack");
    m_contentStack->setContentsMargins(0, 5, 0, 5);

    // 按钮区域
    m_buttonArea = new QWidget();
    m_buttonArea->setObjectName("GuideButtonArea");
    m_buttonArea->setFixedHeight(50);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout(m_buttonArea);
    buttonLayout->setContentsMargins(0, 10, 0, 0);
    buttonLayout->setSpacing(10);

    m_prevButton = new QPushButton("Previous");
    m_prevButton->setObjectName("GuidePrevButton");
    m_prevButton->setFixedHeight(36);

    m_nextButton = new QPushButton("Next");
    m_nextButton->setObjectName("GuideNextButton");
    m_nextButton->setFixedHeight(36);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_prevButton);
    buttonLayout->addWidget(m_nextButton);

    // 连接信号
    connect(m_prevButton, &QPushButton::clicked, this, &GuideWidget::onPrevButtonClicked);
    connect(m_nextButton, &QPushButton::clicked, this, &GuideWidget::onNextButtonClicked);

    // 添加到主布局
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_progressBar);
    m_mainLayout->addWidget(m_contentStack, 1); // 给内容区域更大的空间权重
    m_mainLayout->addWidget(m_buttonArea);

    updateButtons();
}

void GuideWidget::setCurrentStep(int step)
{
    if (step >= 1 && step <= m_totalSteps) {
        m_currentStep = step;
        m_progressBar->setValue(step);
        m_contentStack->setCurrentIndex(step - 1);
        updateButtons();
        emit stepChanged(step);
    }
}

void GuideWidget::setStepContent(int step, QWidget *content)
{
    if (step >= 1 && step <= m_totalSteps && content) {
        // 确保有足够的页面
        while (m_contentStack->count() < step) {
            m_contentStack->addWidget(new QWidget());
        }
        
        // 移除旧的内容（如果有）
        if (m_contentStack->count() >= step) {
            QWidget *oldWidget = m_contentStack->widget(step - 1);
            m_contentStack->removeWidget(oldWidget);
            if (oldWidget && oldWidget->parent() == m_contentStack) {
                oldWidget->deleteLater();
            }
        }
        
        // 插入新内容
        m_contentStack->insertWidget(step - 1, content);
    }
}

void GuideWidget::onNextButtonClicked()
{
    if (m_currentStep < m_totalSteps) {
        setCurrentStep(m_currentStep + 1);
    } else {
        // 完成引导
        emit guidanceCompleted();
    }
}

void GuideWidget::onPrevButtonClicked()
{
    if (m_currentStep > 1) {
        setCurrentStep(m_currentStep - 1);
    } else {
        // 取消引导
        emit guidanceCancelled();
    }
}

void GuideWidget::updateButtons()
{
    m_prevButton->setEnabled(m_currentStep > 1);
    
    if (m_currentStep < m_totalSteps) {
        m_nextButton->setText("Next");
        m_nextButton->setEnabled(true);
    } else {
        m_nextButton->setText("Finish");
        m_nextButton->setEnabled(true);
    }
}
