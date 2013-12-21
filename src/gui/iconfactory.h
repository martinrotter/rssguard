#ifndef ICONFACTORY_H
#define ICONFACTORY_H

#include <QIcon>


class IconFactory {
  private:
    explicit IconFactory();

  public:
    // Used to store/retrieve QIcons from/to database.
    static QIcon fromByteArray(QByteArray array);
    static QByteArray toByteArray(const QIcon &icon);
};

#endif // ICONFACTORY_H
