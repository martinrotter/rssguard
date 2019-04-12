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

#include "miscellaneous/simpleregexp.h"

SimpleRegExp::SimpleRegExp()
  : QRegularExpression(QString(), QRegularExpression::DotMatchesEverythingOption), m_matchedLength(-1) {}

SimpleRegExp::SimpleRegExp(const QString& pattern, Qt::CaseSensitivity cs)
  : QRegularExpression(pattern, QRegularExpression::DotMatchesEverythingOption), m_matchedLength(-1) {
  if (cs == Qt::CaseInsensitive) {
    setPatternOptions(patternOptions() | QRegularExpression::CaseInsensitiveOption);
  }
}

SimpleRegExp::SimpleRegExp(const SimpleRegExp& re)
  : QRegularExpression(re), m_matchedLength(-1) {}

void SimpleRegExp::setMinimal(bool minimal) {
  QRegularExpression::PatternOptions opt;

  if (minimal) {
    opt = patternOptions() | QRegularExpression::InvertedGreedinessOption;
  }
  else {
    opt = patternOptions() & ~QRegularExpression::InvertedGreedinessOption;
  }

  setPatternOptions(opt);
}

int SimpleRegExp::indexIn(const QString& str, int offset) const {
  auto* that = const_cast<SimpleRegExp*>(this);
  QRegularExpressionMatch m = match(str, offset);

  if (!m.hasMatch()) {
    that->m_matchedLength = -1;
    that->m_capturedTexts.clear();
    return -1;
  }
  else {
    that->m_matchedLength = m.capturedLength();
    that->m_capturedTexts = m.capturedTexts();
    return m.capturedStart();
  }
}

int SimpleRegExp::matchedLength() const {
  return m_matchedLength;
}

QString SimpleRegExp::cap(int nth) const {
  if (nth >= 0 && m_capturedTexts.size() > nth) {
    return m_capturedTexts.at(nth);
  }
  else {
    return QString();
  }
}
