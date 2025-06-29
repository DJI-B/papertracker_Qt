#ifndef TRANSLATOR_MANAGER_H
#define TRANSLATOR_MANAGER_H

#include <QTranslator>
#include <QObject>
#include <QDialog>
#include <QComboBox>
#include <QPushButton>

class TranslatorManager : public QObject {
    Q_OBJECT
public:

    enum class LanguageCode {
        EN_US,   // 英语 (美国)
        ZH_CN,   // 中文 (简体)
        JA_JP,   // 日语 (日本)
        KO_KR,   // 韩语 (韩国)
        ES_ES,   // 西班牙语 (西班牙)
        FR_FR,   // 法语 (法国)
        UNKNOWN  // 未知语言（默认值）
    };

    static const QMap<LanguageCode, QString>& codeToStringMap() {
        static const QMap<LanguageCode, QString> map = {
            {LanguageCode::EN_US, "en_US"},
            {LanguageCode::ZH_CN, "zh_CN"},
            {LanguageCode::JA_JP, "ja_JP"},
            {LanguageCode::KO_KR, "ko_KR"},
            {LanguageCode::ES_ES, "es_ES"},
            {LanguageCode::FR_FR, "fr_FR"}
        };
        return map;
    }

    static const QMap<QString, LanguageCode>& stringToCodeMap() {
        static const QMap<QString, LanguageCode> map = {
            {"en_US", LanguageCode::EN_US},
            {"zh_CN", LanguageCode::ZH_CN},
            {"ja_JP", LanguageCode::JA_JP},
            {"ko_KR", LanguageCode::KO_KR},
            {"es_ES", LanguageCode::ES_ES},
            {"fr_FR", LanguageCode::FR_FR}
        };
        return map;
    }

    // 获取语言名称（用于界面显示）
    static QString languageName(LanguageCode code) {
        switch (code) {
            case LanguageCode::EN_US: return "English";
            case LanguageCode::ZH_CN: return "简体中文";
            case LanguageCode::JA_JP: return "日本語";
            case LanguageCode::KO_KR: return "한국어";
            case LanguageCode::ES_ES: return "Español";
            case LanguageCode::FR_FR: return "Français";
            default: return "Unknown";
        }
    }

    static QString codeFromName(const QString& langName) {
        for (auto it = codeToStringMap().cbegin(); it != codeToStringMap().cend(); ++it) {
            if (languageName(it.key()) == langName) {
                return it.value();
            }
        }
        return QString();
    }

    // 字符串转枚举（带默认值）
    static LanguageCode fromString(const QString& code, LanguageCode defaultValue = LanguageCode::UNKNOWN) {
        return stringToCodeMap().value(code, defaultValue);
    }

    // 枚举转字符串
    static QString toString(LanguageCode code) {
        return codeToStringMap().value(code, "");
    }

    // 获取单例实例
    static TranslatorManager& instance() {
        static TranslatorManager instance;
        return instance;
    }

    // 设置语言（如 "en_US"）
    void setLanguage(const QString& langCode);
    QString getCurrentLanguage() const;
    QStringList getAvailableLanguages() const;

    signals:
        void languageSwitched();  // 通知界面更新

private:
    TranslatorManager(QObject* parent = nullptr);
    ~TranslatorManager();

    QTranslator m_translator;
    QString m_currentLang;
};

class LanguageThemeDialog : public QDialog {
    Q_OBJECT

public:
    explicit LanguageThemeDialog(QWidget* parent = nullptr);
    ~LanguageThemeDialog();

private slots:
    void applyChanges();
    void cancelChanges();

private:
    void setupUI();
    void populateLanguages();

    QComboBox* languageComboBox;
    QPushButton* applyButton;
    QPushButton* cancelButton;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool dragging = false;
    QPoint dragPosition;
};

#endif // TRANSLATOR_MANAGER_H