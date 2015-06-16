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
#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#define ADBLOCK_EASYLIST_URL "https://easylist-downloads.adblockplus.org/easylist.txt"

#ifndef QSL
#if QT_VERSION >= 0x050000
#define QSL(x) QStringLiteral(x)
#else
#define QSL(x) QLatin1String(x)
#endif
#endif

#ifndef QL1S
#define QL1S(x) QLatin1String(x)
#endif

#ifndef QL1C
#define QL1C(x) QLatin1Char(x)
#endif


#include <QObject>
#include <QStringList>
#include <QPointer>

class QUrl;
class QNetworkReply;
class QNetworkRequest;

class AdBlockRule;
class AdBlockDialog;
class AdBlockMatcher;
class AdBlockCustomList;
class AdBlockSubscription;

class AdBlockManager : public QObject
{
  Q_OBJECT

public:
  AdBlockManager(QObject* parent = 0);
  ~AdBlockManager();

  static AdBlockManager* instance();

  static QString filterCharsFromFilename(const QString &name);

  static QString ensureUniqueFilename(const QString &name, const QString &appendFormat = QString("(%1)"));

  void load();
  void save();

  bool isEnabled() const;
  bool canRunOnScheme(const QString &scheme) const;

  bool useLimitedEasyList() const;
  void setUseLimitedEasyList(bool useLimited);

  QString elementHidingRules() const;
  QString elementHidingRulesForDomain(const QUrl &url) const;

  AdBlockSubscription* subscriptionByName(const QString &name) const;
  QList<AdBlockSubscription*> subscriptions() const;

  QNetworkReply* block(const QNetworkRequest &request);

  QStringList disabledRules() const;
  void addDisabledRule(const QString &filter);
  void removeDisabledRule(const QString &filter);

  AdBlockSubscription* addSubscription(const QString &title, const QString &url);
  bool removeSubscription(AdBlockSubscription* subscription);

  AdBlockCustomList* customList() const;

signals:
  void enabledChanged(bool enabled);

public slots:
  void setEnabled(bool enabled);
  void showRule();

  void updateAllSubscriptions();

  AdBlockDialog* showDialog();

private:
  inline bool canBeBlocked(const QUrl &url) const;

  bool m_loaded;
  bool m_enabled;
  bool m_useLimitedEasyList;

  QList<AdBlockSubscription*> m_subscriptions;
  static AdBlockManager* s_adBlockManager;
  AdBlockMatcher* m_matcher;
  QStringList m_disabledRules;

  QPointer<AdBlockDialog> m_adBlockDialog;
};

#endif // ADBLOCKMANAGER_H

