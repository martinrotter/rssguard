// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/notificationfactory.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/settings.h"

#include <QRegularExpression>

NotificationFactory::NotificationFactory(QObject* parent) : QObject(parent) {}

QList<Notification> NotificationFactory::allNotifications() const {
  return m_notifications;
}

Notification NotificationFactory::notificationForEvent(Notification::Event event) const {
  auto good_n = boolinq::from(m_notifications).where([event](const Notification& n) {
    return n.event() == event;
  });

  if (good_n.count() <= 0) {
    throw ApplicationException(QSL("notification for event %1 was not found").arg(QString::number(int(event))));
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
    auto sound = settings->value(GROUP(Notifications), key).toString();

    m_notifications.append(Notification(event, sound));
  }
}

void NotificationFactory::save(const QList<Notification>& new_notifications, Settings* settings) {
  settings->remove(GROUP(Notifications));
  m_notifications = new_notifications;

  for (const auto& n : qAsConst(m_notifications)) {
    settings->setValue(GROUP(Notifications), QString::number(int(n.event())), n.soundPath());
  }
}
