// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/localization.h"

#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"

#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>

Localization::Localization(Settings* settings, QObject* parent) : QObject(parent), m_settings(settings) {}

Localization::~Localization() = default;

QString Localization::desiredLanguage() const {
  return m_settings->value(GROUP(General), SETTING(General::Language)).toString();
}

void Localization::loadActiveLanguage() {
  auto* qt_translator = new QTranslator(this);
  auto* app_translator = new QTranslator(this);
  QString desired_localization = desiredLanguage();

  qDebugNN << LOGSEC_CORE << "Starting to load active localization. Desired localization is"
           << QUOTE_W_SPACE_DOT(desired_localization);

  if (app_translator->load(QLocale(desired_localization), QSL("rssguard"), QSL("_"), APP_LANG_PATH)) {
    const QString real_loaded_locale = app_translator->language();

    QCoreApplication::installTranslator(app_translator);

    qDebugNN << LOGSEC_CORE << "Application localization" << QUOTE_W_SPACE(desired_localization)
             << "loaded successfully, specifically sublocalization" << QUOTE_W_SPACE(real_loaded_locale)
             << "was loaded.";
    desired_localization = real_loaded_locale;
  }
  else {
    qWarningNN << LOGSEC_CORE << "Application localization" << QUOTE_W_SPACE(desired_localization)
               << "was not loaded. Loading" << QUOTE_W_SPACE(DEFAULT_LOCALE) << "instead.";
    desired_localization = QSL(DEFAULT_LOCALE);

    if (!app_translator->load(QLocale(desired_localization), QSL("rssguard"), QSL("_"), APP_LANG_PATH)) {
      qCriticalNN << LOGSEC_CORE << "Even default localzation was not loaded.";

      QCoreApplication::installTranslator(app_translator);
    }
  }

  if (qt_translator->load(QLocale(desired_localization), QSL("qtbase"), QSL("_"), APP_LANG_PATH)) {
    QCoreApplication::installTranslator(qt_translator);

    qDebugNN << LOGSEC_CORE << "Qt localization" << QUOTE_W_SPACE(desired_localization) << "loaded successfully.";
  }
  else {
    qWarningNN << LOGSEC_CORE << "Qt localization" << QUOTE_W_SPACE(desired_localization)
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
  auto lang_files =
    file_dir.entryInfoList(QStringList() << QSL("rssguard_*.qm"), QDir::Filter::Files, QDir::SortFlag::Name);

  // Iterate all found language files.
  for (const QFileInfo& file : std::as_const(lang_files)) {
    if (translator.load(file.absoluteFilePath())) {
      Language new_language;

      new_language.m_code = translator.language();
      new_language.m_name = QLocale(new_language.m_code).nativeLanguageName();
      languages << new_language;
    }
  }

  return languages;
}
