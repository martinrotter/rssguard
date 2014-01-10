#ifndef LOCALIZATION_H
#define LOCALIZATION_H

// Loads currently active language.
// NOTE: Macro is used due to QTranslator persistency.
#define LoadLocalization(); \
  QString locale_name = Settings::instance()->value( \
  APP_CFG_GEN, \
  "language", \
  "en").toString(); \
  QTranslator qt_translator, app_translator; \
  if (app_translator.load(QString("rssguard_%1.qm").arg(locale_name), \
  APP_LANG_PATH)) { \
  QApplication::installTranslator(&app_translator); \
  qDebug("Application localization '%s' loaded successfully.", \
  qPrintable(locale_name)); \
  } \
  else { \
  qWarning("Application localization '%s' was not loaded.", qPrintable(locale_name)); \
  } \
  if (qt_translator.load(QString("qt_%1.qm").arg(locale_name), \
  APP_LANG_PATH)) { \
  qDebug("Qt localization '%s' loaded successfully.", \
  qPrintable(locale_name));  \
  } \
  else { \
  qWarning("Qt localization '%s' was not loaded.", qPrintable(locale_name)); \
  } \
  QLocale::setDefault(QLocale(locale_name));

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
    // Constructor.
    explicit Localization();

  public:
    // Returns list of installed application localizations.
    // This list is used ie. in settings dialog.
    static QList<Language> installedLanguages();
};

#endif // LOCALIZATION_H
