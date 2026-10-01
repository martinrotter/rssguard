// Minimal icon service for the isolated settings-widget tests.
#ifndef SHORTCUT_TEST_ICONFACTORY_H
#define SHORTCUT_TEST_ICONFACTORY_H

#include <QIcon>

class IconFactory {
  public:
    QIcon fromTheme(const QString&) const {
      return {};
    }
};

#endif
