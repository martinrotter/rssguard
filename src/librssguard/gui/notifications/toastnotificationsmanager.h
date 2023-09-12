// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOASTNOTIFICATIONSMANAGER_H
#define TOASTNOTIFICATIONSMANAGER_H

#include <QObject>

#include "miscellaneous/application.h"

class BaseToastNotification;
class ToastNotification;

class ToastNotificationsManager : public QObject {
    Q_OBJECT

  public:
    enum NotificationPosition {
      TopLeft = 0,
      TopRight = 1,
      BottomLeft = 2,
      BottomRight = 3
    };

    explicit ToastNotificationsManager(QObject* parent = nullptr);
    virtual ~ToastNotificationsManager();

    QList<BaseToastNotification*> activeNotifications() const;

    // Screen ID, setting this to -1 means using default/primary
    // monitor.
    int screen() const;
    void setScreen(int screen);

    NotificationPosition position() const;
    void setPosition(NotificationPosition position);

  public slots:
    void clear();
    void showNotification(Notification::Event event, const GuiMessage& msg, const GuiAction& action);
    void showNotification(const QList<Message>& new_messages);

  private:
    NotificationPosition m_position;
    int m_screen;
    QList<BaseToastNotification*> m_activeNotifications;
};

#endif // TOASTNOTIFICATIONSMANAGER_H
