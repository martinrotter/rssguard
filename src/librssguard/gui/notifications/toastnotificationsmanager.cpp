// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/toastnotificationsmanager.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/notifications/basetoastnotification.h"
#include "gui/notifications/toastnotification.h"

#include <QRect>
#include <QScreen>
#include <QWindow>

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
  ToastNotification* notif = new ToastNotification(event, msg, action, qApp->mainFormWidget());

  auto* screen = moveToProperScreen(notif);

  // Insert new notification into free space.
  notif->show();

  auto notif_new_pos = cornerForNewNotification(screen->availableGeometry());

  moveNotificationToCorner(notif, notif_new_pos);

  notif->move(notif_new_pos);

  // Make sure notification is finally resized.
  notif->adjustSize();
  qApp->processEvents();

  // Remove out-of-bounds old notifications and shift existing
  // ones to make space for new notifications.
  removeOutOfBoundsNotifications(notif->height());
  makeSpaceForNotification(notif->height());

  m_activeNotifications.prepend(notif);
}

void ToastNotificationsManager::showNotification(const QList<Message>& new_messages) {}

QScreen* ToastNotificationsManager::activeScreen() const {
  if (m_screen >= 0) {
    auto all_screens = QGuiApplication::screens();

    if (m_screen < all_screens.size()) {
      return all_screens.at(m_screen);
    }
  }

  return QGuiApplication::primaryScreen();
}

QPoint ToastNotificationsManager::cornerForNewNotification(QRect rect) {
  switch (m_position) {
    case ToastNotificationsManager::TopLeft:
      return rect.topLeft() + QPoint(NOTIFICATIONS_MARGIN, NOTIFICATIONS_MARGIN);

    case ToastNotificationsManager::TopRight:
      return rect.topRight() - QPoint(NOTIFICATIONS_WIDTH + NOTIFICATIONS_MARGIN, -NOTIFICATIONS_MARGIN);

    case ToastNotificationsManager::BottomLeft:
      return rect.bottomLeft() - QPoint(-NOTIFICATIONS_MARGIN, NOTIFICATIONS_MARGIN);

    case ToastNotificationsManager::BottomRight:
      return rect.bottomRight() - QPoint(NOTIFICATIONS_MARGIN, NOTIFICATIONS_MARGIN);
  }
}

void ToastNotificationsManager::moveNotificationToCorner(BaseToastNotification* notif, const QPoint& corner) {
  switch (m_position) {
    case ToastNotificationsManager::TopLeft:
      notif->move(corner);
      break;

    case ToastNotificationsManager::TopRight:
      notif->move(corner);
      break;

    case ToastNotificationsManager::BottomLeft:
      notif->move(corner);
      break;

    case ToastNotificationsManager::BottomRight:
      notif->move(corner);
      break;
  }
}

void ToastNotificationsManager::makeSpaceForNotification(int height_to_make_space) {
  for (BaseToastNotification* notif : m_activeNotifications) {
    notif->move(notif->pos().x(), notif->pos().y() + height_to_make_space + NOTIFICATIONS_MARGIN);
  }
}

void ToastNotificationsManager::removeOutOfBoundsNotifications(int height_to_reserve) {
  auto* screen = activeScreen();

  int available_height = screen->availableSize().height();

  while (boolinq::from(m_activeNotifications).sum([](BaseToastNotification* notif) {
    return notif->height() + NOTIFICATIONS_MARGIN;
  }) + height_to_reserve >
         available_height) {
    if (!m_activeNotifications.isEmpty()) {
      m_activeNotifications.takeLast()->deleteLater();
    }
    else {
      break;
    }
  }
}

QScreen* ToastNotificationsManager::moveToProperScreen(BaseToastNotification* notif) {
  if (m_screen >= 0) {
    auto all_screens = QGuiApplication::screens();

    if (m_screen < all_screens.size()) {
      notif->windowHandle()->setScreen(all_screens.at(m_screen));

      return all_screens.at(m_screen);
    }
  }

  return QGuiApplication::primaryScreen();
}
