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

#ifndef PARSINGFACTORY_H
#define PARSINGFACTORY_H

#include "core/messagesmodel.h"

#include <QList>


// This class contains methods to
// parse input Unicode textual data into
// another objects.
//
// NOTE: Each parsed message MUST CONTAINT THESE FIELDS (fields
// of Message class:
//  a) m_created,
//  b) m_title.
class ParsingFactory {
  private:
    // Constructors and destructors.
    explicit ParsingFactory();

  public:
    // Parses input textual data into Message objects.
    // NOTE: Input is correctly encoded in Unicode.
    static QList<Message> parseAsATOM10(const QString &data);
    static QList<Message> parseAsRDF(const QString &data);
    static QList<Message> parseAsRSS20(const QString &data);
};

#endif // PARSINGFACTORY_H
