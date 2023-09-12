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

  setupCloseButton(m_ui.m_btnClose);
  loadNotification(event, msg, action);
}

void ToastNotification::loadNotification(Notification::Event event, const GuiMessage& msg, const GuiAction& action) {
  m_ui.m_lblTitle->setText(msg.m_title);
  m_ui.m_lblBody->setText(msg.m_message);
}
