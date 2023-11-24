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

#if QT_VERSION_MAJOR == 6
  m_player->setAudioOutput(m_audio);
#endif

  setupIcons();
  createConnections();

  onPlaybackStateChanged(QMediaPlayer::PLAYBACK_STATE::StoppedState);
  onMediaStatusChanged(QMediaPlayer::MediaStatus::NoMedia);
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

#if QT_VERSION_MAJOR == 6
  m_player->setSource(url);
#else
  m_player->setMedia(QUrl(url));
#endif

  m_player->play();
}

void MediaPlayer::playPause() {
  if (m_player->PLAYBACK_STATE_METHOD() != QMediaPlayer::PLAYBACK_STATE::PlayingState) {
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
  emit urlDownloadRequested(
#if QT_VERSION_MAJOR == 6
    m_player->source()
#else
    m_player->media().request().url()
#endif
  );
}

void MediaPlayer::muteUnmute() {
  m_ui.m_slidVolume->setEnabled(m_muted);
  setVolume(m_muted ? m_ui.m_slidVolume->value() : 0);

  m_muted = !m_muted;
}

void MediaPlayer::setSpeed(int speed) {
  m_player->setPlaybackRate(convertSpeed(speed));
}

void MediaPlayer::setVolume(int volume) {
#if QT_VERSION_MAJOR == 6
  m_player->audioOutput()->setVolume(convertSliderVolume(volume));
#else
  m_player->setVolume(volume);
#endif

  m_ui.m_btnVolume->setIcon(volume <= 0 ? m_iconMute : m_iconUnmute);
}

void MediaPlayer::seek(int position) {
  m_player->setPosition(convertSliderProgress(position));
}

void MediaPlayer::onPlaybackRateChanged(qreal speed) {
  m_ui.m_spinSpeed->blockSignals(true);
  m_ui.m_spinSpeed->setValue(convertSpinSpeed(speed));
  m_ui.m_spinSpeed->blockSignals(false);
}

void MediaPlayer::onDurationChanged(qint64 duration) {
  m_ui.m_slidProgress->blockSignals(true);
  m_ui.m_slidProgress->setMaximum(convertDuration(duration));
  m_ui.m_slidProgress->blockSignals(false);

  updateTimeAndProgress(convertToSliderProgress(m_player->position()), convertDuration(duration));
}

void MediaPlayer::onPositionChanged(qint64 position) {
  m_ui.m_slidProgress->blockSignals(true);
  m_ui.m_slidProgress->setValue(convertToSliderProgress(position));
  m_ui.m_slidProgress->blockSignals(false);

  updateTimeAndProgress(convertToSliderProgress(position), convertDuration(m_player->duration()));
}

void MediaPlayer::updateTimeAndProgress(int progress, int total) {
  m_ui.m_lblTime->setText(QSL("%1/%2").arg(QDateTime::fromSecsSinceEpoch(progress).toUTC().toString("hh:mm:ss"),
                                           QDateTime::fromSecsSinceEpoch(total).toUTC().toString("hh:mm:ss")));
}

void MediaPlayer::onErrorOccurred(QMediaPlayer::Error error, const QString& error_string) {
  QString err = error_string.isEmpty() ? errorToString(error) : error_string;
  m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Error, err, err);
}

void MediaPlayer::onAudioAvailable(bool available) {
  m_ui.m_slidVolume->setEnabled(available);
  m_ui.m_btnVolume->setEnabled(available);
}

void MediaPlayer::onVideoAvailable(bool available) {
  Q_UNUSED(available)
}

void MediaPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
  QString st = mediaStatusToString(status);
  m_ui.m_lblStatus->setStatus(status == QMediaPlayer::MediaStatus::InvalidMedia
                                ? WidgetWithStatus::StatusType::Error
                                : WidgetWithStatus::StatusType::Information,
                              st,
                              st);
}

void MediaPlayer::onPlaybackStateChanged(QMediaPlayer::PLAYBACK_STATE state) {
  switch (state) {
    case QMediaPlayer::PLAYBACK_STATE::StoppedState:
      m_ui.m_btnPlayPause->setIcon(m_iconPlay);
      m_ui.m_btnStop->setEnabled(false);
      break;

    case QMediaPlayer::PLAYBACK_STATE::PlayingState:
      m_ui.m_btnPlayPause->setIcon(m_iconPause);
      m_ui.m_btnStop->setEnabled(true);
      break;

    case QMediaPlayer::PLAYBACK_STATE::PausedState:
      m_ui.m_btnPlayPause->setIcon(m_iconPlay);
      m_ui.m_btnStop->setEnabled(true);
      break;
  }
}

