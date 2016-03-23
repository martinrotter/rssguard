// This file is part of RSS Guard.
//
// Copyright (C) 2014-2015 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2014 by David Rosca <nowrep@gmail.com>
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

#ifndef ADBLOCKMATCHER_H
#define ADBLOCKMATCHER_H

#include <QObject>

#include <QUrl>
#include <QVector>

#include "network-web/adblock/adblocksearchtree.h"


class AdBlockManager;
class AdBlockRule;

class AdBlockMatcher : public QObject {
    Q_OBJECT

  public:
    explicit AdBlockMatcher(AdBlockManager *manager);
    virtual ~AdBlockMatcher();

    const AdBlockRule *match(const QUrl &url, const QString &url_domain, const QString &url_string,
                             const QString &referer, QWebEngineUrlRequestInfo::ResourceType resource_type) const;

    bool adBlockDisabledForUrl(const QUrl &url, const QString &referer, QWebEngineUrlRequestInfo::ResourceType resource_type) const;

  public slots:
    void update();
    void clear();

  private slots:
    void enabledChanged(bool enabled);

  private:
    AdBlockManager *m_manager;

    QVector<const AdBlockRule*> m_networkExceptionRules;
    QVector<const AdBlockRule*> m_networkBlockRules;
    QVector<const AdBlockRule*> m_domainRestrictedCssRules;
    QVector<const AdBlockRule*> m_documentRules;

    AdBlockSearchTree m_networkBlockTree;
    AdBlockSearchTree m_networkExceptionTree;
};

#endif // ADBLOCKMATCHER_H
