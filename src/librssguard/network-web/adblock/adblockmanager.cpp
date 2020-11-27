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
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/adblock/adblockurlinterceptor.h"
#include "network-web/networkurlinterceptor.h"

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QMutexLocker>
#include <QSaveFile>
#include <QTextStream>
#include <QTimer>
#include <QUrlQuery>
#include <QWebEngineProfile>
#include <QWebEngineUrlRequestInfo>

Q_GLOBAL_STATIC(AdBlockManager, qz_adblock_manager)

AdBlockManager::AdBlockManager(QObject* parent)
  : QObject(parent), m_loaded(false), m_enabled(true), m_matcher(new AdBlockMatcher(this)), m_interceptor(new AdBlockUrlInterceptor(this)) {
  load();
  m_adblockIcon = new AdBlockIcon(this);
  m_adblockIcon->setObjectName(QSL("m_adblockIconAction"));
}

AdBlockManager::~AdBlockManager() {
  qDeleteAll(m_subscriptions);
}

AdBlockManager* AdBlockManager::instance() {
  return qz_adblock_manager();
}

void AdBlockManager::setEnabled(bool enabled) {
  if (m_enabled == enabled) {
    return;
  }

  m_enabled = enabled;
  emit enabledChanged(enabled);

  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::AdBlockEnabled, m_enabled);
  load();

  QMutexLocker locker(&m_mutex);

  if (m_enabled) {
    m_matcher->update();
  }
  else {
    m_matcher->clear();
  }
}

QList<AdBlockSubscription*> AdBlockManager::subscriptions() const {
  return m_subscriptions;
}

bool AdBlockManager::block(QWebEngineUrlRequestInfo& request) {
  QMutexLocker locker(&m_mutex);

  if (!isEnabled()) {
    return false;
  }

  const QString urlString = request.requestUrl().toEncoded().toLower();
  const QString urlDomain = request.requestUrl().host().toLower();
  const QString urlScheme = request.requestUrl().scheme().toLower();

  if (!canRunOnScheme(urlScheme) || !canBeBlocked(request.firstPartyUrl())) {
    return false;
  }

  bool res = false;
  const AdBlockRule* blockedRule = m_matcher->match(request, urlDomain, urlString);

  if (blockedRule != nullptr) {
    if (request.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
      QUrlQuery query;
      QUrl url(QSL("rssguard:adblockedpage"));

      query.addQueryItem(QSL("rule"), blockedRule->filter());
      query.addQueryItem(QSL("subscription"), blockedRule->subscription()->title());
      url.setQuery(query);
      request.redirect(url);
    }
    else {
      res = true;
      request.block(true);
    }
  }

  return res;
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

bool AdBlockManager::addSubscriptionFromUrl(const QUrl& url) {
  const QList<QPair<QString, QString>> queryItems = QUrlQuery(url).queryItems(QUrl::FullyDecoded);
  QString subscriptionTitle;
  QString subscriptionUrl;

  for (int i = 0; i < queryItems.count(); ++i) {
    QPair<QString, QString> pair = queryItems.at(i);

    if (pair.first == QL1S("location")) {
      subscriptionUrl = pair.second;
    }
    else if (pair.first == QL1S("title")) {
      subscriptionTitle = pair.second;
    }
  }

  if (subscriptionTitle.isEmpty() || subscriptionUrl.isEmpty()) {
    return false;
  }

  const QString message = tr("Do you want to add <b>%1</b> subscription?").arg(subscriptionTitle);

  QMessageBox::StandardButton result = QMessageBox::question(nullptr, tr("Add AdBlock subscription"), message,
                                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

  if (result == QMessageBox::Yes) {
    AdBlockManager::instance()->addSubscription(subscriptionTitle, subscriptionUrl);
    AdBlockManager::instance()->showDialog();
  }

  return true;
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

void AdBlockManager::load() {
  QMutexLocker locker(&m_mutex);

  if (m_loaded) {
    return;
  }

  m_enabled = qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::AdBlockEnabled)).toBool();
  m_disabledRules = qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::DisabledRules)).toStringList();
  QDateTime lastUpdate = qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::LastUpdatedOn)).toDateTime();

  if (!m_enabled) {
    return;
  }

  QDir adblockDir(storedListsPath());

  // Create if neccessary
  if (!adblockDir.exists()) {
    QDir().mkpath(storedListsPath());
  }

  for (const QString& fileName : adblockDir.entryList(QStringList("*.txt"), QDir::Files)) {
    if (fileName == ADBLOCK_CUSTOMLIST_NAME) {
      continue;
    }

    const QString absolutePath = adblockDir.absoluteFilePath(fileName);
    QFile file(absolutePath);

    if (!file.open(QFile::ReadOnly)) {
      continue;
    }

    QTextStream textStream(&file);

    textStream.setCodec("UTF-8");
    QString title = textStream.readLine(1024).remove(QLatin1String("Title: "));
    QUrl url = QUrl(textStream.readLine(1024).remove(QLatin1String("Url: ")));

    if (title.isEmpty() || !url.isValid()) {
      qWarningNN << LOGSEC_ADBLOCK
                 << "Invalid AdBlock subscription file"
                 << QUOTE_W_SPACE_DOT(absolutePath);
      continue;
    }

    auto* subscription = new AdBlockSubscription(title, this);

    subscription->setUrl(url);
    subscription->setFilePath(absolutePath);
    m_subscriptions.append(subscription);
  }

  // Append CustomList.
  auto* customList = new AdBlockCustomList(this);

  m_subscriptions.append(customList);

  // Load all subscriptions.
  for (AdBlockSubscription* subscription : m_subscriptions) {
    subscription->loadSubscription(m_disabledRules);
    connect(subscription, &AdBlockSubscription::subscriptionChanged, this, &AdBlockManager::updateMatcher);
  }

  if (lastUpdate.addDays(ADBLOCK_UPDATE_DAYS_INTERVAL) < QDateTime::currentDateTime()) {
    QTimer::singleShot(1000 * 60, this, &AdBlockManager::updateAllSubscriptions);
  }

  m_matcher->update();
  m_loaded = true;
  qApp->urlIinterceptor()->installUrlInterceptor(m_interceptor);
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
  return !(scheme == QSL("file") || scheme == QSL("qrc") || scheme == QSL("data") || scheme == QSL("abp"));
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

AdBlockSubscription* AdBlockManager::subscriptionByName(const QString& name) const {
  for (AdBlockSubscription* subscription : m_subscriptions) {
    if (subscription->title() == name) {
      return subscription;
    }
  }

  return nullptr;
}

void AdBlockManager::showDialog() {
  if (m_adBlockDialog == nullptr) {
    m_adBlockDialog = new AdBlockDialog();
  }

  m_adBlockDialog.data()->exec();
}
