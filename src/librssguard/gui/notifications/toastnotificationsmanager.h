// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOASTNOTIFICATIONSMANAGER_H
#define TOASTNOTIFICATIONSMANAGER_H

#include "miscellaneous/application.h"

#include <QObject>

class BaseToastNotification;
class ToastNotification;
class ArticleListNotification;
class QScreen;

class ToastNotificationsManager : public QObject {
    Q_OBJECT

  public:
    enum NotificationPosition {
      TopLeft = 0,
      TopRight = 1,
      BottomLeft = 2,
      BottomRight = 3
    };

    Q_ENUM(NotificationPosition)

    static QString textForPosition(ToastNotificationsManager::NotificationPosition pos);

    explicit ToastNotificationsManager(QObject* parent = nullptr);
    virtual ~ToastNotificationsManager();

    QList<BaseToastNotification*> activeNotifications() const;

    // Screen ID, setting this to -1 means using default/primary
    // monitor.
    int screen() const;
    void setScreen(int screen);

    NotificationPosition position() const;
    void setPosition(NotificationPosition position);

    void resetNotifications(bool reload_existing_notifications);

  public slots:
    void clear(bool delete_from_memory);
    void showNotification(Notification::Event event, const GuiMessage& msg, const GuiAction& action = {});

  private slots:
    void closeNotification(BaseToastNotification* notif, bool delete_from_memory);

  signals:
    void openingArticleInArticleListRequested(Feed* feed, const Message& msg);
    void reloadMessageListRequested(bool mark_selected_messages_read);

  private:
    QScreen* activeScreen() const;
    QScreen* moveToProperScreen(BaseToastNotification* notif);
    QPoint cornerForNewNotification(QRect screen_rect);

    void processNotification(BaseToastNotification* notif);
    void initializeArticleListNotification();
    void hookNotification(BaseToastNotification* notif);
    void moveNotificationToCorner(BaseToastNotification* notif, QPoint corner);
    void makeSpaceForNotification(int height_to_make_space, bool reverse = false, int stard_idx = 0);
    void removeOutOfBoundsNotifications(int height_to_reserve);

  private:
    NotificationPosition m_position;
    int m_screen;
    int m_margins;
    int m_width;
    double m_opacity;

    // List of all displayed notifications, newest notifications are in the beginning of the list
    // and oldest at the end.
    QList<BaseToastNotification*> m_activeNotifications;

    ArticleListNotification* m_articleListNotification;
};

#endif // TOASTNOTIFICATIONSMANAGER_H
