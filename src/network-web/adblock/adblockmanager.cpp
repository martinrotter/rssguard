// This file is part of RSS Guard.
//
// Copyright (C) 2014-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "network-web/adblock/adblockdialog.h"
#include "network-web/adblock/adblockmatcher.h"
#include "network-web/adblock/adblocksubscription.h"
#include "network-web/adblock/adblockblockednetworkreply.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/webpage.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"

#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QTimer>
#include <QWebEnginePage>


AdBlockManager *AdBlockManager::s_adBlockManager = NULL;

AdBlockManager::AdBlockManager(QObject* parent)
  : QObject(parent), m_loaded(false), m_enabled(false), m_useLimitedEasyList(true),
    m_subscriptions(QList<AdBlockSubscription*>()), m_matcher(new AdBlockMatcher(this)) {
  load();
}

AdBlockManager::~AdBlockManager() {
  qDeleteAll(m_subscriptions);
}

AdBlockManager *AdBlockManager::instance() {
  if (s_adBlockManager == NULL) {
    s_adBlockManager = new AdBlockManager(SilentNetworkAccessManager::instance());
  }

  return s_adBlockManager;
}

void AdBlockManager::setEnabled(bool enabled) {
  if (m_enabled == enabled) {
    return;
  }

  m_enabled = enabled;
  emit enabledChanged(enabled);
  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::Enabled, m_enabled);

  // Load subscriptions and other data.
  load();
}

QList<AdBlockSubscription*> AdBlockManager::subscriptions() const {
  return m_subscriptions;
}

QNetworkReply *AdBlockManager::block(const QNetworkRequest &request) {
  const QString url_string = request.url().toEncoded().toLower();
  const QString url_domain = request.url().host().toLower();
  const QString url_scheme = request.url().scheme().toLower();

  if (!isEnabled() || !canRunOnScheme(url_scheme)) {
    return NULL;
  }

  const AdBlockRule *blocked_rule = m_matcher->match(request, url_domain, url_string);

  if (blocked_rule != NULL) {
    QVariant v = request.attribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 100));
    WebPage *web_page = static_cast<WebPage*>(v.value<void*>());

    if (WebPage::isPointerSafeToUse(web_page)) {
      if (!canBeBlocked(web_page->url())) {
        return NULL;
      }

      web_page->addAdBlockRule(blocked_rule, request.url());
    }

    AdBlockBlockedNetworkReply *reply = new AdBlockBlockedNetworkReply(blocked_rule, this);
    reply->setRequest(request);

    return reply;
  }

  return NULL;
}

QStringList AdBlockManager::disabledRules() const {
  return m_disabledRules;
}

void AdBlockManager::addDisabledRule(const QString &filter) {
  m_disabledRules.append(filter);
}

void AdBlockManager::removeDisabledRule(const QString &filter) {
  m_disabledRules.removeOne(filter);
}

AdBlockSubscription *AdBlockManager::addSubscription(const QString &title, const QString &url) {
  if (title.isEmpty() || url.isEmpty()) {
    return NULL;
  }

  const QString file_name = IOFactory::filterBadCharsFromFilename(title.toLower()) + QL1S(".txt");
  const QString file_path = IOFactory::ensureUniqueFilename(baseSubscriptionDirectory() + QDir::separator() + file_name);
  QFile file(file_path);

  if (!file.open(QFile::WriteOnly | QFile::Truncate | QFile::Unbuffered)) {
    qWarning("Cannot write to file '%s'.",qPrintable(file_path));
    return NULL;
  }

  file.write(QString("Title: %1\nUrl: %2\n[Adblock Plus 1.1.1]").arg(title, url).toLatin1());
  file.close();

  AdBlockSubscription *subscription = new AdBlockSubscription(title, this);
  subscription->setUrl(QUrl(url));
  subscription->setFilePath(file_path);
  subscription->loadSubscription(m_disabledRules);

  // This expects that there is at least "Custom rules" subscription available in
  // active subscriptions.
  m_subscriptions.insert(m_subscriptions.size() - 1, subscription);
  m_matcher->update();

  return subscription;
}

bool AdBlockManager::removeSubscription(AdBlockSubscription *subscription) {
  if (!m_subscriptions.contains(subscription) || !subscription->canBeRemoved()) {
    return false;
  }
  else {
    QFile::remove(subscription->filePath());
    m_subscriptions.removeOne(subscription);
    delete subscription;
    m_matcher->update();
    return true;
  }
}

AdBlockCustomList *AdBlockManager::customList() const {
  foreach (AdBlockSubscription *subscription, m_subscriptions) {
    AdBlockCustomList *list = qobject_cast<AdBlockCustomList*>(subscription);

    if (list != NULL) {
      return list;
    }
  }

  return NULL;
}

QString AdBlockManager::baseSubscriptionDirectory() {
  QString directory;

  if (qApp->settings()->type() == SettingsProperties::Portable) {
    directory = qApp->applicationDirPath();
  }
  else {
    directory = qApp->homeFolderPath() + QDir::separator() + QString(APP_LOW_H_NAME);
  }

  directory += QString(QDir::separator()) + ADBLOCK_BASE_DIRECTORY_NAME;
  return QDir::toNativeSeparators(directory);
}

bool AdBlockManager::shouldBeEnabled() const {
  return qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::Enabled)).toBool();
}

