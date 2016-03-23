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

#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include <QObject>

#include <QStringList>
#include <QPointer>
#include <QWebEngineUrlRequestInfo>


class QUrl;
class AdBlockRule;
class AdBlockDialog;
class AdBlockMatcher;
class AdBlockCustomList;
class AdBlockSubscription;

class AdBlockManager : public QObject {
    Q_OBJECT

  public:
    // Constructors.
    explicit AdBlockManager(QObject* parent = 0);
    virtual ~AdBlockManager();

    void load();
    void save();

    bool isEnabled() const;
    bool canRunOnScheme(const QString &scheme) const;

    bool useLimitedEasyList() const;
    void setUseLimitedEasyList(bool use_limited);

    AdBlockSubscription *subscriptionByName(const QString &name) const;
    QList<AdBlockSubscription*> subscriptions() const;

    bool shouldBlock(const QUrl &url, const QString &referer, QWebEngineUrlRequestInfo::ResourceType resource_type);

    QStringList disabledRules() const;
    void addDisabledRule(const QString &filter);
    void removeDisabledRule(const QString &filter);

    AdBlockSubscription *addSubscription(const QString &title, const QString &url);
    bool removeSubscription(AdBlockSubscription* subscription);

    AdBlockCustomList *customList() const;

    bool shouldBeEnabled() const;

    static QString baseSubscriptionDirectory();
    static AdBlockManager *instance();

  public slots:
    void setEnabled(bool enabled);
    void showRule();
    void updateAllSubscriptions();

    AdBlockDialog *showDialog();

  signals:
    void enabledChanged(bool enabled);

  private:
    bool canBeBlocked(const QUrl &url, const QString &referer, QWebEngineUrlRequestInfo::ResourceType resource_type) const;

    bool m_loaded;
    bool m_enabled;
    bool m_useLimitedEasyList;

    QList<AdBlockSubscription*> m_subscriptions;
    AdBlockMatcher* m_matcher;
    QStringList m_disabledRules;

    static AdBlockManager* s_adBlockManager;
};

#endif // ADBLOCKMANAGER_H

