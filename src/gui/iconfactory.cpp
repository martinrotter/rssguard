#include <QBuffer>

#include "gui/iconfactory.h"


IconFactory::IconFactory() {
}

QIcon IconFactory::fromByteArray(QByteArray array) {
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
  return array;
}
