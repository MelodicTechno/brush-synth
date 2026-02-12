#include "AppSettings.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

AppSettings& AppSettings::instance() {
    static AppSettings instance;
    return instance;
}

AppSettings::AppSettings() {
    load();
}

void AppSettings::load() {
    QFile file("settings.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (obj.contains("language")) {
        int lang = obj["language"].toInt();
        if (lang >= 0 && lang <= 1) {
            m_language = (Language)lang;
        }
    }
}

void AppSettings::save() {
    QJsonObject obj;
    obj["language"] = (int)m_language;

    QJsonDocument doc(obj);
    QFile file("settings.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

AppSettings::Language AppSettings::getLanguage() const {
    return m_language;
}

void AppSettings::setLanguage(Language lang) {
    if (m_language != lang) {
        m_language = lang;
        save();
    }
}
