#ifndef ICONFACTORY_H
#define ICONFACTORY_H

#include <QIcon>


class IconFactory {
  private:
    IconFactory();

  public:
    static QIcon fromByteArray(QByteArray array);
    static QByteArray toByteArray(const QIcon &icon);

};

#endif // ICONFACTORY_H
