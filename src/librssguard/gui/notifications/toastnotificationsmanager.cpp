// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/toastnotificationsmanager.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/notifications/basetoastnotification.h"

#include "gui/notifications/articlelistnotification.h"
#include "gui/notifications/toastnotification.h"

#include <QRect>
#include <QScreen>
#include <QWindow>

ToastNotificationsManager::ToastNotificationsManager(NotificationPosition position, int screen, QObject* parent)
  : QObject(parent), m_position(position), m_screen(screen), m_articleListNotification(nullptr) {}

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
  for (BaseToastNotification* notif : m_activeNotifications) {
    closeNotification(notif, true);
  }

  m_activeNotifications.clear();
}

void ToastNotificationsManager::showNotification(Notification::Event event,
                                                 const GuiMessage& msg,
                                                 const GuiAction& action) {
  ToastNotification* notif = new ToastNotification(event, msg, action, qApp->mainFormWidget());

  hookNotification(notif);

  auto* screen = moveToProperScreen(notif);

  // Insert new notification into free space.
  notif->show();

  auto notif_new_pos = cornerForNewNotification(screen->availableGeometry());

  // Make sure notification is finally resized.
  notif->adjustSize();
  qApp->processEvents();

  // Move notification, at this point we already need to know its precise size.
  moveNotificationToCorner(notif, notif_new_pos);

  // Remove out-of-bounds old notifications and shift existing
  // ones to make space for new notifications.
  removeOutOfBoundsNotifications(notif->height());
  makeSpaceForNotification(notif->height());

  m_activeNotifications.prepend(notif);
}

void ToastNotificationsManager::showNotification(const QList<Message>& new_messages) {
  if (m_articleListNotification == nullptr) {
    m_articleListNotification = new ArticleListNotification();
    hookNotification(m_articleListNotification);
  }

  if (!m_activeNotifications.isEmpty() && m_activeNotifications.first() != m_articleListNotification) {
    // Article notification is somewhere in list, clear first to move it to first positon.
    closeNotification(m_articleListNotification, false);
  }

  auto* screen = moveToProperScreen(m_articleListNotification);

  // Insert new notification into free space.
  m_articleListNotification->show();

  auto notif_new_pos = cornerForNewNotification(screen->availableGeometry());

  // Make sure notification is finally resized.
  m_articleListNotification->adjustSize();
  qApp->processEvents();

  // Move notification, at this point we already need to know its precise size.
  moveNotificationToCorner(m_articleListNotification, notif_new_pos);

  // Remove out-of-bounds old notifications and shift existing
  // ones to make space for new notifications.
  removeOutOfBoundsNotifications(m_articleListNotification->height());
  makeSpaceForNotification(m_articleListNotification->height());

  m_activeNotifications.prepend(m_articleListNotification);
}

void ToastNotificationsManager::closeNotification(BaseToastNotification* notif, bool delete_from_memory) {
  auto notif_idx = m_activeNotifications.indexOf(notif);

  if (delete_from_memory) {
    notif->deleteLater();
  }
  else {
    notif->hide();
  }

  m_activeNotifications.removeAll(notif);

  // Shift all notifications.
  if (notif_idx < 0) {
    return;
  }

  makeSpaceForNotification(notif->height(), true, notif_idx);
}

QScreen* ToastNotificationsManager::activeScreen() const {
  if (m_screen >= 0) {
    auto all_screens = QGuiApplication::screens();

    if (m_screen < all_screens.size()) {
      return all_screens.at(m_screen);
    }
  }

  return QGuiApplication::primaryScreen();
}

QPoint ToastNotificationsManager::cornerForNewNotification(QRect screen_rect) {
  switch (m_position) {
    case ToastNotificationsManager::TopLeft:
      return screen_rect.topLeft() + QPoint(NOTIFICATIONS_MARGIN, NOTIFICATIONS_MARGIN);

    case ToastNotificationsManager::TopRight:
      return screen_rect.topRight() - QPoint(NOTIFICATIONS_MARGIN, -NOTIFICATIONS_MARGIN);

    case ToastNotificationsManager::BottomLeft:
      return screen_rect.bottomLeft() - QPoint(-NOTIFICATIONS_MARGIN, NOTIFICATIONS_MARGIN);

    case ToastNotificationsManager::BottomRight:
    default:
      return screen_rect.bottomRight() - QPoint(NOTIFICATIONS_MARGIN, NOTIFICATIONS_MARGIN);
  }
}

void ToastNotificationsManager::hookNotification(BaseToastNotification* notif) {
  connect(notif, &BaseToastNotification::closeRequested, this, [this](BaseToastNotification* notif) {
    closeNotification(notif, false);
  });
}

void ToastNotificationsManager::moveNotificationToCorner(BaseToastNotification* notif, QPoint corner) {
  switch (m_position) {
    case ToastNotificationsManager::TopLeft:
      notif->move(corner);
      break;

    case ToastNotificationsManager::TopRight:
      notif->move(corner.x() - notif->frameGeometry().width(), corner.y());
      break;

    case ToastNotificationsManager::BottomLeft:
      notif->move(corner.x(), corner.y() - notif->frameGeometry().height());
      break;

    case ToastNotificationsManager::BottomRight:
      notif->move(corner.x() - notif->frameGeometry().width(), corner.y() - notif->frameGeometry().height());
      break;
  }
}

void ToastNotificationsManager::makeSpaceForNotification(int height_to_make_space, bool reverse, int stard_idx) {
  for (int i = stard_idx; i < m_activeNotifications.size(); i++) {
    BaseToastNotification* notif = m_activeNotifications.at(i);

    switch (m_position) {
      case ToastNotificationsManager::TopLeft:
      case ToastNotificationsManager::TopRight: {
        std::function<int(int, int)> shift_down;

        if (reverse) {
          shift_down = [](int x, int y) {
            return x - y;
          };
        }
        else {
          shift_down = [](int x, int y) {
            return x + y;
          };
        }

        // Move it all down.
        notif->move(notif->pos().x(), shift_down(notif->pos().y(), (height_to_make_space + NOTIFICATIONS_MARGIN)));
        break;
      }

      case ToastNotificationsManager::BottomLeft:
      case ToastNotificationsManager::BottomRight: {
        std::function<int(int, int)> shift_up;

        if (reverse) {
          shift_up = [](int x, int y) {
            return x + y;
          };
        }
        else {
          shift_up = [](int x, int y) {
            return x - y;
          };
        }

        // Move it all up.
        notif->move(notif->pos().x(), shift_up(notif->pos().y(), height_to_make_space + NOTIFICATIONS_MARGIN));
        break;
      }
    }
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
