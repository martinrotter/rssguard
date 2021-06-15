// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QString>

class Application;

class Notification {
  public:
    enum class Event {
      UnknownEvent = 0,

      // New (unread) messages were downloaded for some feed.
      NewArticlesFetched = 1,

      // RSS Guard started downloading messages for some feed.
      ArticlesFetchingStarted = 2,

      // Login tokens were successfuly refreshed.
      // NOTE: This is primarily used in accounts which use
      // OAuth or similar mechanism.
      LoginDataRefreshed = 4,

      // TODO: app update is available
    };

    explicit Notification();
    explicit Notification(Event event, const QString& sound_path);

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
    QString m_soundPath;
};

#endif // NOTIFICATION_H
