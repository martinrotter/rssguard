// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/systemtrayicon.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QPainter>
#include <QTimer>

#if defined(Q_OS_WIN)
TrayIconMenu::TrayIconMenu(const QString& title, QWidget* parent) : QMenu(title, parent) {}

bool TrayIconMenu::event(QEvent* event) {
  if (event->type() == QEvent::Type::Show && QApplication::activeModalWidget() != nullptr) {
    QTimer::singleShot(0, this, &TrayIconMenu::hide);
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Close dialogs"),
                          tr("Close opened modal dialogs first."),
                          QSystemTrayIcon::MessageIcon::Warning});
  }

  return QMenu::event(event);
}

#endif

SystemTrayIcon::SystemTrayIcon(const QString& normal_icon, const QString& plain_icon, FormMain* parent)
  : QSystemTrayIcon(parent), m_normalIcon(normal_icon), m_plainPixmap(plain_icon) {
  qDebugNN << LOGSEC_GUI << "Creating SystemTrayIcon instance.";
  m_font.setBold(true);

  // Initialize icon.
  setNumber();
  setContextMenu(parent->trayMenu());

  // Create necessary connections.
  connect(this, &SystemTrayIcon::activated, this, &SystemTrayIcon::onActivated);
}

SystemTrayIcon::~SystemTrayIcon() {
  qDebugNN << LOGSEC_GUI << "Destroying SystemTrayIcon instance.";
  hide();
}

void SystemTrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason) {
  switch (reason) {
    case SystemTrayIcon::ActivationReason::Trigger:
    case SystemTrayIcon::ActivationReason::DoubleClick:
    case SystemTrayIcon::ActivationReason::MiddleClick:
      static_cast<FormMain*>(parent())->switchVisibility();
      break;

    default:
      break;
  }
}

bool SystemTrayIcon::isSystemTrayAreaAvailable() {
  return QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages();
}

bool SystemTrayIcon::isSystemTrayDesired() {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool();
}

void SystemTrayIcon::showPrivate() {
  // Make sure that application does not exit some window (for example
  // the settings window) gets closed. Behavior for main window
  // is handled explicitly by FormMain::closeEvent() method.
  qApp->setQuitOnLastWindowClosed(false);

  // Display the tray icon.
  QSystemTrayIcon::show();
  emit shown();

  qDebugNN << LOGSEC_GUI << "Tray icon displayed.";
}

void SystemTrayIcon::show() {
  // Show immediately.
  qDebugNN << LOGSEC_GUI << "Showing tray icon immediately.";
  showPrivate();
}

void SystemTrayIcon::setNumber(int number, bool any_feed_has_new_unread_messages) {
  Q_UNUSED(any_feed_has_new_unread_messages)

  if (number <= 0 || !qApp->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersInTrayIcon)).toBool()) {
    // Either no unread messages or numbers in tray icon are disabled.
    setToolTip(QSL(APP_LONG_NAME));
    QSystemTrayIcon::setIcon(QIcon(m_normalIcon));
  }
  else {
    setToolTip(tr("%1\nUnread news: %2").arg(QSL(APP_LONG_NAME), QString::number(number)));
    QPixmap background(m_plainPixmap);
    QPainter tray_painter;

    tray_painter.begin(&background);

    if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::MonochromeTrayIcon)).toBool() &&
        !qApp->settings()->value(GROUP(GUI), SETTING(GUI::ColoredBusyTrayIcon)).toBool()) {
      tray_painter.setPen(Qt::GlobalColor::white);
    }
    else {
      tray_painter.setPen(Qt::GlobalColor::black);
    }

    tray_painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
    tray_painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, true);

    // Numbers with more than 5 digits won't be readable, display
    // infinity symbol in that case.
    QString num_txt;

    if (number > 99999) {
      num_txt = QChar(8734);
      m_font.setPixelSize(background.width() * 0.78);
    }
    else if (number > 999) {
      num_txt = QSL("%1k").arg(int(number / 1000));
      m_font.setPixelSize(background.width() * 0.43);
    }
    else if (number > 99) {
      num_txt = QString::number(number);
      m_font.setPixelSize(background.width() * 0.43);
    }
    else if (number > 9) {
      num_txt = QString::number(number);
      m_font.setPixelSize(background.width() * 0.56);
    }
    else {
      num_txt = QString::number(number);
      m_font.setPixelSize(background.width() * 0.78);
    }

    tray_painter.setFont(m_font);
    tray_painter.drawText(background.rect(), Qt::AlignmentFlag::AlignCenter, num_txt);
    tray_painter.end();

    QSystemTrayIcon::setIcon(QIcon(background));
  }
}

void SystemTrayIcon::showMessage(const QString& title,
                                 const QString& message,
                                 QSystemTrayIcon::MessageIcon icon,
                                 int milliseconds_timeout_hint,
                                 const std::function<void()>& functor) {
  if (m_connection != nullptr) {
    // Disconnect previous bubble click signalling.
    disconnect(m_connection);
  }

  if (functor) {
    // Establish new connection for bubble click.
    m_connection = connect(this, &SystemTrayIcon::messageClicked, functor);
  }

  // NOTE: If connections do not work, then use QMetaObject::invokeMethod(...).
  QSystemTrayIcon::showMessage(title, message, icon, milliseconds_timeout_hint);
}
