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
#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include <QObject>
#include <QStringList>
#include <QPointer>
#include <QMutex>

#include "qzcommon.h"

class QUrl;
class QWebEngineUrlRequestInfo;

class AdBlockRule;
class AdBlockDialog;
class AdBlockMatcher;
class AdBlockCustomList;
class AdBlockSubscription;
class AdBlockUrlInterceptor;

class QUPZILLA_EXPORT AdBlockManager : public QObject
{
    Q_OBJECT

public:
    AdBlockManager(QObject* parent = 0);
    ~AdBlockManager();

    void load();
    void save();

    bool isEnabled() const;
    bool canRunOnScheme(const QString &scheme) const;

    QString elementHidingRules(const QUrl &url) const;
    QString elementHidingRulesForDomain(const QUrl &url) const;

    AdBlockSubscription* subscriptionByName(const QString &name) const;
    QList<AdBlockSubscription*> subscriptions() const;

    bool block(QWebEngineUrlRequestInfo &request);

    QStringList disabledRules() const;
    void addDisabledRule(const QString &filter);
    void removeDisabledRule(const QString &filter);

    bool addSubscriptionFromUrl(const QUrl &url);

    AdBlockSubscription* addSubscription(const QString &title, const QString &url);
    bool removeSubscription(AdBlockSubscription* subscription);

    AdBlockCustomList* customList() const;

    static AdBlockManager* instance();

signals:
    void enabledChanged(bool enabled);

public slots:
    void setEnabled(bool enabled);
    void showRule();

    void updateMatcher();
    void updateAllSubscriptions();

    AdBlockDialog* showDialog();

private:
    inline bool canBeBlocked(const QUrl &url) const;

    bool m_loaded;
    bool m_enabled;

    QList<AdBlockSubscription*> m_subscriptions;
    AdBlockMatcher* m_matcher;
    QStringList m_disabledRules;

    AdBlockUrlInterceptor *m_interceptor;
    QPointer<AdBlockDialog> m_adBlockDialog;
    QMutex m_mutex;
};

#endif // ADBLOCKMANAGER_H

