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
#include "network-web/adblock/adblockmatcher.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/adblock/adblocksubscription.h"
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
  : QObject(parent), m_loaded(false), m_enabled(false), m_matcher(new AdBlockMatcher(this)),
  m_interceptor(new AdBlockUrlInterceptor(this)) {
  m_adblockIcon = new AdBlockIcon(this);
  m_adblockIcon->setObjectName(QSL("m_adblockIconAction"));
}

AdBlockManager::~AdBlockManager() {
  qDeleteAll(m_subscriptions);
}

QList<AdBlockSubscription*> AdBlockManager::subscriptions() const {
  return m_subscriptions;
}

const AdBlockRule* AdBlockManager::block(const AdblockRequestInfo& request) {
  QMutexLocker locker(&m_mutex);

  if (!isEnabled()) {
    return nullptr;
  }

  const QString url_string = request.requestUrl().toEncoded().toLower();
  const QString url_domain = request.requestUrl().host().toLower();
  const QString url_scheme = request.requestUrl().scheme().toLower();

  if (!canRunOnScheme(url_scheme) || !canBeBlocked(request.firstPartyUrl())) {
    return nullptr;
  }
  else {
    const AdBlockRule* blocked_rule = m_matcher->match(request, url_domain, url_string);

    return blocked_rule;
  }
}

QStringList AdBlockManager::disabledRules() const {
  return m_disabledRules;
}

void AdBlockManager::addDisabledRule(const QString& filter) {
  m_disabledRules.append(filter);
}

void AdBlockManager::removeDisabledRule(const QString& filter) {
  m_disabledRules.removeOne(filter);
}

AdBlockSubscription* AdBlockManager::addSubscription(const QString& title, const QString& url) {
  if (title.isEmpty() || url.isEmpty()) {
    return nullptr;
  }

  QString fileName = title + QSL(".txt");
  QString filePath = storedListsPath() + QDir::separator() + fileName;
  QByteArray data = QString("Title: %1\nUrl: %2\n[Adblock Plus 1.1.1]").arg(title, url).toLatin1();
  QSaveFile file(filePath);

  if (!file.open(QFile::WriteOnly)) {
    qWarningNN << LOGSEC_ADBLOCK
               << "Cannot save AdBlock subscription to file"
               << QUOTE_W_SPACE_DOT(filePath);
    return nullptr;
  }

  file.write(data);
  file.commit();
  auto* subscription = new AdBlockSubscription(title, this);

  subscription->setUrl(QUrl(url));
  subscription->setFilePath(filePath);
  subscription->loadSubscription(m_disabledRules);
  m_subscriptions.insert(m_subscriptions.count() - 1, subscription);

  connect(subscription, &AdBlockSubscription::subscriptionChanged, this, &AdBlockManager::updateMatcher);
  return subscription;
}

bool AdBlockManager::removeSubscription(AdBlockSubscription* subscription) {
  QMutexLocker locker(&m_mutex);

  if (!m_subscriptions.contains(subscription) || !subscription->canBeRemoved()) {
    return false;
  }

  QFile(subscription->filePath()).remove();
  m_subscriptions.removeOne(subscription);
  m_matcher->update();
  delete subscription;
  return true;
}

AdBlockCustomList* AdBlockManager::customList() const {
  for (AdBlockSubscription* subscription : m_subscriptions) {
    auto* list = qobject_cast<AdBlockCustomList*>(subscription);

    if (list != nullptr) {
      return list;
    }
  }

  return nullptr;
}

QString AdBlockManager::storedListsPath() {
  return qApp->userDataFolder() + QDir::separator() + ADBLOCK_LISTS_SUBDIRECTORY;
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
    m_disabledRules = qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::DisabledRules)).toStringList();

    QDateTime last_update = qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::LastUpdatedOn)).toDateTime();
    QDir adblock_dir(storedListsPath());

    // Create if neccessary
    if (!adblock_dir.exists()) {
      QDir().mkpath(storedListsPath());
    }

    for (const QString& subscription_file_name : adblock_dir.entryList(QStringList("*.txt"), QDir::Files)) {
      if (subscription_file_name == ADBLOCK_CUSTOMLIST_NAME) {
        continue;
      }

      const QString absolute_path = adblock_dir.absoluteFilePath(subscription_file_name);
      QFile file(absolute_path);

      if (!file.open(QFile::OpenModeFlag::ReadOnly)) {
        continue;
      }

      QTextStream subscription_stream(&file);

      subscription_stream.setCodec("UTF-8");
      QString title = subscription_stream.readLine(1024).remove(QLatin1String("Title: "));
      QUrl url = QUrl(subscription_stream.readLine(1024).remove(QLatin1String("Url: ")));

      if (title.isEmpty() || !url.isValid()) {
        qWarningNN << LOGSEC_ADBLOCK
                   << "Invalid AdBlock subscription file"
                   << QUOTE_W_SPACE_DOT(absolute_path);
        continue;
      }

      auto* subscription = new AdBlockSubscription(title, this);

      subscription->setUrl(url);
      subscription->setFilePath(absolute_path);
      m_subscriptions.append(subscription);
    }

    // Append CustomList.
    auto* custom_list = new AdBlockCustomList(this);

    m_subscriptions.append(custom_list);

    // Load all subscriptions.
    for (AdBlockSubscription* subscription : m_subscriptions) {
      subscription->loadSubscription(m_disabledRules);
      connect(subscription, &AdBlockSubscription::subscriptionChanged, this, &AdBlockManager::updateMatcher);
    }

    if (last_update.addDays(ADBLOCK_UPDATE_DAYS_INTERVAL) < QDateTime::currentDateTime()) {
      QTimer::singleShot(1000 * 60, this, &AdBlockManager::updateAllSubscriptions);
    }

    qApp->web()->urlIinterceptor()->installUrlInterceptor(m_interceptor);
    m_loaded = true;
  }

  if (m_enabled) {
    m_matcher->update();
  }
  else {
    m_matcher->clear();
  }
}

void AdBlockManager::updateMatcher() {
  QMutexLocker locker(&m_mutex);

  m_matcher->update();
}

void AdBlockManager::updateAllSubscriptions() {
  for (AdBlockSubscription* subscription : m_subscriptions) {
    subscription->updateSubscription();
  }

  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::LastUpdatedOn, QDateTime::currentDateTime());
}

void AdBlockManager::save() {
  if (!m_loaded) {
    return;
  }

  for (AdBlockSubscription* subscription : m_subscriptions) {
    subscription->saveSubscription();
  }

  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::AdBlockEnabled, m_enabled);
  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::DisabledRules, m_disabledRules);
}

bool AdBlockManager::isEnabled() const {
  return m_enabled;
}

bool AdBlockManager::canRunOnScheme(const QString& scheme) const {
  return !(scheme == QSL("file") || scheme == QSL("qrc") ||
           scheme == QSL("data") || scheme == QSL("abp"));
}

bool AdBlockManager::canBeBlocked(const QUrl& url) const {
  return !m_matcher->adBlockDisabledForUrl(url);
}

QString AdBlockManager::elementHidingRules(const QUrl& url) const {
  if (!isEnabled() || !canRunOnScheme(url.scheme()) || !canBeBlocked(url)) {
    return QString();
  }
  else {
    return m_matcher->elementHidingRules();
  }
}

QString AdBlockManager::elementHidingRulesForDomain(const QUrl& url) const {
  if (!isEnabled() || !canRunOnScheme(url.scheme()) || !canBeBlocked(url)) {
    return QString();
  }
  else {
    return m_matcher->elementHidingRulesForDomain(url.host());
  }
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