int MediaPlayer::convertToSliderProgress(qint64 player_progress) const {
  return player_progress / 1000;
}

int MediaPlayer::convertDuration(qint64 duration) const {
  return duration / 1000;
}

qreal MediaPlayer::convertSpeed(int speed) const {
  return speed / 100.0;
}

int MediaPlayer::convertSpinSpeed(qreal speed) const {
  return speed * 100;
}

void MediaPlayer::onSeekableChanged(bool seekable) {
  m_ui.m_slidProgress->setEnabled(seekable);

  if (!seekable) {
    onPositionChanged(0);
  }
}

QString MediaPlayer::errorToString(QMediaPlayer::Error error) const {
  switch (error) {
    case QMediaPlayer::ResourceError:
      return tr("Cannot load media (missing codecs)");

    case QMediaPlayer::FormatError:
      return tr("Unrecognized format");

    case QMediaPlayer::NetworkError:
      return tr("Network problem");

    case QMediaPlayer::AccessDeniedError:
      return tr("Access denied");

#if QT_VERSION_MAJOR == 5
    case QMediaPlayer::ServiceMissingError:
      return tr("Service is missing");

    case QMediaPlayer::MediaIsPlaylist:
      return tr("This is playlist");
#endif

    case QMediaPlayer::NoError:
      return tr("No errors");

    default:
      return tr("Unknown error");
  }
}

float MediaPlayer::convertSliderVolume(int slider_volume) const {
  return slider_volume / 100.0f;
}

qint64 MediaPlayer::convertSliderProgress(int slider_progress) const {
  return qint64(slider_progress) * qint64(1000);
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

#if QT_VERSION_MAJOR == 6
  connect(m_player, &QMediaPlayer::errorOccurred, this, &MediaPlayer::onErrorOccurred);
#else
  connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, [this](QMediaPlayer::Error error) {
    onErrorOccurred(error);
  });
#endif

#if QT_VERSION_MAJOR == 6
  connect(m_player, &QMediaPlayer::hasAudioChanged, this, &MediaPlayer::onAudioAvailable);
  connect(m_player, &QMediaPlayer::hasVideoChanged, this, &MediaPlayer::onVideoAvailable);
  connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MediaPlayer::onPlaybackStateChanged);
#else
  connect(m_player, &QMediaPlayer::audioAvailableChanged, this, &MediaPlayer::onAudioAvailable);
  connect(m_player, &QMediaPlayer::videoAvailableChanged, this, &MediaPlayer::onVideoAvailable);
  connect(m_player, &QMediaPlayer::stateChanged, this, &MediaPlayer::onPlaybackStateChanged);
#endif

  connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MediaPlayer::onMediaStatusChanged);
  connect(m_player, &QMediaPlayer::positionChanged, this, &MediaPlayer::onPositionChanged);
  connect(m_player, &QMediaPlayer::seekableChanged, this, &MediaPlayer::onSeekableChanged);
  connect(m_player, &QMediaPlayer::playbackRateChanged, this, &MediaPlayer::onPlaybackRateChanged);

  connect(m_ui.m_btnPlayPause, &PlainToolButton::clicked, this, &MediaPlayer::playPause);
  connect(m_ui.m_btnStop, &PlainToolButton::clicked, this, &MediaPlayer::stop);
  connect(m_ui.m_btnDownload, &PlainToolButton::clicked, this, &MediaPlayer::download);
  connect(m_ui.m_btnVolume, &PlainToolButton::clicked, this, &MediaPlayer::muteUnmute);
  connect(m_ui.m_slidVolume, &QSlider::valueChanged, this, &MediaPlayer::setVolume);
  connect(m_ui.m_slidProgress, &QSlider::valueChanged, this, &MediaPlayer::seek);
  connect(m_ui.m_spinSpeed, &QSpinBox::valueChanged, this, &MediaPlayer::setSpeed);
}
