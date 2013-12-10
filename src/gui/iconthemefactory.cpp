#include <QIcon>
#include <QFile>
#include <QDir>
#include <QPointer>
#include <QApplication>

#include "gui/iconthemefactory.h"
#include "qtsingleapplication/qtsingleapplication.h"
#include "core/settings.h"
#include "core/defs.h"


QPointer<IconThemeFactory> IconThemeFactory::s_instance;

IconThemeFactory::IconThemeFactory(QObject *parent)
  : QObject(parent), m_currentIconTheme(APP_THEME_DEFAULT) {
}

IconThemeFactory::~IconThemeFactory() {
  qDebug("Destroying IconThemeFactory instance.");
}

IconThemeFactory *IconThemeFactory::getInstance() {
  if (s_instance.isNull()) {
    s_instance = new IconThemeFactory(qApp);
  }

  return s_instance;
}

void IconThemeFactory::setupSearchPaths() {
  QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << APP_THEME_PATH);
  qDebug("Available icon theme paths: %s.",
         qPrintable(QIcon::themeSearchPaths().join(", ")));
}

QString IconThemeFactory::getCurrentIconTheme() {
  return m_currentIconTheme;
}

QIcon IconThemeFactory::fromTheme(const QString &name) {
  if (m_currentIconTheme == APP_NO_THEME) {
    return QIcon();
  }

  if (!m_cachedIcons.contains(name)) {
    // Icon is not cached yet.
    m_cachedIcons.insert(name, QIcon(APP_THEME_PATH + QDir::separator() +
                                     m_currentIconTheme + QDir::separator() +
                                     name + APP_THEME_SUFFIX));
  }

  return m_cachedIcons.value(name);
}

void IconThemeFactory::setCurrentIconTheme(const QString &theme_name) {
  Settings::getInstance()->setValue(APP_CFG_GUI,
                                    "icon_theme",
                                    theme_name);
}

void IconThemeFactory::loadCurrentIconTheme() {
  QStringList installed_themes = getInstalledIconThemes();
  QString theme_name_from_settings = Settings::getInstance()->value(APP_CFG_GUI,
                                                                    "icon_theme",
                                                                    APP_THEME_DEFAULT).toString();

  if (m_currentIconTheme == theme_name_from_settings) {
    qDebug("Icon theme '%s' already loaded.",
           qPrintable(theme_name_from_settings));
    return;
  }

  // Display list of installed themes.
  qDebug("Installed icon themes are: %s.",
         qPrintable(installed_themes.join(", ")));

  if (installed_themes.contains(theme_name_from_settings)) {
    // Desired icon theme is installed and can be loaded.
    qDebug("Loading icon theme '%s'.", qPrintable(theme_name_from_settings));
    m_currentIconTheme = theme_name_from_settings;
  }
  else {
    // Desired icon theme is not currently available.
    // Install "default" icon theme instead.
    qDebug("Icon theme '%s' cannot be loaded because it is not installed. "
           "No icon theme is loaded now.",
           qPrintable(theme_name_from_settings));
    m_currentIconTheme = APP_NO_THEME;
  }
}

QStringList IconThemeFactory::getInstalledIconThemes() {
  QStringList icon_theme_names;
  icon_theme_names << APP_NO_THEME;

  // Iterate all directories with icon themes.
  QStringList icon_themes_paths = QIcon::themeSearchPaths();
  icon_themes_paths.removeDuplicates();

  foreach (const QString &icon_path, icon_themes_paths) {
    QDir icon_dir(icon_path);

    // Iterate all icon themes in this directory.
    foreach (const QString &icon_theme_path, icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                                         QDir::Readable | QDir::CaseSensitive |
                                                         QDir::NoSymLinks,
                                                         QDir::Time)) {
      icon_theme_names << icon_theme_path;
    }
  }

  icon_theme_names.removeDuplicates();
  return icon_theme_names;
}
