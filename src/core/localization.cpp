#include "core/localization.h"

#include "core/defs.h"
#include "core/settings.h"
#include "qtsingleapplication/qtsingleapplication.h"

#include <QPointer>
#include <QApplication>
#include <QTranslator>
#include <QDir>
#include <QFileInfoList>
#include <QLocale>


Localization::Localization() {
}

void Localization::load() {
  QTranslator *qt_translator = new QTranslator(qApp);
  QTranslator *app_translator = new QTranslator(qApp);
  QString locale_system = QLocale::system().name();
  QString locale_name = Settings::instance()->value(APP_CFG_GEN,
                                                    "language",
                                                    locale_system).toString();

  qDebug("Try to load application localization. "
         "System locale was detected as '%s'. "
         "Trying to load localization for '%s'.",
         qPrintable(locale_system),
         qPrintable(locale_name));

  if (app_translator->load(QString("rssguard_%1.qm").arg(locale_name),
                           APP_LANG_PATH)) {
    QApplication::installTranslator(app_translator);
    qDebug("Application localization '%s' loaded successfully.",
           qPrintable(locale_name));
  }
  else {
    qWarning("Application localization '%s' was not loaded.", qPrintable(locale_name));
  }
  if (qt_translator->load(QString("qt_%1.qm").arg(locale_name),
                          APP_LANG_PATH)) {
    QApplication::installTranslator(qt_translator);
    qDebug("Qt localization '%s' loaded successfully.",
           qPrintable(locale_name));
  }
  else {
    qWarning("Qt localization '%s' was not loaded.", qPrintable(locale_name));
  }

  QLocale::setDefault(QLocale(locale_name));
}

QList<Language> Localization::installedLanguages() {
  QList<Language> languages;
  QDir file_dir(APP_LANG_PATH);
  QTranslator translator;

  // Iterate all found language files.
  foreach (const QFileInfo &file, file_dir.entryInfoList(QStringList() << "rssguard_*.qm",
                                                         QDir::Files,
                                                         QDir::Name)) {
    if (translator.load(file.absoluteFilePath())) {
      Language new_language;
      new_language.m_name = translator.translate("QObject", "LANG_NAME");
      new_language.m_code = translator.translate("QObject", "LANG_ABBREV");
      new_language.m_version = translator.translate("QObject", "LANG_VERSION");
      new_language.m_author = translator.translate("QObject", "LANG_AUTHOR");
      new_language.m_email = translator.translate("QObject", "LANG_EMAIL");

      languages << new_language;
    }
  }
  return languages;
}
