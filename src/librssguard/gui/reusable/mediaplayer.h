// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "gui/tabcontent.h"

#include "ui_mediaplayer.h"

#include <QMediaPlayer>

class QAudioOutput;

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

    // NOTE: Volume is from 0 to 100.
    void setVolume(int volume);

    // NOTE: Media is seekable in miliseconds, but that is too muc
    // for "int" data type, therefore we seek by second.
    void seek(int position);

    void onDurationChanged(qint64 duration);
    void onErrorOccurred(QMediaPlayer::Error error, const QString& error_string);
    void onAudioAvailable(bool available);
    void onVideoAvailable(bool available);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onPositionChanged(qint64 position);
    void onSeekableChanged(bool seekable);

  signals:
    void urlDownloadRequested(const QUrl& url);

  private:
    QString mediaStatusToString(QMediaPlayer::MediaStatus status) const;

    void setupIcons();
    void createConnections();

  private:
    Ui::MediaPlayer m_ui;

#if QT_VERSION_MAJOR == 6
    QAudioOutput* m_audio;
#endif

    QMediaPlayer* m_player;
    QIcon m_iconPlay;
    QIcon m_iconPause;
    QIcon m_iconMute;
    QIcon m_iconUnmute;
    bool m_muted;
};

#endif // MEDIAPLAYER_H
