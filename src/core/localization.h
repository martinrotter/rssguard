#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <QString>
#include <QObject>
#include <QPointer>


struct Language {
    QString m_name;
    QString m_code;
    QString m_version;
    QString m_author;
    QString m_email;
};

class Localization : public QObject {
    Q_OBJECT

  private:
    // Constructor.
    explicit Localization(QObject *parent = 0);

  public:
    // Destructor.
    virtual ~Localization();

    // Singleton getter.
    static Localization *instance();

    // Returns code of language that should
    // be loaded according to settings.
    QString desiredLanguage();

    // Loads currently active language.
    void load();

    // Returns list of installed application localizations.
    // This list is used ie. in settings dialog.
    QList<Language> installedLanguages();

    // Returns empty string or loaded language
    // name if it is really loaded.
    inline QString loadedLanguage() const {
      return m_loadedLanguage;
    }

  private:
    // Code of loaded language.
    QString m_loadedLanguage;

    // Singleton.
    static QPointer<Localization> s_instance;
};

#endif // LOCALIZATION_H
