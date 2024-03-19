// For license of this file, see <project-root-folder>/LICENSE.md.

/****************************************************************************
**
** Copyright (C) 2016 Giuseppe D'Angelo <dangelog@gmail.com>.
** Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo
*<giuseppe.dangelo@kdab.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "miscellaneous/regexfactory.h"

QString RegexFactory::wildcardToRegularExpression(const QString& pattern) {
  const int wclen = pattern.length();
  QString rx;

  rx.reserve(wclen + wclen / 16);
  int i = 0;
  const QChar* wc = pattern.unicode();

#if defined(Q_OS_WIN)
  const QLatin1Char nativePathSeparator('\\');
  const QLatin1String starEscape("[^/\\\\]*");
  const QLatin1String questionMarkEscape("[^/\\\\]");
#else
  const QLatin1Char nativePathSeparator('/');
  const QLatin1String starEscape("[^/]*");
  const QLatin1String questionMarkEscape("[^/]");
#endif

  while (i < wclen) {
    const QChar c = wc[i++];

    switch (c.unicode()) {
      case '*':
        rx += starEscape;
        break;

      case '?':
        rx += questionMarkEscape;
        break;

      case '\\':
#if defined(Q_OS_WIN)
      case '/':
        rx += QLatin1String("[/\\\\]");
        break;
#endif
      case '$':
      case '(':
      case ')':
      case '+':
      case '.':
      case '^':
      case '{':
      case '|':
      case '}':
        rx += QLatin1Char('\\');
        rx += c;
        break;

      case '[':
        rx += c;

        // Support for the [!abc] or [!a-c] syntax
        if (i < wclen) {
          if (wc[i] == QLatin1Char('!')) {
            rx += QLatin1Char('^');
            ++i;
          }

          if (i < wclen && wc[i] == QLatin1Char(']'))
            rx += wc[i++];

          while (i < wclen && wc[i] != QLatin1Char(']')) {
            // The '/' appearing in a character class invalidates the
            // regular expression parsing. It also concerns '\\' on
            // Windows OS types.
            if (wc[i] == QLatin1Char('/') || wc[i] == nativePathSeparator)
              return rx;

            if (wc[i] == QLatin1Char('\\'))
              rx += QLatin1Char('\\');

            rx += wc[i++];
          }
        }

        break;

      default:
        rx += c;
        break;
    }
  }

  return anchoredPattern(rx);
}
