// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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


QPointer<Localization> Localization::s_instance;

Localization::Localization(QObject *parent)
  : QObject(parent) {
}

Localization::~Localization() {
  qDebug("Destroying Localization instance.");
}

Localization *Localization::instance() {
  if (s_instance.isNull()) {
    s_instance = new Localization(qApp);
  }

  return s_instance;
}

QString Localization::desiredLanguage() {
  return Settings::instance()->value(APP_CFG_GEN,
                                     "language",
                                     QLocale::system().name()).toString();
}

void Localization::load() {
  QTranslator *qt_translator = new QTranslator(qApp);
  QTranslator *app_translator = new QTranslator(qApp);
  QString desired_localization = desiredLanguage();

  if (app_translator->load(QString("rssguard-%1.qm").arg(desired_localization),
                           APP_LANG_PATH,
                           "-")) {
    QApplication::installTranslator(app_translator);
    qDebug("Application localization '%s' loaded successfully.",
           qPrintable(desired_localization));
  }
  else {
    qWarning("Application localization '%s' was not loaded.", qPrintable(desired_localization));

    desired_localization = DEFAULT_LOCALE;
  }

  if (qt_translator->load(QString("qt-%1.qm").arg(desired_localization),
                          APP_LANG_PATH,
                          "-")) {
    QApplication::installTranslator(qt_translator);
    qDebug("Qt localization '%s' loaded successfully.",
           qPrintable(desired_localization));
  }
  else {
    qWarning("Qt localization '%s' was not loaded.", qPrintable(desired_localization));
  }

  m_loadedLanguage = desired_localization;
  QLocale::setDefault(QLocale(desired_localization));
}

QList<Language> Localization::installedLanguages() {
  QList<Language> languages;
  QDir file_dir(APP_LANG_PATH);
  QTranslator translator;

  // Iterate all found language files.
  foreach (const QFileInfo &file, file_dir.entryInfoList(QStringList() << "rssguard-*.qm",
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

