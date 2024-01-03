// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "gui/tabcontent.h"

#include "gui/mediaplayer/playerbackend.h"

#include "ui_mediaplayer.h"

class MediaPlayer : public TabContent {
    Q_OBJECT

  public:
    explicit MediaPlayer(QWidget* parent = nullptr);
    virtual ~MediaPlayer();

    virtual WebBrowser* webBrowser() const;

  public slots:
    void playUrl(const QString& url);

  private slots:
    void playPause();
    void stop();
    void download();
    void muteUnmute();

    // NOTE: 100 means standard speed, above that value means faster, below means slower.
    void setSpeed(int speed);

    // NOTE: Volume is from 0 to 100 taken directly from slider or
    // elsewhere.
    void setVolume(int volume);

    // NOTE: We seek by second.
    void seek(int position);

    void showPlayerNormal();
    void showPlayerFullscreen();
    void escapeFromFullscreen();
    void switchFullScreen(bool send_event_to_backend);

    void hideControls();
    void showControls();

    void onFullscreenChanged(bool fullscreen);
    void onMutedChanged(bool muted);
    void onSpeedChanged(int speed);
    void onDurationChanged(int duration);
    void onPositionChanged(int position);
    void onVolumeChanged(int volume);
    void onErrorOccurred(const QString& error_string);
    void onStatusChanged(const QString& status);
    void onPlaybackStateChanged(PlayerBackend::PlaybackState state);
    void onAudioAvailable(bool available);
    void onVideoAvailable(bool available);
    void onSeekableChanged(bool seekable);

  signals:
    void urlDownloadRequested(const QUrl& url);
    void closed();

  private:
    bool isFullScreen() const;

    void updateTimeAndProgress(int progress, int total);
    void setupIcons();

    void createBackendConnections();
    void createConnections();

  private:
    Ui::MediaPlayer m_ui;

    PlayerBackend* m_backend;
    QIcon m_iconPlay;
    QIcon m_iconPause;
    QIcon m_iconMute;
    QIcon m_iconUnmute;
    bool m_muted;
};

#endif // MEDIAPLAYER_H
