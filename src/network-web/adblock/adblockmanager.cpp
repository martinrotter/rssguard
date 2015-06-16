/* ============================================================
* QuiteRSS is a open-source cross-platform RSS/Atom news feeds reader
* Copyright (C) 2011-2015 QuiteRSS Team <quiterssteam@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "adblockmanager.h"
#include "adblockdialog.h"
#include "adblockmatcher.h"
#include "adblocksubscription.h"
#include "adblockblockednetworkreply.h"
#include "adblockicon.h"
#include "webpage.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "gui/formmain.h"

#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QTimer>
#include <QWebFrame>
#include <QDebug>

#define ADBLOCK_DEBUG

#ifdef ADBLOCK_DEBUG
#include <QElapsedTimer>
#endif

AdBlockManager* AdBlockManager::s_adBlockManager = 0;

AdBlockManager::AdBlockManager(QObject* parent)
  : QObject(parent)
  , m_loaded(false)
  , m_enabled(true)
  , m_useLimitedEasyList(true)
  , m_matcher(new AdBlockMatcher(this)), m_subscriptions(QList<AdBlockSubscription*>())
{
  load();
}

AdBlockManager::~AdBlockManager()
{
  qDeleteAll(m_subscriptions);
}

AdBlockManager* AdBlockManager::instance()
{
  if (!s_adBlockManager) {
    s_adBlockManager = new AdBlockManager(SilentNetworkAccessManager::instance());
  }

  return s_adBlockManager;
}

QString AdBlockManager::filterCharsFromFilename(const QString &name)
{
  QString value = name;

  value.replace(QLatin1Char('/'), QLatin1Char('-'));
  value.remove(QLatin1Char('\\'));
  value.remove(QLatin1Char(':'));
  value.remove(QLatin1Char('*'));
  value.remove(QLatin1Char('?'));
  value.remove(QLatin1Char('"'));
  value.remove(QLatin1Char('<'));
  value.remove(QLatin1Char('>'));
  value.remove(QLatin1Char('|'));

  return value;
}

QString AdBlockManager::ensureUniqueFilename(const QString &name, const QString &appendFormat)
{
  if (!QFile::exists(name)) {
    return name;
  }

  QString tmpFileName = name;
  int i = 1;
  while (QFile::exists(tmpFileName)) {
    tmpFileName = name;
    int index = tmpFileName.lastIndexOf(QLatin1Char('.'));

    QString appendString = appendFormat.arg(i);
    if (index == -1) {
      tmpFileName.append(appendString);
    }
    else {
      tmpFileName = tmpFileName.left(index) + appendString + tmpFileName.mid(index);
    }
    i++;
  }
  return tmpFileName;
}

void AdBlockManager::setEnabled(bool enabled)
{
  if (m_enabled == enabled) {
    return;
  }

  m_enabled = enabled;
  emit enabledChanged(enabled);

  Settings *settings = qApp->settings();
  settings->beginGroup("AdBlock");
  settings->setValue("enabled", m_enabled);
  settings->endGroup();

  load();
  // TODO
  //mainApp->reloadUserStyleBrowser();
}

QList<AdBlockSubscription*> AdBlockManager::subscriptions() const
{
  return m_subscriptions;
}

QNetworkReply* AdBlockManager::block(const QNetworkRequest &request)
{
#ifdef ADBLOCK_DEBUG
  QElapsedTimer timer;
  timer.start();
#endif
  const QString urlString = request.url().toEncoded().toLower();
  const QString urlDomain = request.url().host().toLower();
  const QString urlScheme = request.url().scheme().toLower();

  if (!isEnabled() || !canRunOnScheme(urlScheme))
    return 0;

  const AdBlockRule* blockedRule = m_matcher->match(request, urlDomain, urlString);

  if (blockedRule) {
    QVariant v = request.attribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 100));
    WebPage* webPage = static_cast<WebPage*>(v.value<void*>());
    if (WebPage::isPointerSafeToUse(webPage)) {
      if (!canBeBlocked(webPage->mainFrame()->url())) {
        return 0;
      }

      webPage->addAdBlockRule(blockedRule, request.url());
    }

    AdBlockBlockedNetworkReply* reply = new AdBlockBlockedNetworkReply(blockedRule, this);
    reply->setRequest(request);

#ifdef ADBLOCK_DEBUG
    qDebug() << "BLOCKED: " << timer.elapsed() << blockedRule->filter() << request.url();
#endif

    return reply;
  }

#ifdef ADBLOCK_DEBUG
  qDebug() << timer.elapsed() << request.url();
#endif

  return 0;
}

QStringList AdBlockManager::disabledRules() const
{
  return m_disabledRules;
}

void AdBlockManager::addDisabledRule(const QString &filter)
{
  m_disabledRules.append(filter);
}

void AdBlockManager::removeDisabledRule(const QString &filter)
{
  m_disabledRules.removeOne(filter);
}

AdBlockSubscription* AdBlockManager::addSubscription(const QString &title, const QString &url)
{
  if (title.isEmpty() || url.isEmpty()) {
    return 0;
  }

  QString fileName = filterCharsFromFilename(title.toLower()) + ".txt";
  QString filePath = ensureUniqueFilename(qApp->homeFolderPath() + "/adblock/" + fileName);

  QByteArray data = QString("Title: %1\nUrl: %2\n[Adblock Plus 1.1.1]").arg(title, url).toLatin1();

  QFile file(filePath);
  if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
    qWarning() << "AdBlockManager: Cannot write to file" << filePath;
    return 0;
  }

  file.write(data);
  file.close();

  AdBlockSubscription* subscription = new AdBlockSubscription(title, this);
  subscription->setUrl(QUrl(url));
  subscription->setFilePath(filePath);
  subscription->loadSubscription(m_disabledRules);

  m_subscriptions.insert(m_subscriptions.count() - 1, subscription);

  return subscription;
}

bool AdBlockManager::removeSubscription(AdBlockSubscription* subscription)
{
  if (!m_subscriptions.contains(subscription) || !subscription->canBeRemoved()) {
    return false;
  }

  QFile(subscription->filePath()).remove();
  m_subscriptions.removeOne(subscription);

  delete subscription;
  return true;
}

AdBlockCustomList* AdBlockManager::customList() const
{
  foreach (AdBlockSubscription* subscription, m_subscriptions) {
    AdBlockCustomList* list = qobject_cast<AdBlockCustomList*>(subscription);

    if (list) {
      return list;
    }
  }

  return 0;
}

void AdBlockManager::load()
{
  if (m_loaded) {
    return;
  }

#ifdef ADBLOCK_DEBUG
  QElapsedTimer timer;
  timer.start();
#endif

  Settings *settings = qApp->settings();
  m_enabled = settings->value("AdBlock","enabled", m_enabled).toBool();
  m_useLimitedEasyList = settings->value("AdBlock","useLimitedEasyList", m_useLimitedEasyList).toBool();
  m_disabledRules = settings->value("AdBlock","disabledRules", QStringList()).toStringList();
  QDateTime lastUpdate = settings->value("AdBlock","lastUpdate", QDateTime()).toDateTime();

  if (!m_enabled) {
    return;
  }

  QDir adblockDir(qApp->homeFolderPath() + "/adblock");
  // Create if neccessary
  if (!adblockDir.exists()) {
    QDir(qApp->homeFolderPath()).mkdir("adblock");
  }

  foreach (const QString &fileName, adblockDir.entryList(QStringList("*.txt"), QDir::Files)) {
    if (fileName == QLatin1String("customlist.txt")) {
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
      qWarning() << "AdBlockManager: Invalid subscription file" << absolutePath;
      continue;
    }

    AdBlockSubscription* subscription = new AdBlockSubscription(title, this);
    subscription->setUrl(url);
    subscription->setFilePath(absolutePath);

    m_subscriptions.append(subscription);
  }

  // Prepend EasyList if subscriptions are empty
  if (m_subscriptions.isEmpty()) {
    AdBlockSubscription* easyList = new AdBlockSubscription(tr("EasyList"), this);
    easyList->setUrl(QUrl(ADBLOCK_EASYLIST_URL));
    easyList->setFilePath(qApp->homeFolderPath() + "/adblock/easylist.txt");

    // TODO
    //connect(easyList, SIGNAL(subscriptionUpdated()), mainApp, SLOT(reloadUserStyleBrowser()));

    m_subscriptions.prepend(easyList);
  }

  // Append CustomList
  AdBlockCustomList* customList = new AdBlockCustomList(this);
  m_subscriptions.append(customList);

  // Load all subscriptions
  foreach (AdBlockSubscription* subscription, m_subscriptions) {
    subscription->loadSubscription(m_disabledRules);

    // TODO
    //connect(subscription, SIGNAL(subscriptionUpdated()), mainApp, SLOT(reloadUserStyleBrowser()));
    connect(subscription, SIGNAL(subscriptionChanged()), m_matcher, SLOT(update()));
  }

  if (lastUpdate.addDays(5) < QDateTime::currentDateTime()) {
    QTimer::singleShot(1000 * 60, this, SLOT(updateAllSubscriptions()));
  }

#ifdef ADBLOCK_DEBUG
  qDebug() << "AdBlock loaded in" << timer.elapsed();
#endif

  m_matcher->update();
  m_loaded = true;
}

void AdBlockManager::updateAllSubscriptions()
{
  foreach (AdBlockSubscription* subscription, m_subscriptions) {
    subscription->updateSubscription();
  }

  Settings *settings = qApp->settings();
  settings->beginGroup("AdBlock");
  settings->setValue("lastUpdate", QDateTime::currentDateTime());
  settings->endGroup();
}

void AdBlockManager::save()
{
  if (!m_loaded) {
    return;
  }

  foreach (AdBlockSubscription* subscription, m_subscriptions) {
    subscription->saveSubscription();
  }

  Settings *settings = qApp->settings();
  settings->beginGroup("AdBlock");
  settings->setValue("enabled", m_enabled);
  settings->setValue("useLimitedEasyList", m_useLimitedEasyList);
  settings->setValue("disabledRules", m_disabledRules);
  settings->endGroup();
}

bool AdBlockManager::isEnabled() const
{
  return m_enabled;
}

bool AdBlockManager::canRunOnScheme(const QString &scheme) const
{
  return !(scheme == QLatin1String("file") || scheme == QLatin1String("qrc")
           || scheme == QLatin1String("qupzilla") || scheme == QLatin1String("data")
           || scheme == QLatin1String("abp"));
}

bool AdBlockManager::useLimitedEasyList() const
{
  return m_useLimitedEasyList;
}

void AdBlockManager::setUseLimitedEasyList(bool useLimited)
{
  m_useLimitedEasyList = useLimited;

  foreach (AdBlockSubscription* subscription, m_subscriptions) {
    if (subscription->url() == QUrl(ADBLOCK_EASYLIST_URL)) {
      subscription->updateSubscription();
    }
  }
}

bool AdBlockManager::canBeBlocked(const QUrl &url) const
{
  return !m_matcher->adBlockDisabledForUrl(url);
}

QString AdBlockManager::elementHidingRules() const
{
  return m_matcher->elementHidingRules();
}

QString AdBlockManager::elementHidingRulesForDomain(const QUrl &url) const
{
  if (!isEnabled() || !canRunOnScheme(url.scheme()) || !canBeBlocked(url))
    return QString();

  // Acid3 doesn't like the way element hiding rules are embedded into page
  if (url.host() == QLatin1String("acid3.acidtests.org"))
    return QString();

  return m_matcher->elementHidingRulesForDomain(url.host());
}

AdBlockSubscription* AdBlockManager::subscriptionByName(const QString &name) const
{
  foreach (AdBlockSubscription* subscription, m_subscriptions) {
    if (subscription->title() == name) {
      return subscription;
    }
  }

  return 0;
}

AdBlockDialog *AdBlockManager::showDialog() {
  QPointer<AdBlockDialog> form_pointer = new AdBlockDialog();
  form_pointer.data()->show();
  form_pointer.data()->raise();
  form_pointer.data()->activateWindow();
  form_pointer.data()->setAttribute(Qt::WA_DeleteOnClose, true);
  return form_pointer.data();
  /*
  if (!m_adBlockDialog) {
    m_adBlockDialog = new AdBlockDialog;
  }

  m_adBlockDialog.data()->show();
  m_adBlockDialog.data()->raise();
  m_adBlockDialog.data()->activateWindow();

  return m_adBlockDialog.data();*/
}

void AdBlockManager::showRule()
{
  if (QAction* action = qobject_cast<QAction*>(sender())) {
    const AdBlockRule* rule = static_cast<const AdBlockRule*>(action->data().value<void*>());

    if (rule) {
      showDialog()->showRule(rule);
    }
  }
}
