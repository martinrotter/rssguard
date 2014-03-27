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

#include "miscellaneous/iconfactory.h"

#include <QBuffer>


IconFactory::IconFactory() {
}

QIcon IconFactory::fromByteArray(QByteArray array) {
  array = QByteArray::fromBase64(array);

  QIcon icon;
  QBuffer buffer(&array);
  buffer.open(QIODevice::ReadOnly);

  QDataStream in(&buffer);
  in >> icon;

  buffer.close();
  return icon;
}

QByteArray IconFactory::toByteArray(const QIcon &icon) {
  QByteArray array;
  QBuffer buffer(&array);
  buffer.open(QIODevice::WriteOnly);

  QDataStream out(&buffer);
  out << icon;

  buffer.close();
  return array.toBase64();
}
