// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef QTMULTIMEDIABACKEND_H
#define QTMULTIMEDIABACKEND_H

#include "gui/mediaplayer/playerbackend.h"

#include <QObject>

#include <QMediaPlayer>

#if QT_VERSION_MAJOR == 6
#define PLAYBACK_STATE        PlaybackState
#define PLAYBACK_STATE_METHOD playbackState
#else
#define PLAYBACK_STATE        State
#define PLAYBACK_STATE_METHOD state
#endif

#if QT_VERSION_MAJOR == 6
class QAudioOutput;
#endif

class QVideoWidget;

class QtMultimediaBackend : public PlayerBackend {
    Q_OBJECT

  public:
    explicit QtMultimediaBackend(QWidget* parent = nullptr);

    virtual QUrl url() const;
    virtual int position() const;
    virtual int duration() const;

  public slots:
    virtual void playUrl(const QUrl& url);
    virtual void playPause();
    virtual void pause();
    virtual void stop();

    virtual void setPlaybackSpeed(int speed);
    virtual void setVolume(int volume);
    virtual void setPosition(int position);
    virtual void setFullscreen(bool fullscreen);
    virtual void setMuted(bool muted);

  private slots:
    void onPlaybackRateChanged(qreal speed);
    void onDurationChanged(qint64 duration);
    void onErrorOccurred(QMediaPlayer::Error error, const QString& error_string = {});
    void onAudioAvailable(bool available);
    void onVideoAvailable(bool available);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PLAYBACK_STATE state);
    void onPositionChanged(qint64 position);
    void onSeekableChanged(bool seekable);

  private:
    int convertToSliderVolume(float volume) const;
    float convertSliderVolume(int slider_volume) const;
    qint64 convertSliderProgress(int slider_progress) const;
    int convertToSliderProgress(qint64 player_progress) const;
    int convertDuration(qint64 duration) const;
    qreal convertSpeed(int speed) const;
    int convertSpinSpeed(qreal speed) const;

    QString errorToString(QMediaPlayer::Error error) const;
    QString mediaStatusToString(QMediaPlayer::MediaStatus status) const;

  private:
#if QT_VERSION_MAJOR == 6
    QAudioOutput* m_audio;
#endif

    QMediaPlayer* m_player;
    QVideoWidget* m_video;
    int m_volume;
};

#endif // QTMULTIMEDIABACKEND_H
