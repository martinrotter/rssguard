// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GLOBALS_H
#define GLOBALS_H

#include "definitions/definitions.h"

#include <QMetaEnum>
#include <QMetaObject>
#include <QString>

class Globals {
  public:
    template <typename T, typename U>
    static bool hasFlag(T value, U flag);

  private:
    Globals();
};

template <typename T, typename U>
inline bool Globals::hasFlag(T value, U flag) {
  return (int(value) & int(flag)) == int(flag);
}

template <typename EnumType>
QString enumToString(EnumType value) {
  const QMetaEnum enumer = QMetaEnum::fromType<EnumType>();
  const QString key = enumer.valueToKey(int(value));

  return key.isEmpty() ? QString::number(int(value)) : key;
}

template <typename EnumType>
EnumType stringToEnum(const QString& value) {
  const QByteArray value_data = value.toLocal8Bit();
  const QMetaEnum enumer = QMetaEnum::fromType<EnumType>();
  const int en = enumer.keyToValue(value_data.constData());

  return EnumType(en);
}

template <typename EnumType>
QList<QPair<EnumType, QString>> enumToStrings() {
  const QMetaEnum enumer = QMetaEnum::fromType<EnumType>();

  QList<QPair<EnumType, QString>> strings;

  for (int i = 0; i < enumer.keyCount(); i++) {
    strings.append({static_cast<EnumType>(enumer.value(i)), QString(enumer.key(i))});
  }

  return strings;
}

#endif // GLOBALS_H
