// For license of this file, see <project-root-folder>/LICENSE.md.

#include "mediaplayer.h"

#include "miscellaneous/iconfactory.h"

#if QT_VERSION_MAJOR == 6
#include <QAudioOutput>
#endif

MediaPlayer::MediaPlayer(QWidget* parent)
  : TabContent(parent),
#if QT_VERSION_MAJOR == 6
    m_audio(new QAudioOutput(this)),
#endif
    m_player(new QMediaPlayer(this)), m_muted(false) {
  m_ui.setupUi(this);

  m_player->setVideoOutput(m_ui.m_video);
  m_player->setAudioOutput(m_audio);

  setupIcons();
  createConnections();

  onPlaybackStateChanged(QMediaPlayer::PlaybackState::StoppedState);
  onMediaStatusChanged(QMediaPlayer::MediaStatus::NoMedia);
}

MediaPlayer::~MediaPlayer() {}

WebBrowser* MediaPlayer::webBrowser() const {
  return nullptr;
}

void MediaPlayer::playUrl(const QString& url) {
  setVolume(m_ui.m_slidVolume->value());

  m_player->setSource(url);
  m_player->play();
}

void MediaPlayer::playPause() {
  if (m_player->playbackState() != QMediaPlayer::PlaybackState::PlayingState) {
    m_player->play();
  }
  else {
    m_player->pause();
  }
}

void MediaPlayer::stop() {
  m_player->stop();
}

void MediaPlayer::download() {
  emit urlDownloadRequested(m_player->source());
}

void MediaPlayer::muteUnmute() {
  m_ui.m_slidVolume->setEnabled(m_muted);
  setVolume(m_muted ? m_ui.m_slidVolume->value() : 0);

  m_muted = !m_muted;
}

void MediaPlayer::setVolume(int volume) {
  m_player->audioOutput()->setVolume(volume / 100.0f);
  m_ui.m_btnVolume->setIcon(volume <= 0 ? m_iconMute : m_iconUnmute);
}

void MediaPlayer::seek(int position) {
  m_player->setPosition(position * 1000);
}

void MediaPlayer::onDurationChanged(qint64 duration) {
  m_ui.m_slidProgress->blockSignals(true);
  m_ui.m_slidProgress->setMaximum(duration / 1000);
  m_ui.m_slidProgress->blockSignals(false);
}

void MediaPlayer::onErrorOccurred(QMediaPlayer::Error error, const QString& error_string) {
  m_ui.m_lblStatus->setText(error_string);
}

void MediaPlayer::onAudioAvailable(bool available) {
  m_ui.m_slidVolume->setEnabled(available);
  m_ui.m_btnVolume->setEnabled(available);
}

void MediaPlayer::onVideoAvailable(bool available) {
  Q_UNUSED(available)
}

void MediaPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
  m_ui.m_lblStatus->setText(mediaStatusToString(status));
}

void MediaPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
  switch (state) {
    case QMediaPlayer::StoppedState:
      m_ui.m_btnPlayPause->setIcon(m_iconPlay);
      m_ui.m_btnStop->setEnabled(false);
      break;

    case QMediaPlayer::PlayingState:
      m_ui.m_btnPlayPause->setIcon(m_iconPause);
      m_ui.m_btnStop->setEnabled(true);
      break;

    case QMediaPlayer::PausedState:
      m_ui.m_btnPlayPause->setIcon(m_iconPlay);
      m_ui.m_btnStop->setEnabled(true);
      break;
  }
}

void MediaPlayer::onPositionChanged(qint64 position) {
  m_ui.m_slidProgress->blockSignals(true);
  m_ui.m_slidProgress->setValue(position / 1000);
  m_ui.m_slidProgress->blockSignals(false);
}

void MediaPlayer::onSeekableChanged(bool seekable) {
  m_ui.m_slidProgress->setEnabled(seekable);

  if (!seekable) {
    onPositionChanged(0);
  }
}

QString MediaPlayer::mediaStatusToString(QMediaPlayer::MediaStatus status) const {
  switch (status) {
    case QMediaPlayer::NoMedia:
      return tr("No media");

    case QMediaPlayer::LoadingMedia:
      return tr("Loading...");

    case QMediaPlayer::LoadedMedia:
      return tr("Media loaded");

    case QMediaPlayer::StalledMedia:
      return tr("Media stalled");

    case QMediaPlayer::BufferingMedia:
      return tr("Buffering...");

    case QMediaPlayer::BufferedMedia:
      return tr("Loaded");

    case QMediaPlayer::EndOfMedia:
      return tr("Ended");

    case QMediaPlayer::InvalidMedia:
      return tr("Media is invalid");

    default:
      return tr("Unknown");
  }
}

void MediaPlayer::setupIcons() {
  m_iconPlay = qApp->icons()->fromTheme(QSL("media-playback-start"), QSL("player_play"));
  m_iconPause = qApp->icons()->fromTheme(QSL("media-playback-pause"), QSL("player_pause"));
  m_iconMute = qApp->icons()->fromTheme(QSL("player-volume-muted"), QSL("audio-volume-muted"));
  m_iconUnmute = qApp->icons()->fromTheme(QSL("player-volume"), QSL("stock_volume"));

  m_ui.m_btnDownload->setIcon(qApp->icons()->fromTheme(QSL("download"), QSL("browser-download")));
  m_ui.m_btnStop->setIcon(qApp->icons()->fromTheme(QSL("media-playback-stop"), QSL("player_stop")));
}

void MediaPlayer::createConnections() {
  connect(m_player, &QMediaPlayer::durationChanged, this, &MediaPlayer::onDurationChanged);
  connect(m_player, &QMediaPlayer::errorOccurred, this, &MediaPlayer::onErrorOccurred);
  connect(m_player, &QMediaPlayer::hasAudioChanged, this, &MediaPlayer::onAudioAvailable);
  connect(m_player, &QMediaPlayer::hasVideoChanged, this, &MediaPlayer::onVideoAvailable);
  connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MediaPlayer::onMediaStatusChanged);
  connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MediaPlayer::onPlaybackStateChanged);
  connect(m_player, &QMediaPlayer::positionChanged, this, &MediaPlayer::onPositionChanged);
  connect(m_player, &QMediaPlayer::seekableChanged, this, &MediaPlayer::onSeekableChanged);

  connect(m_ui.m_btnPlayPause, &PlainToolButton::clicked, this, &MediaPlayer::playPause);
  connect(m_ui.m_btnStop, &PlainToolButton::clicked, this, &MediaPlayer::stop);
  connect(m_ui.m_btnDownload, &PlainToolButton::clicked, this, &MediaPlayer::download);
  connect(m_ui.m_btnVolume, &PlainToolButton::clicked, this, &MediaPlayer::muteUnmute);
  connect(m_ui.m_slidVolume, &QSlider::valueChanged, this, &MediaPlayer::setVolume);
  connect(m_ui.m_slidProgress, &QSlider::valueChanged, this, &MediaPlayer::seek);
}