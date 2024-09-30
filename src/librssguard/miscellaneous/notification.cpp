// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/notification.h"

#include "miscellaneous/application.h"

#include <QDir>

#if !defined(Q_OS_OS2)
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QSoundEffect>
#endif

Notification::Notification(Notification::Event event,
                           bool balloon,
                           bool play_sound,
                           const QString& sound_path,
                           int volume)
  : m_event(event), m_balloonEnabled(balloon), m_soundEnabled(play_sound), m_soundPath(sound_path), m_volume(volume) {}

Notification::Event Notification::event() const {
  return m_event;
}

void Notification::setEvent(Event event) {
  m_event = event;
}

QString Notification::soundPath() const {
  return m_soundPath;
}

void Notification::setSoundPath(const QString& sound_path) {
  m_soundPath = sound_path;
}

void Notification::playSound(Application* app) const {
  if (!m_soundPath.isEmpty()) {
#if !defined(Q_OS_OS2)
    if (m_soundPath.endsWith(QSL(".wav"), Qt::CaseSensitivity::CaseInsensitive)) {
      qDebugNN << LOGSEC_CORE << "Using QSoundEffect to play notification sound.";

      QSoundEffect* play = new QSoundEffect(app);

      QObject::connect(play, &QSoundEffect::playingChanged, play, [play]() {
        if (!play->isPlaying()) {
          play->deleteLater();
        }
      });

      if (m_soundPath.startsWith(QSL(":"))) {
        play->setSource(QUrl(QSL("qrc") + m_soundPath));
      }
      else {
        play
          ->setSource(QUrl::fromLocalFile(QDir::toNativeSeparators(app
                                                                     ->replaceUserDataFolderPlaceholder(m_soundPath))));
      }

      play->setVolume(fractionalVolume());
      play->play();
    }
    else {
      qDebugNN << LOGSEC_CORE << "Using QMediaPlayer to play notification sound.";

      QMediaPlayer* play = new QMediaPlayer(app);

      QAudioOutput* out = new QAudioOutput(app);

      play->setAudioOutput(out);

      QObject::connect(play, &QMediaPlayer::playbackStateChanged, play, [play, out](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlaybackState::StoppedState) {
          out->deleteLater();
          play->deleteLater();
        }
      });

      if (m_soundPath.startsWith(QSL(":"))) {
        play->setSource(QUrl(QSL("qrc") + m_soundPath));
      }
      else {
        play
          ->setSource(QUrl::fromLocalFile(QDir::toNativeSeparators(app
                                                                     ->replaceUserDataFolderPlaceholder(m_soundPath))));
      }

      play->audioOutput()->setVolume(fractionalVolume());
      play->play();
    }
#endif
  }
}

QList<Notification::Event> Notification::allEvents() {
  return {Event::GeneralEvent,
          Event::NewUnreadArticlesFetched,
          Event::ArticlesFetchingStarted,
          Event::LoginDataRefreshed,
          Event::LoginFailure,
          Event::NewAppVersionAvailable,
          Event::NodePackageUpdated,
          Event::NodePackageFailedToUpdate};
}

QString Notification::nameForEvent(Notification::Event event) {
  switch (event) {
    case Notification::Event::NewUnreadArticlesFetched:
      return QObject::tr("New (unread) articles fetched");

    case Notification::Event::ArticlesFetchingStarted:
      return QObject::tr("Fetching articles right now");

    case Notification::Event::LoginDataRefreshed:
      return QObject::tr("Login data refreshed");

    case Notification::Event::LoginFailure:
      return QObject::tr("Login failed");

    case Notification::Event::NewAppVersionAvailable:
      return QObject::tr("New %1 version is available").arg(QSL(APP_NAME));

    case Notification::Event::GeneralEvent:
      return QObject::tr("Miscellaneous events");

    case Notification::Event::NodePackageUpdated:
      return QObject::tr("Node.js - package(s) updated");

    case Notification::Event::NodePackageFailedToUpdate:
      return QObject::tr("Node.js - package(s) failed to update");

    default:
      return QObject::tr("Unknown event");
  }
}

void Notification::setSoundEnabled(bool play_sound) {
  m_soundEnabled = play_sound;
}

int Notification::volume() const {
  return m_volume;
}

qreal Notification::fractionalVolume() const {
  return (m_volume * 1.0f) / 100.0f;
}

void Notification::setVolume(int volume) {
  m_volume = volume;
}

bool Notification::soundEnabled() const {
  return m_soundEnabled;
}

bool Notification::balloonEnabled() const {
  return m_balloonEnabled;
}
