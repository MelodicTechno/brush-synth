#pragma once
#include <QJsonObject>
#include <QString>

class AppSettings {
public:
    enum Language { English, Chinese };

    static AppSettings& instance();

    void load();
    void save();

    Language getLanguage() const;
    void setLanguage(Language lang);

private:
    AppSettings();
    Language m_language = Chinese;
};
