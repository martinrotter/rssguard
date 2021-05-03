// For license of this file, see <project-root-folder>/LICENSE.md.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#include "network-web/adblock/adblockmanager.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/adblock/adblockdialog.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/adblock/adblockurlinterceptor.h"
#include "network-web/networkurlinterceptor.h"
#include "network-web/webfactory.h"

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QMutexLocker>
#include <QSaveFile>
#include <QTextStream>
#include <QTimer>
#include <QUrlQuery>
#include <QWebEngineProfile>

AdBlockManager::AdBlockManager(QObject* parent)
  : QObject(parent), m_loaded(false), m_enabled(false), m_interceptor(new AdBlockUrlInterceptor(this)) {
  m_adblockIcon = new AdBlockIcon(this);
  m_adblockIcon->setObjectName(QSL("m_adblockIconAction"));

  m_unifiedFiltersFile = qApp->userDataFolder() + QDir::separator() + QSL("adblock-unified-filters.txt");
}

bool AdBlockManager::block(const AdblockRequestInfo& request) {
  QMutexLocker locker(&m_mutex);

  if (!isEnabled()) {
    return false;
  }

  const QString url_string = request.requestUrl().toEncoded().toLower();
  const QString url_scheme = request.requestUrl().scheme().toLower();

  if (!canRunOnScheme(url_scheme)) {
    return false;
  }
  else {
    // TODO: start server if needed, call it.
    return false;
  }
}

void AdBlockManager::load(bool initial_load) {
  QMutexLocker locker(&m_mutex);
  auto new_enabled = qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::AdBlockEnabled)).toBool();

  if (!initial_load) {
    new_enabled = !new_enabled;
  }

  if (new_enabled != m_enabled) {
    emit enabledChanged(new_enabled);

    qApp->settings()->setValue(GROUP(AdBlock), AdBlock::AdBlockEnabled, new_enabled);
  }
  else if (!initial_load) {
    return;
  }

  m_enabled = new_enabled;

  if (!m_loaded) {
    qApp->web()->urlIinterceptor()->installUrlInterceptor(m_interceptor);
    m_loaded = true;
  }

  if (m_enabled) {
    if (!QFile::exists(m_unifiedFiltersFile)) {
      updateUnifiedFiltersFile();
    }
  }
  else {
    if (QFile::exists(m_unifiedFiltersFile)) {
      QFile::remove(m_unifiedFiltersFile);
    }
  }
}

bool AdBlockManager::isEnabled() const {
  return m_enabled;
}

bool AdBlockManager::canRunOnScheme(const QString& scheme) const {
  return !(scheme == QSL("file") || scheme == QSL("qrc") || scheme == QSL("data") || scheme == QSL("abp"));
}

QString AdBlockManager::elementHidingRulesForDomain(const QUrl& url) const {
  // TODO: call service for cosmetic rules.
  return {};
}

QString AdBlockManager::generateJsForElementHiding(const QString& css) const {
  QString source = QL1S("(function() {"
                        "var head = document.getElementsByTagName('head')[0];"
                        "if (!head) return;"
                        "var css = document.createElement('style');"
                        "css.setAttribute('type', 'text/css');"
                        "css.appendChild(document.createTextNode('%1'));"
                        "head.appendChild(css);"
                        "})()");
  QString style = css;

  style.replace(QL1S("'"), QL1S("\\'"));
  style.replace(QL1S("\n"), QL1S("\\n"));

  return source.arg(style);
}

void AdBlockManager::showDialog() {
  AdBlockDialog(qApp->mainFormWidget()).exec();
}

void AdBlockManager::updateUnifiedFiltersFile() {
  // TODO: download contents of all filter lists + append custom filters
  // and combine into single file.
}
