#include <QString>

#include "core/defs.h"
#include "core/textfactory.h"


TextFactory::TextFactory() {
}

TextFactory::~TextFactory() {
}

QString TextFactory::shorten(const QString &input) {
  if (input.size() > TEXT_TITLE_LIMIT) {
    return input.left(TEXT_TITLE_LIMIT - 3) + QString(3, '.');
  }
  else {
    return input;
  }
}
