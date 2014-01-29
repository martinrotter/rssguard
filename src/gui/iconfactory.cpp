#include "gui/iconfactory.h"

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
