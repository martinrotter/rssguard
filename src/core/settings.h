#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>


class Settings : public QSettings {
  private:
    static QPointer<Settings> s_instance;

  public:
    // Singleton getter.
    static Settings *getInstance();

    // Constructor and destructor.
    Settings(const QString & file_name, Format format, QObject * parent = 0);
    ~Settings();

    // Getter/setter for settings values.
    QVariant value(const QString &section,
                   const QString &key,
                   const QVariant &default_value = QVariant());

    void setValue(const QString &section,
                  const QString &key,
                  const QVariant &value);

    // Synchronises settings.
    QSettings::Status checkSettings();

  protected:
    // Creates settings file in correct location.
    static QSettings::Status setupSettings();
};

#endif // SETTINGS_H
