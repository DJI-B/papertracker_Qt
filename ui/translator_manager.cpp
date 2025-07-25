#include "translator_manager.h"
#include <QApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QDir>
#include <QLabel>
#include <QBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QSettings>

TranslatorManager::TranslatorManager(QObject* parent) : QObject(parent)
{
    // 初始化配置文件路径（以用户配置目录为例）
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir().mkpath(configPath); // 确保目录存在
    QString configFilePath = QDir(configPath).filePath("config.ini");

    // 读取配置
    QSettings settings(configFilePath, QSettings::IniFormat);
    QString savedLang = settings.value("Language/Current", "en_US").toString();
    setLanguage(savedLang);
}

TranslatorManager::~TranslatorManager() {
    QApplication::removeTranslator(&m_translator);
}

void TranslatorManager::setLanguage(const QString& langCode) {
    if (m_currentLang == langCode) return;

    // 卸载旧翻译
    QApplication::removeTranslator(&m_translator);

    // 构建翻译文件路径（需确保文件存在 ./translations/ 目录）
    QString qmPath = QString(":/resources/translations/%1.qm").arg(langCode);

    if (m_translator.load(qmPath)) {
        // 安装新翻译
        QApplication::installTranslator(&m_translator);
        m_currentLang = langCode;
        emit languageSwitched();  // 触发界面更新

        // 保存到配置文件
        QSettings settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/config.ini", QSettings::IniFormat);
        settings.setValue("Language/Current", langCode);
    } else {
        qWarning() << "无法加载翻译文件：" << qmPath;
    }
}

QString TranslatorManager::getCurrentLanguage() const
{
    return m_currentLang;
}

QStringList TranslatorManager::getAvailableLanguages() const {
    QStringList availableLangNames;

    // 1. 获取 translations 目录下的所有 .qm 文件
    QDir dir(":/resources/translations");
    if (!dir.exists()) {
        qWarning() << "translations 目录不存在：" << dir.path();
        return availableLangNames;
    }

    // 2. 扫描 .qm 文件并提取语言代码
    QStringList qmFiles = dir.entryList(QStringList() << "*.qm", QDir::Files);
    QRegularExpression langRegex(R"((\w+_\w+)\.qm)", QRegularExpression::CaseInsensitiveOption);

    for (const QString& file : qmFiles) {
        QRegularExpressionMatch match = langRegex.match(file);
        if (match.hasMatch()) {
            QString langCode = match.captured(1);
            LanguageCode codeEnum = fromString(langCode);

            // 3. 仅保留预定义的 LanguageCode 枚举语言
            if (codeEnum != LanguageCode::UNKNOWN) {
                // 4. 获取本地化语言名称（如 "简体中文"）
                QString langName = languageName(codeEnum);
                availableLangNames << langName;
            }
        }
    }

    // 5. 去重并排序（按本地化名称排序）
    availableLangNames.removeDuplicates();
    std::sort(availableLangNames.begin(), availableLangNames.end());

    return availableLangNames;
}


LanguageThemeDialog::LanguageThemeDialog(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint) {
    setupUI();
    populateLanguages();
}

LanguageThemeDialog::~LanguageThemeDialog() = default;

void LanguageThemeDialog::setupUI() {
    // 移除无边框标志以恢复系统标题栏
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);

    // 主体布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(15);
    contentLayout->setContentsMargins(20, 10, 20, 10);

    // 语言选择区域
    QLabel* languageLabel = new QLabel(Translator::tr("选择语言"), contentWidget);
    languageComboBox = new QComboBox(contentWidget);
    languageComboBox->setFixedHeight(30);

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    applyButton = new QPushButton(Translator::tr("保存"));
    cancelButton = new QPushButton(Translator::tr("取消"));
    applyButton->setFixedHeight(32);
    cancelButton->setFixedHeight(32);

    // 添加控件到布局
    contentLayout->addWidget(languageLabel);
    contentLayout->addWidget(languageComboBox);

    buttonLayout->addStretch();
    buttonLayout->addWidget(applyButton);
    buttonLayout->addWidget(cancelButton);
    contentLayout->addLayout(buttonLayout);

    mainLayout->addWidget(contentWidget);

    // 保留拖动功能（如果需要）：
    // ...保持 mousePressEvent/mouseMoveEvent 等实现...

    // 信号连接（保持不变）
    connect(applyButton, &QPushButton::clicked, this, &LanguageThemeDialog::applyChanges);
    connect(cancelButton, &QPushButton::clicked, this, &LanguageThemeDialog::cancelChanges);
}

void LanguageThemeDialog::populateLanguages() {
    // 获取可用语言名称列表（已按本地化名称排序）
    QStringList languages = TranslatorManager::instance().getAvailableLanguages();

    // 获取当前语言代码（如 "en_US"）
    QString currentLangCode = TranslatorManager::instance().getCurrentLanguage();

    // 填充语言选项
    for (const QString& langName : languages) {
        // 根据名称获取对应的 LanguageCode 枚举
        auto codeStr = TranslatorManager::codeFromName(langName);
        TranslatorManager::LanguageCode code = TranslatorManager::fromString(codeStr);

        // 将语言名称作为显示文本，语言代码作为用户数据存储
        if (code != TranslatorManager::LanguageCode::UNKNOWN) {
            QString langCode = TranslatorManager::toString(code);
            languageComboBox->addItem(langName, langCode); // 存储语言代码作为用户数据
        }
    }

    // 设置当前语言为默认选中项
    int index = languageComboBox->findData(currentLangCode);
    if (index != -1) {
        languageComboBox->setCurrentIndex(index);
    }
}

void LanguageThemeDialog::applyChanges() {
    // 获取选中的语言代码（如 "en_US"）
    QString selectedLangCode = languageComboBox->currentData().toString();
    if (!selectedLangCode.isEmpty()) {
        TranslatorManager::instance().setLanguage(selectedLangCode);
    }

    accept(); // 关闭对话框
}

void LanguageThemeDialog::cancelChanges() {
    reject(); // 关闭对话框
}

void LanguageThemeDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragPosition = event->globalPos() - frameGeometry().topLeft();
        dragging = true;
    }
    QDialog::mousePressEvent(event);
}

void LanguageThemeDialog::mouseMoveEvent(QMouseEvent* event) {
    if (dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - dragPosition);
    }
    QDialog::mouseMoveEvent(event);
}

void LanguageThemeDialog::mouseReleaseEvent(QMouseEvent* event) {
    dragging = false;
    QDialog::mouseReleaseEvent(event);
}