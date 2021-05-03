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
class AdBlockUrlInterceptor;
class AdBlockIcon;

class AdBlockManager : public QObject {
  Q_OBJECT

  public:
    explicit AdBlockManager(QObject* parent = nullptr);

    // If "initial_load" is false, then we want to explicitly turn off
    // Adblock if it is running or turn on when not running.
    // if "initial_load" is true, then we want to forcefully perform
    // initial loading of Adblock.
    void load(bool initial_load);

    // General method for adblocking. Returns true if request should be blocked.
    bool block(const AdblockRequestInfo& request);

    bool isEnabled() const;
    bool canRunOnScheme(const QString& scheme) const;

    QString elementHidingRulesForDomain(const QUrl& url) const;
    QString generateJsForElementHiding(const QString& css) const;

    AdBlockIcon* adBlockIcon() const;

  public slots:
    void showDialog();

  signals:
    void enabledChanged(bool enabled);

  private slots:
    void updateUnifiedFiltersFile();

  private:
    bool m_loaded;
    bool m_enabled;
    AdBlockIcon* m_adblockIcon;
    AdBlockUrlInterceptor* m_interceptor;
    QMutex m_mutex;
    QString m_unifiedFiltersFile;
};

inline AdBlockIcon* AdBlockManager::adBlockIcon() const {
  return m_adblockIcon;
}

#endif // ADBLOCKMANAGER_H
