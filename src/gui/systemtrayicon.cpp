#include "gui/systemtrayicon.h"
#include "gui/formmain.h"
#include "core/settings.h"
#include "core/defs.h"


QPointer<SystemTrayIcon> SystemTrayIcon::m_trayIcon;

SystemTrayIcon::SystemTrayIcon(QObject *parent) : QSystemTrayIcon(parent) {
  qDebug("Creating SystemTrayIcon instance.");
}

SystemTrayIcon::~SystemTrayIcon() {
  qDebug("Destroying SystemTrayIcon instance.");
}

bool SystemTrayIcon::isSystemTrayAvailable() {
  return QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages();
}

bool SystemTrayIcon::isSystemTrayActivated() {
  return SystemTrayIcon::isSystemTrayAvailable() && Settings::getInstance()->value(APP_CFG_GUI,
                                                                                   "use_tray_icon",
                                                                                   true).toBool();
}

SystemTrayIcon *SystemTrayIcon::getInstance() {
  if (m_trayIcon.isNull()) {
    m_trayIcon = new SystemTrayIcon(FormMain::getInstance());
  }

  return m_trayIcon;
}

