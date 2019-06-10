// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <QObject>

#include <QLocale>
#include <QString>

struct Language {
  QString m_name;
  QString m_code;
  QString m_author;
  QString m_email;
};

class RSSGUARD_DLLSPEC Localization : public QObject {
  Q_OBJECT

  public:

    // Constructor.
    explicit Localization(QObject* parent = nullptr);

    // Destructor.
    virtual ~Localization();

    // Returns code of language that should
    // be loaded according to settings.
    QString desiredLanguage() const;

    // Loads currently active language.
    void loadActiveLanguage();

    // Returns list of installed application localizations.
    // This list is used ie. in settings dialog.
    QList<Language> installedLanguages() const;

    // Returns empty string or loaded language
    // name if it is really loaded.
    inline QString loadedLanguage() const {
      return m_loadedLanguage;
    }

    inline QLocale loadedLocale() const {
      return m_loadedLocale;
    }

  private:

    // Code of loaded language.
    QString m_loadedLanguage;
    QLocale m_loadedLocale;
};

#endif // LOCALIZATION_H
