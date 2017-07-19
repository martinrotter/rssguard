// This file is part of RSS Guard.
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

#ifndef SIMPLEREGEXP_H
#define SIMPLEREGEXP_H

#include <QRegularExpression>
#include <QStringList>


class SimpleRegExp : public QRegularExpression {
  public:
    explicit SimpleRegExp();
    explicit SimpleRegExp(const QString &pattern, Qt::CaseSensitivity cs = Qt::CaseSensitive);
    explicit SimpleRegExp(const SimpleRegExp &re);

    void setMinimal(bool minimal);
    int indexIn(const QString &str, int offset = 0) const;
    int matchedLength() const;
    QString cap(int nth = 0) const;

  private:
    QStringList m_capturedTexts;
    int m_matchedLength;
};

#endif // SIMPLEREGEXP_H
