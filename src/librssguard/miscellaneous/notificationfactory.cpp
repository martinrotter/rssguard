// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/notificationfactory.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/settings.h"

NotificationFactory::NotificationFactory(QObject* parent) : QObject(parent) {}

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

void NotificationFactory::load(Settings* settings) {}

void NotificationFactory::save(const QList<Notification> new_notifications, Settings* settings) {}
