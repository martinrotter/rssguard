#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <QString>


struct Language {
    QString m_name;
    QString m_code;
    QString m_version;
    QString m_author;
    QString m_email;
};

class Localization {
  private:
    // Constructor.
    explicit Localization();

  public:
    // Loads currently active language.
    static void load();

    // Returns list of installed application localizations.
    // This list is used ie. in settings dialog.
    static QList<Language> installedLanguages();
};

#endif // LOCALIZATION_H
