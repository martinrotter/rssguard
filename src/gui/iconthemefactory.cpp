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
QEvent::Type IconThemeFactoryEvent::m_typeOfEvent = QEvent::None;

//
// ThemeFactoryEvent class
//

IconThemeFactoryEvent::IconThemeFactoryEvent() : QEvent(IconThemeFactoryEvent::type()) {
}

IconThemeFactoryEvent::~IconThemeFactoryEvent() {
  qDebug("Destroying IconThemeFactoryEvent.");
}

QEvent::Type IconThemeFactoryEvent::type()  {
  if (m_typeOfEvent == QEvent::None) {
    m_typeOfEvent = static_cast<QEvent::Type>(QEvent::registerEventType(2000));
  }

  return m_typeOfEvent;
}


//
// ThemeFactory class
//

IconThemeFactory::IconThemeFactory(QObject *parent)
  : QObject(parent), m_currentIconTheme(APP_THEME_SYSTEM) {
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
  QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << APP_THEME_PATH_QRC << APP_THEME_PATH);
  qDebug("Available icon theme paths: %s.",
         qPrintable(QIcon::themeSearchPaths().join(", ")));
}

QString IconThemeFactory::getCurrentIconTheme() {
  return m_currentIconTheme;
}

QIcon IconThemeFactory::fromTheme(const QString &name, const QIcon &fallback) {
  return QIcon::fromTheme(name, fallback);
}

void IconThemeFactory::setCurrentIconTheme(const QString &theme_name) {
  Settings::getInstance()->setValue(APP_CFG_GUI,
                                    "icon_theme",
                                    theme_name);
  loadCurrentIconTheme(true);
}

void IconThemeFactory::loadCurrentIconTheme(bool notify_widgets) {
  QStringList installed_themes = getInstalledIconThemes();
  QString theme_name_from_settings = Settings::getInstance()->value(APP_CFG_GUI,
                                                                    "icon_theme",
                                                                    "mini-kfaenza").toString();

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
    QIcon::setThemeName(theme_name_from_settings);
    m_currentIconTheme = theme_name_from_settings;
  }
  else {
    // Desired icon theme is not currently available.
    // Install "default" icon theme instead.
    // NOTE: "Default" icon theme is:
    //  a) system icon theme on Linux,
    //  b) no icon theme on other platforms.
    qDebug("Icon theme '%s' cannot be loaded because it is not installed. Loading 'default' theme.",
           qPrintable(theme_name_from_settings));
    QIcon::setThemeName(APP_THEME_SYSTEM);
    m_currentIconTheme = APP_THEME_SYSTEM;
  }

  // We need to deliver custom event for all widgets
  // to make sure they get a chance to setup their icons.
  if (notify_widgets) {
    foreach (QWidget *widget, QtSingleApplication::allWidgets()) {
      QtSingleApplication::postEvent((QObject*) widget,
                                     new IconThemeFactoryEvent());
    }
  }
}

QStringList IconThemeFactory::getInstalledIconThemes() {
  QStringList icon_theme_names;
  icon_theme_names << APP_THEME_SYSTEM;

  // Iterate all directories with icon themes.
  QStringList icon_themes_paths = QIcon::themeSearchPaths();
  icon_themes_paths.removeDuplicates();

  foreach (QString icon_path, icon_themes_paths) {
    QDir icon_dir(icon_path);

    // Iterate all icon themes in this directory.
    foreach (QString icon_theme_path, icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                                         QDir::Readable | QDir::CaseSensitive |
                                                         QDir::NoSymLinks,
                                                         QDir::Time)) {
      // Check if index.theme file in this path exists.
      if (QFile::exists(icon_dir.path() + "/" + icon_theme_path + "/index.theme")) {
        icon_theme_names << icon_theme_path;
      }
    }
  }

  icon_theme_names.removeDuplicates();
  return icon_theme_names;
}
