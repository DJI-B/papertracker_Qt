#ifndef GUIDE_WIDGET_H
#define GUIDE_WIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QStackedWidget>
#include <QPushButton>

/**
 * @brief 引导界面组件
 * 负责显示多步骤的引导流程
 */
class GuideWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GuideWidget(const QString &title, int totalSteps, QWidget *parent = nullptr);

    void setCurrentStep(int step);
    void setStepContent(int step, QWidget *content);

signals:
    void stepChanged(int currentStep);
    void guidanceCompleted();
    void guidanceCancelled();

private slots:
    void onNextButtonClicked();
    void onPrevButtonClicked();

private:
    void setupUI();
    void updateButtons();

    QString m_guideTitle;
    int m_totalSteps;
    int m_currentStep;
    
    // UI组件
    QVBoxLayout *m_mainLayout = nullptr;
    QLabel *m_titleLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QWidget *m_buttonArea = nullptr;
    QPushButton *m_prevButton = nullptr;
    QPushButton *m_nextButton = nullptr;
};

#endif // GUIDE_WIDGET_H
