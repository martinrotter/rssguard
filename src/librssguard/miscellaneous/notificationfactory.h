// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NOTIFICATIONFACTORY_H
#define NOTIFICATIONFACTORY_H

#include <QObject>

#include "miscellaneous/notification.h"

class Settings;

class NotificationFactory : public QObject {
    Q_OBJECT

  public:
    explicit NotificationFactory(QObject* parent = nullptr);

    QList<Notification> allNotifications() const;
    Notification notificationForEvent(Notification::Event event) const;

    // Determines whether balloon tips are enabled or not.
    bool areNotificationsEnabled() const;
    bool useToastNotifications() const;

  public slots:
    void load(Settings* settings);
    void save(const QList<Notification>& new_notifications, Settings* settings);

  private:
    QList<Notification> m_notifications = {};
};

#endif // NOTIFICATIONFACTORY_H
