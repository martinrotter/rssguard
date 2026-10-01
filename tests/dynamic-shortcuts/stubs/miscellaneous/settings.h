// Temporary INI storage without RSS Guard startup or user-profile access.
#ifndef SHORTCUT_TEST_SETTINGS_H
#define SHORTCUT_TEST_SETTINGS_H

#include <QSettings>

class Settings : public QSettings {
  public:
    using QSettings::QSettings;

    QVariant value(const QString& section, const QString& key, const QVariant& fallback = {}) const {
      return QSettings::value(section + QLatin1Char('/') + key, fallback);
    }
    void setValue(const QString& section, const QString& key, const QVariant& value) {
      QSettings::setValue(section + QLatin1Char('/') + key, value);
    }
};

#endif
