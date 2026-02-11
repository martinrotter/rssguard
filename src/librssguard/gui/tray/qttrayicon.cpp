// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/tray/qttrayicon.h"

QtTrayIcon::QtTrayIcon(const QString& id,
                       const QString& title,
                       const QPixmap& normal_icon,
                       const QPixmap& plain_icon,
                       QObject* parent)
  : TrayIcon(id, title, normal_icon, plain_icon, parent), m_trayIcon(nullptr) {
  m_tmrDoubleFire.setSingleShot(true);
  m_tmrDoubleFire.setInterval(100);
}

QtTrayIcon::~QtTrayIcon() {
  if (m_trayIcon != nullptr) {
    m_trayIcon->hide();
  }
}

TrayIcon::MessageSeverity QtTrayIcon::convertIcon(QSystemTrayIcon::MessageIcon icon) {
  switch (icon) {
    case QSystemTrayIcon::MessageIcon::NoIcon:
      return TrayIcon::MessageSeverity::NoIcon;

    case QSystemTrayIcon::MessageIcon::Information:
      return TrayIcon::MessageSeverity::Information;

    case QSystemTrayIcon::MessageIcon::Warning:
      return TrayIcon::MessageSeverity::Warning;

    case QSystemTrayIcon::MessageIcon::Critical:
      return TrayIcon::MessageSeverity::Critical;
  }

  // Fallback (should never happen unless Qt adds new enum values)
  return TrayIcon::MessageSeverity::Information;
}

QSystemTrayIcon::MessageIcon QtTrayIcon::convertIcon(MessageSeverity severity) {
  switch (severity) {
    case TrayIcon::MessageSeverity::NoIcon:
      return QSystemTrayIcon::MessageIcon::NoIcon;

    case TrayIcon::MessageSeverity::Information:
      return QSystemTrayIcon::MessageIcon::Information;

    case TrayIcon::MessageSeverity::Warning:
      return QSystemTrayIcon::MessageIcon::Warning;

    case TrayIcon::MessageSeverity::Critical:
      return QSystemTrayIcon::MessageIcon::Critical;
  }

  return QSystemTrayIcon::MessageIcon::Information;
}

void QtTrayIcon::setToolTip(const QString& tool_tip) {
  trayIcon()->setToolTip(tool_tip);
}

void QtTrayIcon::setPixmap(const QPixmap& icon) {
  trayIcon()->setIcon(QIcon(icon));
}

void QtTrayIcon::setStatus(Status status) {
  // NOTE: Not supported by Qt.
}

void QtTrayIcon::setContextMenu(TrayIconMenu* menu) {
  trayIcon()->setContextMenu(menu);
}

void QtTrayIcon::showMessage(const QString& title,
                             const QString& message,
                             MessageSeverity icon,
                             int milliseconds_timeout_hint,
                             const std::function<void()>& message_clicked_callback) {
  if (m_connection != nullptr) {
    // Disconnect previous bubble click signalling.
    disconnect(m_connection);
  }

  if (message_clicked_callback) {
    // Establish new connection for bubble click.
    m_connection = connect(trayIcon(), &QSystemTrayIcon::messageClicked, message_clicked_callback);
  }

  // NOTE: If connections do not work, then use QMetaObject::invokeMethod(...).
  trayIcon()->showMessage(title, message, convertIcon(icon), milliseconds_timeout_hint);
}

bool QtTrayIcon::isAvailable() const {
  return QSystemTrayIcon::isSystemTrayAvailable() && QSystemTrayIcon::supportsMessages();
}

void QtTrayIcon::setMainWindow(QWidget* main_window) {
  // NOTE: Not supported or needed.
}

void QtTrayIcon::show() {
  trayIcon()->show();
  emit shown();
}

void QtTrayIcon::hide() {
  trayIcon()->hide();
  emit hidden();
}

QSystemTrayIcon* QtTrayIcon::trayIcon() {
  if (m_trayIcon == nullptr) {
    m_trayIcon = new QSystemTrayIcon(m_normalIcon, this);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
      if (m_tmrDoubleFire.isActive()) {
        return;
      }

      m_tmrDoubleFire.start();

      switch (reason) {
        case QSystemTrayIcon::ActivationReason::Trigger:
        case QSystemTrayIcon::ActivationReason::DoubleClick:
        case QSystemTrayIcon::ActivationReason::MiddleClick:
          emit activated();
          break;

        default:
          break;
      }
    });
  }

  return m_trayIcon;
}
