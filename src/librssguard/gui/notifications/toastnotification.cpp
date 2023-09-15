// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/toastnotification.h"

#include "miscellaneous/iconfactory.h"

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

ToastNotification::ToastNotification(Notification::Event event,
                                     const GuiMessage& msg,
                                     const GuiAction& action,
                                     QWidget* parent)
  : BaseToastNotification(parent) {
  m_ui.setupUi(this);

  auto fon = m_ui.m_lblTitle->font();

  fon.setBold(true);
  // fon.s

  m_ui.m_lblTitle->setFont(fon);

  setupCloseButton(m_ui.m_btnClose);
  setupTimedClosing();

  loadNotification(event, msg, action);
}

bool ToastNotification::alwaysOnTop() const {
  return false;
}

void ToastNotification::loadNotification(Notification::Event event, const GuiMessage& msg, const GuiAction& action) {
  m_ui.m_lblTitle->setText(msg.m_title);
  m_ui.m_lblBody->setText(msg.m_message);

  m_ui.m_lblIcon->setPixmap(iconForType(msg.m_type)
                              .pixmap({
                                32,
                                32,
                              }));
}

QIcon ToastNotification::iconForType(QSystemTrayIcon::MessageIcon icon) const {
  switch (icon) {
    case QSystemTrayIcon::Warning:
      return qApp->icons()->fromTheme(QSL("dialog-warning"));

    case QSystemTrayIcon::Critical:
      return qApp->icons()->fromTheme(QSL("dialog-error"));

    case QSystemTrayIcon::Information:
    case QSystemTrayIcon::NoIcon:
    default:
      return qApp->icons()->fromTheme(QSL("dialog-information"));
  }
}
