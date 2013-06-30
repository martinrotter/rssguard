#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <QPointer>


struct Language {
    QString m_name;
    QString m_code;
    QString m_version;
    QString m_author;
    QString m_email;
};

class Localization {
  private:
    Localization();

  public:
    // Sets up localization strings and locale from application settings.
    static void load();

    // Returns list of installed application localizations.
    // This list is used ie. in settings dialog.
    static QList<Language> getInstalledLanguages();
};

#endif // LOCALIZATION_H
