#include <QIcon>
#include <QApplication>

#include "gui/themefactory.h"
#include "core/defs.h"


ThemeFactory::ThemeFactory() {
}

void ThemeFactory::setupSearchPaths() {
  QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << APP_THEME_PATH);

  qDebug("Available icon theme paths: %s.",
         qPrintable(QIcon::themeSearchPaths().join(", ")));
}
