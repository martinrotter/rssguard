#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QPointer>


class Settings : public QSettings {
    Q_OBJECT

  public:
    enum Type {
      Portable,
      NonPortable
    };

    // Singleton getter.
    static Settings *getInstance();

    // Destructor.
    virtual ~Settings();

    Type type() const;

    // Getter/setter for settings values.
    QVariant value(const QString &section,
                   const QString &key,
                   const QVariant &default_value = QVariant());

    void setValue(const QString &section,
                  const QString &key,
                  const QVariant &value);

    // Synchronizes settings.
    QSettings::Status checkSettings();

  private:
    // Constructor.
    Settings(const QString & file_name, Format format,
             const Type &type, QObject * parent = 0);

    Type m_initializationStatus;

    // Creates settings file in correct location.
    static QSettings::Status setupSettings();

    // Private singleton value.
    static QPointer<Settings> s_instance;
};

#endif // SETTINGS_H
