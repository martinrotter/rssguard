/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2010-2017 David Rosca <nowrep@gmail.com>
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
#include "adblockurlinterceptor.h"
#include "datapaths.h"
#include "mainapplication.h"
#include "webpage.h"
#include "qztools.h"
#include "browserwindow.h"
#include "settings.h"
#include "networkmanager.h"

#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QTimer>
#include <QMessageBox>
#include <QUrlQuery>
#include <QMutexLocker>
#include <QSaveFile>

//#define ADBLOCK_DEBUG

#ifdef ADBLOCK_DEBUG
#include <QElapsedTimer>
#endif

Q_GLOBAL_STATIC(AdBlockManager, qz_adblock_manager)

AdBlockManager::AdBlockManager(QObject* parent)
    : QObject(parent)
    , m_loaded(false)
    , m_enabled(true)
    , m_matcher(new AdBlockMatcher(this))
    , m_interceptor(new AdBlockUrlInterceptor(this))
{
    load();
}

AdBlockManager::~AdBlockManager()
{
    qDeleteAll(m_subscriptions);
}

AdBlockManager* AdBlockManager::instance()
{
    return qz_adblock_manager();
}

void AdBlockManager::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    emit enabledChanged(enabled);

    Settings settings;
    settings.beginGroup("AdBlock");
    settings.setValue("enabled", m_enabled);
    settings.endGroup();

    load();
    mApp->reloadUserStyleSheet();

    QMutexLocker locker(&m_mutex);

    if (m_enabled) {
        m_matcher->update();
    } else {
        m_matcher->clear();
    }
}

QList<AdBlockSubscription*> AdBlockManager::subscriptions() const
{
    return m_subscriptions;
}

