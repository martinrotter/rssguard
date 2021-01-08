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

#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QStringList>

class QUrl;
class AdblockRequestInfo;
class AdBlockMatcher;
class AdBlockCustomList;
class AdBlockSubscription;
class AdBlockRule;
class AdBlockUrlInterceptor;
class AdBlockIcon;

class AdBlockManager : public QObject {
  Q_OBJECT

  public:
    explicit AdBlockManager(QObject* parent = nullptr);
    virtual ~AdBlockManager();

    // If "initial_load" is true, then we want to explicitly turn off
    // Adblock if it is running or turn on when not running.
    // if "initial_load" is true, then we want to forcefully perform
    // initial loading of Adblock.
    void load(bool initial_load);

    // Save all subscriptions to file(s).
    void save();

    // General method for adblocking. Returns pointer to rule if request should
    // be blocked.
    const AdBlockRule* block(const AdblockRequestInfo& request);

    bool isEnabled() const;
    bool canRunOnScheme(const QString& scheme) const;

    QString elementHidingRules(const QUrl& url) const;
    QString elementHidingRulesForDomain(const QUrl& url) const;
    QString generateJsForElementHiding(const QString& css) const;

    QList<AdBlockSubscription*> subscriptions() const;

    QStringList disabledRules() const;
    void addDisabledRule(const QString& filter);
    void removeDisabledRule(const QString& filter);

    AdBlockSubscription* addSubscription(const QString& title, const QString& url);
    bool removeSubscription(AdBlockSubscription* subscription);

    AdBlockCustomList* customList() const;
    AdBlockIcon* adBlockIcon() const;

    static QString storedListsPath();

  public slots:
    void updateMatcher();
    void updateAllSubscriptions();
    void showDialog();

  signals:
    void enabledChanged(bool enabled);

  private:
    inline bool canBeBlocked(const QUrl& url) const;

  private:
    bool m_loaded;
    bool m_enabled;
    AdBlockIcon* m_adblockIcon;
    QList<AdBlockSubscription*> m_subscriptions;
    AdBlockMatcher* m_matcher;
    QStringList m_disabledRules;
    AdBlockUrlInterceptor* m_interceptor;
    QMutex m_mutex;
};

inline AdBlockIcon* AdBlockManager::adBlockIcon() const {
  return m_adblockIcon;
}

#endif // ADBLOCKMANAGER_H
