// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef MESSAGE_H
#define MESSAGE_H

#include "definitions/definitions.h"

#include <QDateTime>
#include <QStringList>


// Represents single enclosure.
struct Enclosure {
    QString m_url;
    QString m_mimeType;

    explicit Enclosure(const QString &url = QString(), const QString &mime = QString());
};

// Represents single enclosure.
class Enclosures {
  public:
    static QList<Enclosure> decodeEnclosuresFromString(const QString &enclosures_data);
    static QString encodeEnclosuresToString(const QList<Enclosure> &enclosures);
};

// Represents single message.
class Message {
  public:
    explicit Message();

    QString m_title;
    QString m_url;
    QString m_author;
    QString m_contents;
    QDateTime m_created;
    QString m_feedId;
    int m_accountId;
    int m_id;
    QString m_customId;

    bool m_isRead;
    bool m_isImportant;

    QList<Enclosure> m_enclosures;

    // Is true if "created" date was obtained directly
    // from the feed, otherwise is false
    bool m_createdFromFeed;
};

#endif // MESSAGE_H
