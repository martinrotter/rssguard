#include <QPainter>

#include "gui/systemtrayicon.h"
#include "gui/formmain.h"
#include "core/settings.h"
#include "core/defs.h"


QPointer<SystemTrayIcon> SystemTrayIcon::m_trayIcon;

SystemTrayIcon::SystemTrayIcon(const QString &normal_icon,
                               const QString &plain_icon,
                               QObject *parent)
  : QSystemTrayIcon(parent), m_normalIcon(normal_icon), m_plainIcon(plain_icon) {
  qDebug("Creating SystemTrayIcon instance.");

  // Initialize icon.
  setNumber();
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
    m_trayIcon = new SystemTrayIcon(APP_ICON_PATH,
                                    APP_ICON_PLAIN_PATH,
                                    FormMain::getInstance());
  }

  return m_trayIcon;
}

void SystemTrayIcon::setNumber(int number) {
  if (number < 0) {
    QSystemTrayIcon::setIcon(QIcon(m_normalIcon));
  }
  else {
    QPixmap background = QPixmap(APP_ICON_PLAIN_PATH);
    QPainter trayPainter;
    QFont font = QFont();

    font.setBold(true);
    trayPainter.begin(&background);
    trayPainter.setPen(Qt::black);

    // Numbers with more than 2 digits won't be readable, display
    // infinity symbol in that case.
    if (number > 99) {
      font.setPixelSize(90);
      trayPainter.setFont(font);
      trayPainter.drawText(QRect(0, 0, 128, 128),
                           Qt::AlignVCenter | Qt::AlignCenter ,
                           "∞");
    }
    else {
      // Smaller number if it has 2 digits.
      if (number > 9) {
        font.setPixelSize(70);
      }
      // Bigger number if it has just one digit.
      else {
        font.setPixelSize(90);
      }

      trayPainter.setFont(font);
      trayPainter.drawText(QRect(0, 0, 128, 128),
                           Qt::AlignVCenter | Qt::AlignCenter ,
                           QString::number(number));
    }

    trayPainter.end();

    QSystemTrayIcon::setIcon(QIcon(background));
  }
}
