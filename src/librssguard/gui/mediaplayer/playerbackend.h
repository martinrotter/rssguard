// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef PLAYERBACKEND_H
#define PLAYERBACKEND_H

#include <QWidget>

#include <QUrl>

class QVBoxLayout;
class Application;

class PlayerBackend : public QWidget {
    Q_OBJECT

  public:
    enum class PlaybackState {
      StoppedState,
      PlayingState,
      PausedState
    };

    explicit PlayerBackend(Application* app, QWidget* parent = nullptr);

    virtual QUrl url() const = 0;
    virtual int position() const = 0;
    virtual int duration() const = 0;

  signals:
    void closed();
    void fullscreenChanged(bool fullscreen);
    void mutedChanged(bool muted);
    void speedChanged(int speed);
    void durationChanged(int duration);
    void positionChanged(int position);
    void volumeChanged(int volume);
    void audioAvailable(bool available);
    void videoAvailable(bool available);
    void seekableChanged(bool seekable);

    void errorOccurred(const QString& error_string);
    void statusChanged(const QString& status);
    void playbackStateChanged(PlayerBackend::PlaybackState state);

  public slots:
    virtual void playUrl(const QUrl& url) = 0;
    virtual void playPause() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    virtual void setFullscreen(bool fullscreen) = 0;
    virtual void setMuted(bool muted) = 0;
    virtual void setPlaybackSpeed(int speed) = 0;
    virtual void setVolume(int volume) = 0;
    virtual void setPosition(int position) = 0;

  protected:
    Application* m_app;

  private:
    QVBoxLayout* m_mainLayout;
};

#endif // PLAYERBACKEND_H
