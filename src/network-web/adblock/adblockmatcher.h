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
* Copyright (C) 2014  David Rosca <nowrep@gmail.com>
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
#ifndef ADBLOCKMATCHER_H
#define ADBLOCKMATCHER_H

#include <QUrl>
#include <QObject>
#include <QVector>

#include "adblocksearchtree.h"

class AdBlockManager;
class AdBlockRule;

class AdBlockMatcher : public QObject
{
  Q_OBJECT

public:
  explicit AdBlockMatcher(AdBlockManager* manager);
  ~AdBlockMatcher();

  const AdBlockRule* match(const QNetworkRequest &request, const QString &urlDomain, const QString &urlString) const;

  bool adBlockDisabledForUrl(const QUrl &url) const;
  bool elemHideDisabledForUrl(const QUrl &url) const;

  QString elementHidingRules() const;
  QString elementHidingRulesForDomain(const QString &domain) const;

public slots:
  void update();
  void clear();

private slots:
  void enabledChanged(bool enabled);

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
