// Keep settings-widget tests independent of application startup and user data.
#ifndef SHORTCUT_TEST_APPLICATION_H
#define SHORTCUT_TEST_APPLICATION_H

#include "definitions/definitions.h"
#include "miscellaneous/iconfactory.h"

#include <QApplication>

class Settings;

class ShortcutSettingsApplication : public QApplication {
  public:
    using QApplication::QApplication;
    IconFactory* icons() {
      return &m_icons;
    }
    Settings* settings() {
      return m_settings;
    }
    void setSettings(Settings* settings) {
      m_settings = settings;
    }

  private:
    IconFactory m_icons;
    Settings* m_settings = nullptr;
};

#undef qApp
#define qApp static_cast<ShortcutSettingsApplication*>(QCoreApplication::instance())

#endif
