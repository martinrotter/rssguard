#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QPointer>


class Settings : public QSettings {
    Q_OBJECT

  private:
    // Constructor.
    Settings(const QString & file_name, Format format, QObject * parent = 0);

    // Creates settings file in correct location.
    static QSettings::Status setupSettings();

    // Private singleton value.
    static QPointer<Settings> s_instance;

  public:
    // Singleton getter.
    static Settings *getInstance();

    // Destructor.
    virtual ~Settings();

    // Getter/setter for settings values.
    QVariant value(const QString &section,
                   const QString &key,
                   const QVariant &default_value = QVariant());

    void setValue(const QString &section,
                  const QString &key,
                  const QVariant &value);

    // Synchronises settings.
    QSettings::Status checkSettings();
};

#endif // SETTINGS_H
