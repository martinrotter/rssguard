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
