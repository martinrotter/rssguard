// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/toastnotificationsmanager.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/notifications/articlelistnotification.h"
#include "gui/notifications/basetoastnotification.h"
#include "gui/notifications/toastnotification.h"
#include "miscellaneous/settings.h"

#include <QRect>
#include <QScreen>
#include <QWindow>

QString ToastNotificationsManager::textForPosition(ToastNotificationsManager::NotificationPosition pos) {
  switch (pos) {
    case TopLeft:
      return QObject::tr("top-left");

    case TopRight:
      return QObject::tr("top-right");

    case BottomLeft:
      return QObject::tr("bottom-left");

    case BottomRight:
    default:
      return QObject::tr("bottom-right");
  }
}

ToastNotificationsManager::ToastNotificationsManager(QObject* parent)
  : QObject(parent), m_position(ToastNotificationsManager::NotificationPosition::TopRight), m_screen(0), m_margins(0),
    m_width(0), m_opacity(0.0), m_articleListNotification(nullptr) {
  resetNotifications(false);
}

ToastNotificationsManager::~ToastNotificationsManager() {
  clear(true);
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

void ToastNotificationsManager::resetNotifications(bool reload_existing_notifications) {
  m_position = qApp->settings()
                 ->value(GROUP(GUI), SETTING(GUI::ToastNotificationsPosition))
                 .value<ToastNotificationsManager::NotificationPosition>();
  m_screen = qApp->settings()->value(GROUP(GUI), SETTING(GUI::ToastNotificationsScreen)).toInt();
  m_margins = qApp->settings()->value(GROUP(GUI), SETTING(GUI::ToastNotificationsMargin)).toInt();
  m_opacity = qApp->settings()->value(GROUP(GUI), SETTING(GUI::ToastNotificationsOpacity)).toDouble();
  m_width = qApp->settings()->value(GROUP(GUI), SETTING(GUI::ToastNotificationsWidth)).toInt();

  if (reload_existing_notifications) {
    auto notif = m_activeNotifications;

    clear(false);

    while (!notif.isEmpty()) {
      BaseToastNotification* one_notif = notif.takeLast();

      processNotification(one_notif);
    }
  }
}

void ToastNotificationsManager::clear(bool delete_from_memory) {
  for (BaseToastNotification* notif : m_activeNotifications) {
    closeNotification(notif, delete_from_memory);
  }

  m_activeNotifications.clear();
}

void ToastNotificationsManager::processNotification(BaseToastNotification* notif) {
  notif->setWindowOpacity(m_opacity);
  notif->setFixedWidth(m_width);

  notif->show();

  auto* screen = moveToProperScreen(notif);
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

void ToastNotificationsManager::showNotification(Notification::Event event,
                                                 const GuiMessage& msg,
                                                 const GuiAction& action) {
  BaseToastNotification* notif;

  if (!msg.m_feedFetchResults.updatedFeeds().isEmpty()) {
    if (m_articleListNotification == nullptr) {
      initializeArticleListNotification();
    }
    else if (m_activeNotifications.contains(m_articleListNotification)) {
      // Article notification is somewhere in list, clear first to move it to first positon.
      closeNotification(m_articleListNotification, false);
    }

    m_articleListNotification->loadResults(msg.m_feedFetchResults.updatedFeeds());

    notif = m_articleListNotification;
  }
  else {
    notif = new ToastNotification(event, msg, action, qApp->mainFormWidget());
    hookNotification(notif);
  }

  processNotification(notif);
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
      return screen_rect.topLeft() + QPoint(m_margins, m_margins);

    case ToastNotificationsManager::TopRight:
      return screen_rect.topRight() - QPoint(m_margins, -m_margins);

    case ToastNotificationsManager::BottomLeft:
      return screen_rect.bottomLeft() - QPoint(-m_margins, m_margins);

    case ToastNotificationsManager::BottomRight:
    default:
      return screen_rect.bottomRight() - QPoint(m_margins, m_margins);
  }
}

void ToastNotificationsManager::initializeArticleListNotification() {
  m_articleListNotification = new ArticleListNotification();
  hookNotification(m_articleListNotification);

  connect(m_articleListNotification,
          &ArticleListNotification::openingArticleInArticleListRequested,
          this,
          &ToastNotificationsManager::openingArticleInArticleListRequested);

  connect(m_articleListNotification,
          &ArticleListNotification::reloadMessageListRequested,
          this,
          &ToastNotificationsManager::reloadMessageListRequested);
}

void ToastNotificationsManager::hookNotification(BaseToastNotification* notif) {
  connect(notif, &BaseToastNotification::closeRequested, this, [this](BaseToastNotification* notif) {
    closeNotification(notif, notif != m_articleListNotification);
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
        notif->move(notif->pos().x(), shift_down(notif->pos().y(), (height_to_make_space + m_margins)));
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
        notif->move(notif->pos().x(), shift_up(notif->pos().y(), height_to_make_space + m_margins));
        break;
      }
    }
  }
}

void ToastNotificationsManager::removeOutOfBoundsNotifications(int height_to_reserve) {
  auto* screen = activeScreen();

  int available_height = screen->availableSize().height();

  while (boolinq::from(m_activeNotifications).sum([this](BaseToastNotification* notif) {
    return notif->height() + m_margins;
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
