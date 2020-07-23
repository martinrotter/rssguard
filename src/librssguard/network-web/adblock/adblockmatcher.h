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

#ifndef ADBLOCKMATCHER_H
#define ADBLOCKMATCHER_H

#include <QUrl>

#include "network-web/adblock/adblocksearchtree.h"

#include <QObject>
#include <QVector>

class QWebEngineUrlRequestInfo;
class AdBlockManager;

class AdBlockMatcher : public QObject {
  Q_OBJECT

  public:
    explicit AdBlockMatcher(AdBlockManager* manager);
    virtual ~AdBlockMatcher();

    const AdBlockRule* match(const QWebEngineUrlRequestInfo& request, const QString& urlDomain, const QString& urlString) const;

    bool adBlockDisabledForUrl(const QUrl& url) const;
    bool elemHideDisabledForUrl(const QUrl& url) const;

    QString elementHidingRules() const;
    QString elementHidingRulesForDomain(const QString& domain) const;

  public slots:
    void update();
    void clear();

  private:
    AdBlockManager* m_manager;

    QVector<AdBlockRule*> m_createdRules;
    QVector<const AdBlockRule*> m_networkExceptionRules;
    QVector<const AdBlockRule*> m_networkBlockRules;
    QVector<const AdBlockRule*> m_domainRestrictedCssRules;
    QVector<const AdBlockRule*> m_documentRules;
    QVector<const AdBlockRule*> m_elemhideRules;

    QString m_elementHidingRules;
    AdBlockSearchTree m_networkBlockTree;
    AdBlockSearchTree m_networkExceptionTree;
};

#endif // ADBLOCKMATCHER_H
