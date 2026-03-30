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
  m_ui.m_lblIcon->setFixedSize({
    48,
    48,
  });

  setupHeading(m_ui.m_lblTitle);
  setupCloseButton(m_ui.m_btnClose);
  setupTimedClosing(false);

  loadNotification(event, msg, action);
}

void ToastNotification::loadNotification(Notification::Event event, const GuiMessage& msg, const GuiAction& action) {
  m_ui.m_lblTitle->setText(msg.m_title);
  m_ui.m_lblTitle->setToolTip(msg.m_title);
  m_ui.m_lblBody->setText(msg.m_message);
  m_ui.m_lblBody->setToolTip(msg.m_message);
  m_ui.m_lblIcon->setPixmap((msg.m_icon.isNull() ? iconForType(msg.m_type) : msg.m_icon)
                              .pixmap({
                                48,
                                48,
                              }));

  if (action.m_action) {
    m_ui.m_btnAction->setText(action.m_title.isEmpty() ? tr("Do it!") : action.m_title);
    connect(m_ui.m_btnAction, &QPushButton::clicked, this, [this, action]() {
      QMetaObject::invokeMethod(action.m_target, action.m_action, Qt::ConnectionType::QueuedConnection);
      emit closeRequested(this, true);
    });
  }
  else {
    m_ui.m_mainLayout->removeItem(m_ui.m_actionLayout);
    m_ui.m_mainLayout->update();
    m_ui.m_btnAction->deleteLater();
  }
}

QIcon ToastNotification::iconForType(QSystemTrayIcon::MessageIcon icon) const {
  switch (icon) {
    case QSystemTrayIcon::Warning:
      return qApp->icons()->fromTheme(QSL("dialog-warning"));

    case QSystemTrayIcon::Critical:
      return qApp->icons()->fromTheme(QSL("dialog-error"));

    case QSystemTrayIcon::Information:
      return qApp->icons()->fromTheme(QSL("dialog-information"));

    default:
      return QIcon();
  }
}
