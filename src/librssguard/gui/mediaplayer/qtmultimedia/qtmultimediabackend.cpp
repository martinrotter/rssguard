// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/qtmultimedia/qtmultimediabackend.h"

#include <QAudioOutput>
#include <QLayout>
#include <QVideoWidget>
#include <QWindow>

QtMultimediaBackend::QtMultimediaBackend(Application* app, QWidget* parent)
  : PlayerBackend(app, parent),
    m_audio(new QAudioOutput(this)),
    m_player(new QMediaPlayer(this)),

    m_video(new QVideoWidget(this)) {
  layout()->addWidget(m_video);

  m_player->setVideoOutput(m_video);

  m_player->setAudioOutput(m_audio);

  connect(m_player, &QMediaPlayer::durationChanged, this, &QtMultimediaBackend::onDurationChanged);
  connect(m_player, &QMediaPlayer::errorOccurred, this, &QtMultimediaBackend::onErrorOccurred);
  connect(m_player, &QMediaPlayer::hasAudioChanged, this, &QtMultimediaBackend::onAudioAvailable);
  connect(m_player, &QMediaPlayer::hasVideoChanged, this, &QtMultimediaBackend::onVideoAvailable);
  connect(m_player, &QMediaPlayer::playbackStateChanged, this, &QtMultimediaBackend::onPlaybackStateChanged);
  connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &QtMultimediaBackend::onMediaStatusChanged);
  connect(m_player, &QMediaPlayer::positionChanged, this, &QtMultimediaBackend::onPositionChanged);
  connect(m_player, &QMediaPlayer::seekableChanged, this, &QtMultimediaBackend::onSeekableChanged);
  connect(m_player, &QMediaPlayer::playbackRateChanged, this, &QtMultimediaBackend::onPlaybackRateChanged);
}

int QtMultimediaBackend::convertToSliderProgress(qint64 player_progress) const {
  return player_progress / 1000;
}

int QtMultimediaBackend::convertDuration(qint64 duration) const {
  return duration / 1000;
}

qreal QtMultimediaBackend::convertSpeed(int speed) const {
  return speed / 100.0;
}

int QtMultimediaBackend::convertSpinSpeed(qreal speed) const {
  return speed * 100;
}

float QtMultimediaBackend::convertSliderVolume(int slider_volume) const {
  return slider_volume / 100.0f;
}

qint64 QtMultimediaBackend::convertSliderProgress(int slider_progress) const {
  return qint64(slider_progress) * qint64(1000);
}

QString QtMultimediaBackend::mediaStatusToString(QMediaPlayer::MediaStatus status) const {
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

QString QtMultimediaBackend::errorToString(QMediaPlayer::Error error) const {
  switch (error) {
    case QMediaPlayer::ResourceError:
      return tr("Cannot load media (missing codecs)");

    case QMediaPlayer::FormatError:
      return tr("Unrecognized format");

    case QMediaPlayer::NetworkError:
      return tr("Network problem");

    case QMediaPlayer::AccessDeniedError:
      return tr("Access denied");

    case QMediaPlayer::NoError:
      return tr("No errors");

    default:
      return tr("Unknown error");
  }
}

void QtMultimediaBackend::playUrl(const QUrl& url) {
  m_player->setSource(url);

  m_player->play();
}

void QtMultimediaBackend::playPause() {
  if (m_player->playbackState() != QMediaPlayer::PlaybackState::PlayingState) {
    m_player->play();
  }
  else {
    m_player->pause();
  }
}

void QtMultimediaBackend::pause() {
  m_player->pause();
}

void QtMultimediaBackend::stop() {
  m_player->stop();
}

void QtMultimediaBackend::setPlaybackSpeed(int speed) {
  m_player->setPlaybackRate(convertSpeed(speed));
}

void QtMultimediaBackend::setVolume(int volume) {
  m_player->audioOutput()->setVolume(convertSliderVolume(volume));

  emit volumeChanged(volume);
}

void QtMultimediaBackend::setPosition(int position) {
  m_player->setPosition(convertSliderProgress(position));
}

void QtMultimediaBackend::setFullscreen(bool fullscreen) {
  Q_UNUSED(fullscreen)
  // No extra work needed here.
}

void QtMultimediaBackend::setMuted(bool muted) {
  if (muted) {
    // Remember volume and mute.
    m_volume = convertToSliderVolume(m_player->audioOutput()->volume());

    m_player->audioOutput()->setVolume(convertSliderVolume(0));
  }
  else {
    // Unmute.
    m_player->audioOutput()->setVolume(convertSliderVolume(m_volume));
  }

  emit mutedChanged(muted);
}

QUrl QtMultimediaBackend::url() const {
  return m_player->source();
}

int QtMultimediaBackend::position() const {
  return convertToSliderProgress(m_player->position());
}

int QtMultimediaBackend::duration() const {
  return convertDuration(m_player->duration());
}

void QtMultimediaBackend::onPositionChanged(qint64 position) {
  emit positionChanged(convertToSliderProgress(position));
}

void QtMultimediaBackend::onPlaybackRateChanged(qreal speed) {
  emit speedChanged(convertSpinSpeed(speed));
}

void QtMultimediaBackend::onDurationChanged(qint64 duration) {
  emit durationChanged(convertDuration(duration));
}

void QtMultimediaBackend::onErrorOccurred(QMediaPlayer::Error error, const QString& error_string) {
  QString err = error_string.isEmpty() ? errorToString(error) : error_string;
  emit errorOccurred(err);
}

void QtMultimediaBackend::onAudioAvailable(bool available) {
  emit audioAvailable(available);
}

void QtMultimediaBackend::onVideoAvailable(bool available) {
  emit videoAvailable(available);
}

void QtMultimediaBackend::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
  QString st = mediaStatusToString(status);
  emit statusChanged(st);
}

void QtMultimediaBackend::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
  switch (state) {
    case QMediaPlayer::PlaybackState::StoppedState:
      emit playbackStateChanged(PlayerBackend::PlaybackState::StoppedState);
      break;

    case QMediaPlayer::PlaybackState::PlayingState:
      emit playbackStateChanged(PlayerBackend::PlaybackState::PlayingState);
      break;

    case QMediaPlayer::PlaybackState::PausedState:
      emit playbackStateChanged(PlayerBackend::PlaybackState::PausedState);
      break;
  }
}

void QtMultimediaBackend::onSeekableChanged(bool seekable) {
  emit seekableChanged(seekable);
}

int QtMultimediaBackend::convertToSliderVolume(float volume) const {
  return volume * 100;
}
