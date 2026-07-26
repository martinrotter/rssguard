// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGS_H
#define SETTINGS_H

#include "definitions/definitions.h"
#include "miscellaneous/settingsproperties.h"
#include "miscellaneous/textfactory.h"

#include <QByteArray>
#include <QColor>
#include <QDateTime>
#include <QNetworkProxy>
#include <QReadWriteLock>
#include <QSettings>
#include <QStringList>
#include <QWriteLocker>

class RSSGUARD_DLLSPEC Settings : public QSettings {
    Q_OBJECT

  public:
    explicit Settings(const QString& file_name,
                      Format format,
                      SettingsProperties::SettingsType type,
                      QObject* parent = nullptr);
    virtual ~Settings();

    // Type of used settings.
    SettingsProperties::SettingsType type() const;

    // Getters/setters for settings values.
    QVariant password(const QString& section, const QString& key, const QVariant& default_value = QVariant()) const;
    void setPassword(const QString& section, const QString& key, const QVariant& value);

    QStringList allKeys(const QString& section);

    QVariant value(const QString& section, const QString& key, const QVariant& default_value = QVariant()) const;
    void setValue(const QString& section, const QString& key, const QVariant& value);
    void setValue(const QString& key, const QVariant& value);

    bool contains(const QString& section, const QString& key) const;
    void remove(const QString& section, const QString& key = {});

    // Returns the path which contains the settings.
    QString pathName() const;

    // Synchronizes settings.
    QSettings::Status checkSettings();

    bool initiateRestoration(const QString& settings_backup_file_path);
    static void finishRestoration(const QString& desired_settings_file_path);

    // Creates settings file in correct location.
    static Settings* setupSettings(QObject* parent);

    // Returns properties of the actual application-wide settings.
    static SettingsProperties determineProperties();

  private:
    mutable QReadWriteLock m_lock;
    SettingsProperties::SettingsType m_initializationStatus;
};

inline SettingsProperties::SettingsType Settings::type() const {
  return m_initializationStatus;
}

// Getters/setters for settings values.
inline QVariant Settings::password(const QString& section, const QString& key, const QVariant& default_value) const {
  return TextFactory::decrypt(value(section, key, default_value).toString());
}

inline void Settings::setPassword(const QString& section, const QString& key, const QVariant& value) {
  setValue(section, key, TextFactory::encrypt(value.toString()));
}

inline QVariant Settings::value(const QString& section, const QString& key, const QVariant& default_value) const {
  return QSettings::value(QString(QSL("%1/%2")).arg(section, key), default_value);
}

inline void Settings::setValue(const QString& section, const QString& key, const QVariant& value) {
  QWriteLocker lck(&m_lock);
  QSettings::setValue(QString(QSL("%1/%2")).arg(section, key), value);
}

inline void Settings::setValue(const QString& key, const QVariant& value) {
  QWriteLocker lck(&m_lock);
  QSettings::setValue(key, value);
}

inline bool Settings::contains(const QString& section, const QString& key) const {
  return QSettings::contains(QString(QSL("%1/%2")).arg(section, key));
}

inline void Settings::remove(const QString& section, const QString& key) {
  QWriteLocker lck(&m_lock);

  if (key.isEmpty()) {
    beginGroup(section);
    QSettings::remove({});
    endGroup();
  }
  else {
    QSettings::remove(QString(QSL("%1/%2")).arg(section, key));
  }
}

#endif // SETTINGS_H
