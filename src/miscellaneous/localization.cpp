// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "miscellaneous/localization.h"

#include "miscellaneous/application.h"

#include <QTranslator>
#include <QDir>
#include <QFileInfoList>
#include <QLocale>


Localization::Localization(QObject *parent)
  : QObject(parent) {
}

Localization::~Localization() {
}

QString Localization::desiredLanguage() const {
  return qApp->settings()->value(GROUP(General), SETTING(General::Language)).toString();
}

void Localization::loadActiveLanguage() {
  QTranslator *qt_translator = new QTranslator(qApp);
  QTranslator *app_translator = new QTranslator(qApp);
  QString desired_localization = desiredLanguage();

  qDebug("Starting to load active localization. Desired localization is '%s'.", qPrintable(desired_localization));

  if (app_translator->load(QLocale(desired_localization), "rssguard", QSL("-"), APP_LANG_PATH)) {
    const QString real_loaded_locale = app_translator->translate("QObject", "LANG_ABBREV");

    Application::installTranslator(app_translator);
    qDebug("Application localization '%s' loaded successfully, specifically sublocalization '%s' was loaded.",
           qPrintable(desired_localization),
           qPrintable(real_loaded_locale));
    desired_localization = real_loaded_locale;
  }
  else {
    qWarning("Application localization '%s' was not loaded. Loading '%s' instead.",
             qPrintable(desired_localization),
             DEFAULT_LOCALE);
    desired_localization = DEFAULT_LOCALE;
  }

  if (qt_translator->load(QLocale(desired_localization), "qtbase", QSL("-"), APP_LANG_PATH)) {
    Application::installTranslator(qt_translator);
    qDebug("Qt localization '%s' loaded successfully.", qPrintable(desired_localization));
  }
  else {
    qWarning("Qt localization '%s' was not loaded.", qPrintable(desired_localization));
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
  foreach (const QFileInfo &file, file_dir.entryInfoList(QStringList() << "rssguard-*.qm", QDir::Files, QDir::Name)) {
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

