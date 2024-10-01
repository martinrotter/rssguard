// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/notificationfactory.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QRegularExpression>

NotificationFactory::NotificationFactory(QObject* parent) : QObject(parent) {}

QList<Notification> NotificationFactory::allNotifications() const {
  return m_notifications;
}

bool NotificationFactory::areNotificationsEnabled() const {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::EnableNotifications)).toBool();
}

bool NotificationFactory::useToastNotifications() const {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::UseToastNotifications)).toBool();
}

Notification NotificationFactory::notificationForEvent(Notification::Event event) const {
  if (!qApp->settings()->value(GROUP(GUI), SETTING(GUI::EnableNotifications)).toBool()) {
    return Notification();
  }

  auto good_n = boolinq::from(m_notifications).where([event](const Notification& n) {
    return n.event() == event;
  });

  if (good_n.count() <= 0) {
    qDebugNN << LOGSEC_CORE << "Notification for event" << QUOTE_W_SPACE(int(event)) << "not found";

    return Notification();
  }
  else {
    return good_n.first();
  }
}

void NotificationFactory::load(Settings* settings) {
  auto notif_keys = settings->allKeys(GROUP(Notifications)).filter(QRegularExpression(QSL("^\\d+$")));

  m_notifications.clear();

  for (const auto& key : notif_keys) {
    auto event = Notification::Event(key.toInt());
    auto data = settings->value(GROUP(Notifications), key).toStringList();
    auto enabled = data.at(0).toInt() != 0;
    auto sound_path = data.at(1);
    auto volume = data.size() > 2 ? data.at(2).toInt() : DEFAULT_NOTIFICATION_VOLUME;
    auto play_sound = data.size() > 3 ? data.at(3).toInt() != 0 : true;

    m_notifications.append(Notification(event, enabled, play_sound, sound_path, volume));
  }
}

void NotificationFactory::save(const QList<Notification>& new_notifications, Settings* settings) {
  settings->remove(GROUP(Notifications));
  m_notifications = new_notifications;

  for (const auto& n : std::as_const(m_notifications)) {
    settings->setValue(GROUP(Notifications),
                       QString::number(int(n.event())),
                       QStringList{n.balloonEnabled() ? QSL("1") : QSL("0"),
                                   n.soundPath(),
                                   QString::number(n.volume()),
                                   n.soundEnabled() ? QSL("1") : QSL("0")});
  }
}