void AdBlockManager::load() {
  if (m_loaded) {
    // It is already loaded: subscriptions are loaded from files into objects,
    // enabled status is determined, disabled rules are also determined.
    return;
  }

  const Settings *settings = qApp->settings();
  m_enabled = settings->value(GROUP(AdBlock), SETTING(AdBlock::Enabled)).toBool();
  m_useLimitedEasyList = settings->value(GROUP(AdBlock), SETTING(AdBlock::UseLimitedEasyList)).toBool();
  m_disabledRules = settings->value(GROUP(AdBlock), SETTING(AdBlock::DisabledRules)).toStringList();

  if (!m_enabled) {
    // We loaded settings, but Adblock should be disabled. Do not continue to save memory.
    return;
  }

  QDir adblock_dir(baseSubscriptionDirectory());

  if (!adblock_dir.exists()) {
    adblock_dir.mkpath(adblock_dir.absolutePath());
  }

  foreach (const QString &file_name, adblock_dir.entryList(QStringList(QL1S("*.txt")), QDir::Files)) {
    if (file_name == QSL(ADBLOCK_CUSTOM_LIST_FILENAME)) {
      continue;
    }

    const QString absolute_path = adblock_dir.absoluteFilePath(file_name);
    QFile file(absolute_path);

    if (!file.open(QFile::ReadOnly)) {
      continue;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    const QString title = stream.readLine(1024).remove(QSL("Title: "));
    const QUrl url = QUrl(stream.readLine(1024).remove(QSL("Url: ")));

    // Close the file.
    file.close();

    if (title.isEmpty() || !url.isValid()) {
      qWarning("Invalid subscription file '%s'.", qPrintable(absolute_path));
      continue;
    }

    AdBlockSubscription *subscription = new AdBlockSubscription(title, this);
    subscription->setUrl(url);
    subscription->setFilePath(absolute_path);

    m_subscriptions.append(subscription);
  }

  // Append list for "Custom rules".
  m_subscriptions.append(new AdBlockCustomList(this));

  // Load all subscriptions, including obligatory "Custom rules" list.
  foreach (AdBlockSubscription *subscription, m_subscriptions) {
    subscription->loadSubscription(m_disabledRules);

    connect(subscription, SIGNAL(subscriptionChanged()), m_matcher, SLOT(update()));
  }

  // Notify matcher about loaded subscriptions
  // and mark all this shit as loaded.
  m_matcher->update();
  m_loaded = true;
}

void AdBlockManager::updateAllSubscriptions() {
  foreach (AdBlockSubscription *subscription, m_subscriptions) {
    subscription->updateSubscription();
  }

  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::LastUpdated, QDateTime::currentDateTime());
}

void AdBlockManager::save() {
  if (m_loaded) {
    foreach (AdBlockSubscription *subscription, m_subscriptions) {
      subscription->saveSubscription();
    }

    Settings *settings = qApp->settings();
    settings->setValue(GROUP(AdBlock), AdBlock::Enabled, m_enabled);
    settings->setValue(GROUP(AdBlock), AdBlock::UseLimitedEasyList, m_useLimitedEasyList);
    settings->setValue(GROUP(AdBlock), AdBlock::DisabledRules, m_disabledRules);
  }
}

bool AdBlockManager::isEnabled() const {
  return m_enabled;
}

bool AdBlockManager::canRunOnScheme(const QString &scheme) const {
  return !(scheme == QL1S("file") || scheme == QL1S("qrc") || scheme == QL1S("data") || scheme == QL1S("abp"));
}

bool AdBlockManager::useLimitedEasyList() const {
  return m_useLimitedEasyList;
}

void AdBlockManager::setUseLimitedEasyList(bool use_limited) {
  m_useLimitedEasyList = use_limited;

  foreach (AdBlockSubscription *subscription, m_subscriptions) {
    if (subscription->url() == QUrl(ADBLOCK_EASYLIST_URL)) {
      // User really has EasyList activated, update it.
      subscription->updateSubscription();
    }
  }
}

bool AdBlockManager::canBeBlocked(const QUrl &url) const {
  return !m_matcher->adBlockDisabledForUrl(url);
}

QString AdBlockManager::elementHidingRules() const {
  return m_matcher->elementHidingRules();
}

QString AdBlockManager::elementHidingRulesForDomain(const QUrl &url) const {
  if (!isEnabled() || !canRunOnScheme(url.scheme()) || !canBeBlocked(url)) {
    return QString();
  }
  // Acid3 doesn't like the way element hiding rules are embedded into page
  else if (url.host() == QL1S("acid3.acidtests.org")) {
    return QString();
  }
  else {
    return m_matcher->elementHidingRulesForDomain(url.host());
  }
}

AdBlockSubscription *AdBlockManager::subscriptionByName(const QString &name) const {
  foreach (AdBlockSubscription *subscription, m_subscriptions) {
    if (subscription->title() == name) {
      return subscription;
    }
  }

  return NULL;
}

AdBlockDialog *AdBlockManager::showDialog() {
  QPointer<AdBlockDialog> form_pointer = new AdBlockDialog(qApp->mainForm());
  form_pointer.data()->setModal(true);
  form_pointer.data()->show();
  form_pointer.data()->raise();
  form_pointer.data()->activateWindow();
  return form_pointer.data();
}

void AdBlockManager::showRule() {
  if (QAction *action = qobject_cast<QAction*>(sender())) {
    const AdBlockRule* rule = static_cast<const AdBlockRule*>(action->data().value<void*>());

    if (rule != NULL) {
      showDialog()->showRule(rule);
    }
  }
}
