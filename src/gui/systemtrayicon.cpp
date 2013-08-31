#include <QPainter>
#include <QTimer>
#include <QMessageBox>

#include "gui/systemtrayicon.h"
#include "gui/formmain.h"
#include "gui/formsettings.h"
#include "core/settings.h"
#include "core/defs.h"
#include "qtsingleapplication/qtsingleapplication.h"


#if defined(Q_OS_WIN)
TrayIconMenu::TrayIconMenu(const QString &title, QWidget *parent)
  : QMenu(title, parent) {
}

TrayIconMenu::~TrayIconMenu() {
}

bool TrayIconMenu::event(QEvent *event) {
  if (QtSingleApplication::activeModalWidget() != NULL &&
      event->type() == QEvent::Show) {
    QTimer::singleShot(0, this, SLOT(hide()));
    SystemTrayIcon::getInstance()->showMessage(APP_LONG_NAME,
                                               tr("Close opened modal dialogs first."),
                                               QSystemTrayIcon::Warning);
  }
  return QMenu::event(event);
}
#endif

QPointer<SystemTrayIcon> SystemTrayIcon::s_trayIcon;

SystemTrayIcon::SystemTrayIcon(const QString &normal_icon,
                               const QString &plain_icon,
                               FormMain *parent)
  : QSystemTrayIcon(parent), m_normalIcon(normal_icon), m_plainIcon(plain_icon) {
  qDebug("Creating SystemTrayIcon instance.");

  // Initialize icon.
  setNumber();
  setContextMenu(parent->getTrayMenu());

  // Create necessary connections.
  connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
}

SystemTrayIcon::~SystemTrayIcon() {
  qDebug("Destroying SystemTrayIcon instance.");
  hide();
}

void SystemTrayIcon::onActivated(const QSystemTrayIcon::ActivationReason &reason) {
  switch (reason) {
    case SystemTrayIcon::Trigger:
    case SystemTrayIcon::DoubleClick:
    case SystemTrayIcon::MiddleClick:
      static_cast<FormMain*>(parent())->switchVisibility();
    default:
      break;
  }
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
  if (s_trayIcon.isNull()) {
    s_trayIcon = new SystemTrayIcon(APP_ICON_PATH,
                                    APP_ICON_PLAIN_PATH,
                                    FormMain::getInstance());
  }

  return s_trayIcon;
}

void SystemTrayIcon::deleteInstance() {
  if (!s_trayIcon.isNull()) {
    qDebug("Disabling tray icon and raising main application window.");
    static_cast<FormMain*>((*s_trayIcon).parent())->display();
    delete s_trayIcon.data();

    // Make sure that application quits when last window is closed.
    qApp->setQuitOnLastWindowClosed(true);
  }
}

void SystemTrayIcon::showPrivate() {
  // Make sure that application does not exit some window (for example
  // the settings window) gets closed. Behavior for main window
  // is handled explicitly by FormMain::closeEvent() method.
  qApp->setQuitOnLastWindowClosed(false);

  // Display the tray icon.
  QSystemTrayIcon::show();
  qDebug("Tray icon displayed.");
}

void SystemTrayIcon::show() {
#if defined(Q_OS_WIN)
  // Show immediately.
  qDebug("Showing tray icon immediately.");
  showPrivate();
#else
  // Delay avoids race conditions and tray icon is properly displayed.
  qDebug("Showing tray icon with 1000 ms delay.");
  QTimer::singleShot(1000,
                     this, SLOT(showPrivate()));
#endif
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
