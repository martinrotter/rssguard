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

#ifndef FOLLOWREDIRECTREPLY_H
#define FOLLOWREDIRECTREPLY_H

#include <QObject>
#include <QNetworkReply>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

class FollowRedirectReply : public QObject
{
  Q_OBJECT
public:
  explicit FollowRedirectReply(const QUrl &url, QNetworkAccessManager* manager);
  ~FollowRedirectReply();

  QNetworkReply* reply() const;
  QUrl originalUrl() const;
  QUrl url() const;

  QNetworkReply::NetworkError error() const;
  QByteArray readAll();

signals:
  void finished();

private slots:
  void replyFinished();

private:
  QNetworkAccessManager* m_manager;
  QNetworkReply* m_reply;
  int m_redirectCount;

};

#endif // FOLLOWREDIRECTREPLY_H
