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

#ifndef FOLLOWREDIRECTREPLY_H
#define FOLLOWREDIRECTREPLY_H

#include <QObject>

#include <QNetworkReply>


class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

class FollowRedirectReply : public QObject {
    Q_OBJECT

  public:
    explicit FollowRedirectReply(const QUrl &url, QNetworkAccessManager* manager);
    virtual ~FollowRedirectReply();

    QNetworkReply *reply() const;
    QUrl originalUrl() const;
    QUrl url() const;

    QNetworkReply::NetworkError error() const;
    QByteArray readAll();

  signals:
    void finished();

  private slots:
    void replyFinished();

  private:
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_reply;
    int m_redirectCount;
};

#endif // FOLLOWREDIRECTREPLY_H