bool AdBlockManager::block(QWebEngineUrlRequestInfo &request)
{
    QMutexLocker locker(&m_mutex);

    if (!isEnabled()) {
        return false;
    }

#ifdef ADBLOCK_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif
    const QString urlString = request.requestUrl().toEncoded().toLower();
    const QString urlDomain = request.requestUrl().host().toLower();
    const QString urlScheme = request.requestUrl().scheme().toLower();

    if (!canRunOnScheme(urlScheme) || !canBeBlocked(request.firstPartyUrl())) {
        return false;
    }

    bool res = false;
    const AdBlockRule* blockedRule = m_matcher->match(request, urlDomain, urlString);

    if (blockedRule) {
        res = true;

        if (request.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
            QUrl url(QSL("qupzilla:adblock"));
            QUrlQuery query;
            query.addQueryItem(QSL("rule"), blockedRule->filter());
            query.addQueryItem(QSL("subscription"), blockedRule->subscription()->title());
            url.setQuery(query);
            request.redirect(url);
        }
        else {
            request.block(true);
        }

#ifdef ADBLOCK_DEBUG
        qDebug() << "BLOCKED: " << timer.elapsed() << blockedRule->filter() << request.requestUrl();
#endif
    }

#ifdef ADBLOCK_DEBUG
    qDebug() << timer.elapsed() << request.requestUrl();
#endif

    return res;
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

bool AdBlockManager::addSubscriptionFromUrl(const QUrl &url)
{
    const QList<QPair<QString, QString> > queryItems = QUrlQuery(url).queryItems(QUrl::FullyDecoded);

    QString subscriptionTitle;
    QString subscriptionUrl;

    for (int i = 0; i < queryItems.count(); ++i) {
        QPair<QString, QString> pair = queryItems.at(i);
        if (pair.first == QL1S("location"))
            subscriptionUrl = pair.second;
        else if (pair.first == QL1S("title"))
            subscriptionTitle = pair.second;
    }

    if (subscriptionTitle.isEmpty() || subscriptionUrl.isEmpty())
        return false;

    const QString message = AdBlockManager::tr("Do you want to add <b>%1</b> subscription?").arg(subscriptionTitle);

    QMessageBox::StandardButton result = QMessageBox::question(0, AdBlockManager::tr("AdBlock Subscription"), message, QMessageBox::Yes | QMessageBox::No);
    if (result == QMessageBox::Yes) {
        AdBlockManager::instance()->addSubscription(subscriptionTitle, subscriptionUrl);
        AdBlockManager::instance()->showDialog();
    }

    return true;
}

AdBlockSubscription* AdBlockManager::addSubscription(const QString &title, const QString &url)
{
    if (title.isEmpty() || url.isEmpty()) {
        return 0;
    }

    QString fileName = QzTools::filterCharsFromFilename(title.toLower()) + ".txt";
    QString filePath = QzTools::ensureUniqueFilename(DataPaths::currentProfilePath() + "/adblock/" + fileName);

    QByteArray data = QString("Title: %1\nUrl: %2\n[Adblock Plus 1.1.1]").arg(title, url).toLatin1();

    QSaveFile file(filePath);
    if (!file.open(QFile::WriteOnly)) {
        qWarning() << "AdBlockManager: Cannot write to file" << filePath;
        return 0;
    }
    file.write(data);
    file.commit();

    AdBlockSubscription* subscription = new AdBlockSubscription(title, this);
    subscription->setUrl(QUrl(url));
    subscription->setFilePath(filePath);
    subscription->loadSubscription(m_disabledRules);

    m_subscriptions.insert(m_subscriptions.count() - 1, subscription);
    connect(subscription, SIGNAL(subscriptionUpdated()), mApp, SLOT(reloadUserStyleSheet()));
    connect(subscription, SIGNAL(subscriptionChanged()), this, SLOT(updateMatcher()));

    return subscription;
}

bool AdBlockManager::removeSubscription(AdBlockSubscription* subscription)
{
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
    QMutexLocker locker(&m_mutex);

    if (m_loaded) {
        return;
    }

#ifdef ADBLOCK_DEBUG
    QElapsedTimer timer;
    timer.start();
#endif

    Settings settings;
    settings.beginGroup("AdBlock");
    m_enabled = settings.value("enabled", m_enabled).toBool();
    m_disabledRules = settings.value("disabledRules", QStringList()).toStringList();
    QDateTime lastUpdate = settings.value("lastUpdate", QDateTime()).toDateTime();
    settings.endGroup();

    if (!m_enabled) {
        return;
    }

    QDir adblockDir(DataPaths::currentProfilePath() + "/adblock");
    // Create if neccessary
    if (!adblockDir.exists()) {
        QDir(DataPaths::currentProfilePath()).mkdir("adblock");
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
        easyList->setFilePath(DataPaths::currentProfilePath() + QLatin1String("/adblock/easylist.txt"));

        m_subscriptions.prepend(easyList);
    }

    // Append CustomList
    AdBlockCustomList* customList = new AdBlockCustomList(this);
    m_subscriptions.append(customList);

    // Load all subscriptions
    foreach (AdBlockSubscription* subscription, m_subscriptions) {
        subscription->loadSubscription(m_disabledRules);

        connect(subscription, SIGNAL(subscriptionUpdated()), mApp, SLOT(reloadUserStyleSheet()));
        connect(subscription, SIGNAL(subscriptionChanged()), this, SLOT(updateMatcher()));
    }

    if (lastUpdate.addDays(5) < QDateTime::currentDateTime()) {
        QTimer::singleShot(1000 * 60, this, SLOT(updateAllSubscriptions()));
    }

#ifdef ADBLOCK_DEBUG
    qDebug() << "AdBlock loaded in" << timer.elapsed();
#endif

    m_matcher->update();
    m_loaded = true;

    mApp->networkManager()->installUrlInterceptor(m_interceptor);
}

void AdBlockManager::updateMatcher()
{
    QMutexLocker locker(&m_mutex);

    m_matcher->update();
}

void AdBlockManager::updateAllSubscriptions()
{
    foreach (AdBlockSubscription* subscription, m_subscriptions) {
        subscription->updateSubscription();
    }

    Settings settings;
    settings.beginGroup("AdBlock");
    settings.setValue("lastUpdate", QDateTime::currentDateTime());
    settings.endGroup();
}

void AdBlockManager::save()
{
    if (!m_loaded) {
        return;
    }

    foreach (AdBlockSubscription* subscription, m_subscriptions) {
        subscription->saveSubscription();
    }

    Settings settings;
    settings.beginGroup("AdBlock");
    settings.setValue("enabled", m_enabled);
    settings.setValue("disabledRules", m_disabledRules);
    settings.endGroup();
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

bool AdBlockManager::canBeBlocked(const QUrl &url) const
{
    return !m_matcher->adBlockDisabledForUrl(url);
}

QString AdBlockManager::elementHidingRules(const QUrl &url) const
{
    if (!isEnabled() || !canRunOnScheme(url.scheme()) || !canBeBlocked(url))
        return QString();

    return m_matcher->elementHidingRules();
}

QString AdBlockManager::elementHidingRulesForDomain(const QUrl &url) const
{
    if (!isEnabled() || !canRunOnScheme(url.scheme()) || !canBeBlocked(url))
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

AdBlockDialog* AdBlockManager::showDialog()
{
    if (!m_adBlockDialog) {
        m_adBlockDialog = new AdBlockDialog;
    }

    m_adBlockDialog.data()->show();
    m_adBlockDialog.data()->raise();
    m_adBlockDialog.data()->activateWindow();

    return m_adBlockDialog.data();
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
