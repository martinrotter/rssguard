// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FEEDPARSER_H
#define FEEDPARSER_H

#include <QDomDocument>
#include <QString>

#include "core/message.h"


class FeedParser {
  public:
    explicit FeedParser(const QString &data);
    virtual ~FeedParser();

    virtual QList<Message> messages();

  protected:
    QStringList textsFromPath(const QDomElement &element, const QString &namespace_uri, const QString &xml_path, bool only_first) const;
    virtual QDomNodeList messageElements() = 0;
    virtual QString feedAuthor() const;
    virtual Message extractMessage(const QDomElement &msg_element, QDateTime current_time) const = 0;

  protected:
    QString m_xmlData;
    QDomDocument m_xml;
};

#endif // FEEDPARSER_H
