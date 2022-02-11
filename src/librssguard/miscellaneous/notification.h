// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QString>

#include "definitions/definitions.h"

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

      // New RSS Guard version available.
      NewAppVersionAvailable = 5,

      // Some service failed to login.
      LoginFailure = 6,

      // Node.js - package updated.
      NodePackageUpdated = 7,

      // Node.js - package failde to update.
      NodePackageFailedToUpdate = 8
    };

    explicit Notification(Event event = Event::NoEvent, bool balloon = {}, const QString& sound_path = {},
                          int volume = DEFAULT_NOTIFICATION_VOLUME);

    bool balloonEnabled() const;

    Event event() const;
    void setEvent(Event event);

    int volume() const;
    void setVolume(int volume);

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
    int m_volume;
};

#endif // NOTIFICATION_H
