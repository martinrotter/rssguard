#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>


class Settings {
  private:
    // We use QPointer instead of QScopedPointer
    // because of late s_instance usage in QApplication::aboutToQuit() listeners.
    static QPointer<QSettings> s_instance;

  public:
    // Getter/setter for settings values.
    static QVariant value(const QString &section,
                          const QString &key,
                          const QVariant &default_value = QVariant());

    static void setValue(const QString &section,
                         const QString &key,
                         const QVariant &value);

    // It's better to cleanup settings manually via this function.
    static void deleteSettings();

    // Synchronises settings.
    static QSettings::Status checkSettings();

  protected:
    // Creates settings file in correct location.
    static QSettings::Status setupSettings();
};

#endif // SETTINGS_H
