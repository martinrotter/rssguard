// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/localization.h"

#include "miscellaneous/application.h"

#include <QDir>
#include <QFileInfoList>
#include <QLocale>
#include <QTranslator>

Localization::Localization(QObject* parent)
  : QObject(parent) {}

Localization::~Localization() = default;

QString Localization::desiredLanguage() const {
  return qApp->settings()->value(GROUP(General), SETTING(General::Language)).toString();
}

void Localization::loadActiveLanguage() {
  auto* qt_translator = new QTranslator(qApp);
  auto* app_translator = new QTranslator(qApp);
  QString desired_localization = desiredLanguage();

  qDebugNN << LOGSEC_CORE
           << "Starting to load active localization. Desired localization is"
           << QUOTE_W_SPACE_DOT(desired_localization);

  if (app_translator->load(QLocale(desired_localization), "rssguard", QSL("_"), APP_LANG_PATH)) {
    const QString real_loaded_locale = app_translator->translate("QObject", "LANG_ABBREV");

    Application::installTranslator(app_translator);
    qDebugNN << LOGSEC_CORE
             << "Application localization"
             << QUOTE_W_SPACE(desired_localization)
             << "loaded successfully, specifically sublocalization"
             << QUOTE_W_SPACE(real_loaded_locale)
             << "was loaded.";
    desired_localization = real_loaded_locale;
  }
  else {
    qWarningNN << LOGSEC_CORE
               << "Application localization"
               << QUOTE_W_SPACE(desired_localization)
               << "was not loaded. Loading"
               << QUOTE_W_SPACE(DEFAULT_LOCALE)
               << "instead.";
    desired_localization = DEFAULT_LOCALE;
  }

  if (qt_translator->load(QLocale(desired_localization), "qtbase", QSL("_"), APP_LANG_PATH)) {
    Application::installTranslator(qt_translator);
    qDebugNN << LOGSEC_CORE
             << "Qt localization"
             << QUOTE_W_SPACE(desired_localization)
             << "loaded successfully.";
  }
  else {
    qWarningNN << LOGSEC_CORE
               << "Qt localization"
               << QUOTE_W_SPACE(desired_localization)
               << "WAS NOT loaded successfully.";
  }

  m_loadedLanguage = desired_localization;
  m_loadedLocale = QLocale(desired_localization);
  QLocale::setDefault(m_loadedLocale);
}

QList<Language> Localization::installedLanguages() const {
  QList<Language> languages;
  const QDir file_dir(APP_LANG_PATH);
  QTranslator translator;

  // Iterate all found language files.
  for (const QFileInfo& file : file_dir.entryInfoList(QStringList() << "rssguard_*.qm", QDir::Files, QDir::Name)) {
    if (translator.load(file.absoluteFilePath())) {
      Language new_language;

      new_language.m_code = translator.translate("QObject", "LANG_ABBREV");
      new_language.m_author = translator.translate("QObject", "LANG_AUTHOR");
      new_language.m_email = translator.translate("QObject", "LANG_EMAIL");
      new_language.m_name = QLocale(new_language.m_code).nativeLanguageName();
      languages << new_language;
    }
  }

  return languages;
}
