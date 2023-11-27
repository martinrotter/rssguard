// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/mediaplayer.h"

#include "miscellaneous/iconfactory.h"

#if defined(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
#include "gui/mediaplayer/qtmultimedia/qtmultimediabackend.h"
#elif defined(ENABLE_MEDIAPLAYER_LIBMPV)
#include "gui/mediaplayer/libmpv/libmpvbackend.h"
#endif

MediaPlayer::MediaPlayer(QWidget* parent)
  : TabContent(parent), m_backend(
#if defined(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
                          new QtMultimediaBackend(this)
#else
                          new LibMpvBackend(this)
#endif
                            ),
    m_muted(false) {
  m_ui.setupUi(this);

  m_ui.m_layoutMain->insertWidget(0, m_backend, 1);

  setupIcons();
  createBackendConnections();
  createConnections();
}

MediaPlayer::~MediaPlayer() {}

WebBrowser* MediaPlayer::webBrowser() const {
  return nullptr;
}

void MediaPlayer::playUrl(const QString& url) {
  if (m_muted) {
    muteUnmute();
  }
  else {
    setVolume(m_ui.m_slidVolume->value());
  }

  m_backend->playUrl(url);
}

void MediaPlayer::playPause() {
  m_backend->playPause();
}

void MediaPlayer::stop() {
  m_backend->stop();
}

void MediaPlayer::download() {
  emit urlDownloadRequested(m_backend->url());
}

void MediaPlayer::muteUnmute() {
  m_ui.m_slidVolume->setEnabled(m_muted);
  setVolume(m_muted ? m_ui.m_slidVolume->value() : 0);

  m_muted = !m_muted;
}

void MediaPlayer::setSpeed(int speed) {
  m_backend->setPlaybackSpeed(speed);
}

void MediaPlayer::setVolume(int volume) {
  m_backend->setVolume(volume);

  m_ui.m_btnVolume->setIcon(volume <= 0 ? m_iconMute : m_iconUnmute);
}

void MediaPlayer::seek(int position) {
  m_backend->setPosition(position);
}

void MediaPlayer::onPositionChanged(int position) {
  m_ui.m_slidProgress->blockSignals(true);
  m_ui.m_slidProgress->setValue(position);
  m_ui.m_slidProgress->blockSignals(false);

  updateTimeAndProgress(position, m_backend->duration());
}

void MediaPlayer::onSpeedChanged(int speed) {
  m_ui.m_spinSpeed->blockSignals(true);
  m_ui.m_spinSpeed->setValue(speed);
  m_ui.m_spinSpeed->blockSignals(false);
}

void MediaPlayer::onDurationChanged(int duration) {
  m_ui.m_slidProgress->blockSignals(true);
  m_ui.m_slidProgress->setMaximum(duration);
  m_ui.m_slidProgress->blockSignals(false);

  updateTimeAndProgress(m_backend->position(), duration);
}

void MediaPlayer::updateTimeAndProgress(int progress, int total) {
  m_ui.m_lblTime->setText(QSL("%1/%2").arg(QDateTime::fromSecsSinceEpoch(progress).toUTC().toString("hh:mm:ss"),
                                           QDateTime::fromSecsSinceEpoch(total).toUTC().toString("hh:mm:ss")));
}

void MediaPlayer::onErrorOccurred(const QString& error_string) {
  m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Error, error_string, error_string);
}

void MediaPlayer::onAudioAvailable(bool available) {
  m_ui.m_slidVolume->setEnabled(available);
  m_ui.m_btnVolume->setEnabled(available);
}

void MediaPlayer::onVideoAvailable(bool available) {
  Q_UNUSED(available)
}

void MediaPlayer::onStatusChanged(const QString& status) {
  m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Information, status, status);
}

void MediaPlayer::onPlaybackStateChanged(PlayerBackend::PlaybackState state) {
  switch (state) {
    case PlayerBackend::PlaybackState::StoppedState:
      m_ui.m_btnPlayPause->setIcon(m_iconPlay);
      m_ui.m_btnStop->setEnabled(false);
      break;

    case PlayerBackend::PlaybackState::PlayingState:
      m_ui.m_btnPlayPause->setIcon(m_iconPause);
      m_ui.m_btnStop->setEnabled(true);
      break;

    case PlayerBackend::PlaybackState::PausedState:
      m_ui.m_btnPlayPause->setIcon(m_iconPlay);
      m_ui.m_btnStop->setEnabled(true);
      break;
  }
}

void MediaPlayer::onSeekableChanged(bool seekable) {
  m_ui.m_slidProgress->setEnabled(seekable);

  if (!seekable) {
    onPositionChanged(0);
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

void MediaPlayer::createBackendConnections() {
  installEventFilter(m_backend);

  connect(m_backend, &PlayerBackend::speedChanged, this, &MediaPlayer::onSpeedChanged);
  connect(m_backend, &PlayerBackend::durationChanged, this, &MediaPlayer::onDurationChanged);
  connect(m_backend, &PlayerBackend::positionChanged, this, &MediaPlayer::onPositionChanged);
  connect(m_backend, &PlayerBackend::errorOccurred, this, &MediaPlayer::onErrorOccurred);
  connect(m_backend, &PlayerBackend::playbackStateChanged, this, &MediaPlayer::onPlaybackStateChanged);
  connect(m_backend, &PlayerBackend::statusChanged, this, &MediaPlayer::onStatusChanged);
  connect(m_backend, &PlayerBackend::audioAvailable, this, &MediaPlayer::onAudioAvailable);
  connect(m_backend, &PlayerBackend::videoAvailable, this, &MediaPlayer::onVideoAvailable);
  connect(m_backend, &PlayerBackend::seekableChanged, this, &MediaPlayer::onSeekableChanged);
}

void MediaPlayer::createConnections() {
  connect(m_ui.m_btnPlayPause, &PlainToolButton::clicked, this, &MediaPlayer::playPause);
  connect(m_ui.m_btnStop, &PlainToolButton::clicked, this, &MediaPlayer::stop);
  connect(m_ui.m_btnDownload, &PlainToolButton::clicked, this, &MediaPlayer::download);
  connect(m_ui.m_btnVolume, &PlainToolButton::clicked, this, &MediaPlayer::muteUnmute);
  connect(m_ui.m_slidVolume, &QSlider::valueChanged, this, &MediaPlayer::setVolume);
  connect(m_ui.m_slidProgress, &QSlider::valueChanged, this, &MediaPlayer::seek);
  connect(m_ui.m_spinSpeed, QOverload<int>::of(&QSpinBox::valueChanged), this, &MediaPlayer::setSpeed);
}
