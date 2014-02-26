// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef ICONFACTORY_H
#define ICONFACTORY_H

#include <QIcon>


class IconFactory {
  private:
    // Constructors and destructors.
    explicit IconFactory();

  public:
    // Used to store/retrieve QIcons from/to database via Base64-encoded
    // byte array.
    static QIcon fromByteArray(QByteArray array);
    static QByteArray toByteArray(const QIcon &icon);
};

#endif // ICONFACTORY_H
