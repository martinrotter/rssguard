// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/toastnotificationsmanager.h"

#include "gui/notifications/toastnotification.h"

ToastNotificationsManager::ToastNotificationsManager(QObject* parent)
  : QObject(parent), m_position(NotificationPosition::BottomRight), m_screen(-1) {}

ToastNotificationsManager::~ToastNotificationsManager() {
  clear();
}

QList<BaseToastNotification*> ToastNotificationsManager::activeNotifications() const {
  return m_activeNotifications;
}

int ToastNotificationsManager::screen() const {
  return m_screen;
}

void ToastNotificationsManager::setScreen(int screen) {
  m_screen = screen;
}

ToastNotificationsManager::NotificationPosition ToastNotificationsManager::position() const {
  return m_position;
}

void ToastNotificationsManager::setPosition(NotificationPosition position) {
  m_position = position;
}

void ToastNotificationsManager::clear() {
  for (BaseToastNotification* nt : m_activeNotifications) {
    nt->close();
    nt->deleteLater();
  }

  m_activeNotifications.clear();
}

void ToastNotificationsManager::showNotification(Notification::Event event,
                                                 const GuiMessage& msg,
                                                 const GuiAction& action) {
  // Remove top existing notifications as long as their combined height with height of this
  // new notification extends.

  ToastNotification* notif = new ToastNotification(event, msg, action, qApp->mainFormWidget());

  auto aa = notif->height();

  notif->show();

  auto bb = notif->height();
  auto cc = notif->height();
}

void ToastNotificationsManager::showNotification(const QList<Message>& new_messages) {}
