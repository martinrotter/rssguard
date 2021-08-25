// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QString>

class Application;

class Notification {
  public:
    enum class Event {
      // Is here to provide "empty" events - events which should
      // not trigger any notifications.
      NoEvent = 0,

      // Used for many events which happen throught application lifecycle.
      GeneralEvent = 1,

      // New (unread) messages were downloaded for some feed.
      NewUnreadArticlesFetched = 2,

      // RSS Guard started downloading messages for some feed.
      ArticlesFetchingStarted = 3,

      // Login tokens were successfuly refreshed.
      // NOTE: This is primarily used in accounts which use
      // OAuth or similar mechanism.
      LoginDataRefreshed = 4,

      NewAppVersionAvailable = 5,

      LoginFailure = 6
    };

    explicit Notification(Event event = Event::NoEvent, bool balloon = {}, const QString& sound_path = {});

    bool balloonEnabled() const;

    Event event() const;
    void setEvent(const Event& event);

    // Returns full path to audio file which should be played when notification
    // is launched.
    // NOTE: This property supports "%data%" placeholder.
    QString soundPath() const;
    void setSoundPath(const QString& sound_path);

    void playSound(Application* app) const;

    static QList<Event> allEvents();
    static QString nameForEvent(Event event);

  private:
    Event m_event;
    bool m_balloonEnabled;
    QString m_soundPath;
};

#endif // NOTIFICATION_H
