// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvbackend.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "gui/mediaplayer/libmpv/libmpvwidget.h"
#include "gui/mediaplayer/libmpv/qthelper.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"

#include <clocale>
#include <sstream>
#include <stdexcept>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLayout>

#define EVENT_CODE_FS       2
#define EVENT_CODE_VOLUME   3
#define EVENT_CODE_DURATION 4
#define EVENT_CODE_MUTE     5
#define EVENT_CODE_POSITION 6
#define EVENT_CODE_SPEED    7
#define EVENT_CODE_SEEKABLE 8
#define EVENT_CODE_TRACKS   9
#define EVENT_CODE_PAUSE    10
#define EVENT_CODE_IDLE     11
#define EVENT_CODE_STOP     12

#define CONFIG_MAIN_NAME  "mpv.conf"
#define CONFIG_INPUT_NAME "input.conf"

static void wakeup(void* ctx) {
  // This callback is invoked from any mpv thread (but possibly also
  // recursively from a thread that is calling the mpv API). Just notify
  // the Qt GUI thread to wake up (so that it can process events with
  // mpv_wait_event()), and return as quickly as possible.
  LibMpvBackend* backend = (LibMpvBackend*)ctx;
  emit backend->launchMpvEvents();
}

LibMpvBackend::LibMpvBackend(Application* app, QWidget* parent)
  : PlayerBackend(app, parent), m_mpvContainer(nullptr), m_mpvHandle(nullptr) {
  installEventFilter(this);
  loadSettings();

  m_mpvHandle = mpv_create();
  m_mpvContainer = new LibMpvWidget(m_mpvHandle, this);

  if (m_mpvHandle == nullptr) {
    qFatal("cannot create mpv instance");
  }

  setMouseTracking(true);
  layout()->addWidget(m_mpvContainer);

  m_mpvContainer->bind();

  mpv_set_option_string(m_mpvHandle, "msg-level", "all=v");
  mpv_set_option_string(m_mpvHandle, "config", "yes");
  mpv_set_option_string(m_mpvHandle, "force-window", "yes");
  mpv_set_option_string(m_mpvHandle, "script-opts", "osc-idlescreen=no");
  mpv_set_option_string(m_mpvHandle, "hwdec", "auto");
  mpv_set_option_string(m_mpvHandle, "osd-playing-msg", "${media-title}");
  mpv_set_option_string(m_mpvHandle, "osc", "yes");
  mpv_set_option_string(m_mpvHandle, "input-cursor", "yes");
  mpv_set_option_string(m_mpvHandle, "idle", "yes");
  mpv_set_option_string(m_mpvHandle, "save-position-on-quit", "no");
  mpv_set_option_string(m_mpvHandle, "no-resume-playback", "yes");

#if !defined(NDEBUG)
  mpv_set_option_string(m_mpvHandle, "terminal", "yes");
#endif

  if (!m_customConfigFolder.isEmpty()) {
    QByteArray cfg_folder = QDir::toNativeSeparators(m_customConfigFolder).toLocal8Bit();

    mpv_set_option_string(m_mpvHandle, "config-dir", cfg_folder.constData());
  }
  else {
    mpv_set_option_string(m_mpvHandle, "input-default-bindings", "yes");
  }

  // Observe some properties.
  mpv_observe_property(m_mpvHandle, EVENT_CODE_FS, "fullscreen", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_VOLUME, "volume", MPV_FORMAT_INT64);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_DURATION, "duration", MPV_FORMAT_INT64);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_MUTE, "mute", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_POSITION, "time-pos", MPV_FORMAT_INT64);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_SPEED, "speed", MPV_FORMAT_DOUBLE);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_SEEKABLE, "seekable", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_PAUSE, "pause", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_IDLE, "idle-active", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_TRACKS, "track-list", MPV_FORMAT_NODE);

  // From this point on, the wakeup function will be called. The callback
  // can come from any thread, so we use the QueuedConnection mechanism to
  // relay the wakeup in a thread-safe way.
  connect(this,
          &LibMpvBackend::launchMpvEvents,
          this,
          &LibMpvBackend::onMpvEvents,
          Qt::ConnectionType::QueuedConnection);

  mpv_set_wakeup_callback(m_mpvHandle, wakeup, this);

  if (mpv_initialize(m_mpvHandle) < 0) {
    qFatal("cannot create mpv instance");
  }
}

void LibMpvBackend::destroyHandle() {
  if (m_mpvHandle != nullptr) {
    mpv_terminate_destroy(m_mpvHandle);
    m_mpvHandle = nullptr;
  }
}

void LibMpvBackend::loadSettings() {
  if (m_app->settings()->value(GROUP(VideoPlayer), SETTING(VideoPlayer::MpvUseCustomConfigFolder)).toBool()) {
    m_customConfigFolder =
      m_app->replaceUserDataFolderPlaceholder(m_app->settings()
                                                ->value(GROUP(VideoPlayer), SETTING(VideoPlayer::MpvCustomConfigFolder))
                                                .toString());

    installCustomConfig(m_customConfigFolder);
  }
}

LibMpvBackend::~LibMpvBackend() {
  destroyHandle();
}

void LibMpvBackend::handleMpvEvent(mpv_event* event) {
  switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
      mpv_event_property* prop = (mpv_event_property*)event->data;
      processPropertyChange(prop, event->reply_userdata);
      break;
    }

    case MPV_EVENT_FILE_LOADED:
      emit statusChanged(tr("File loaded"));
      emit playbackStateChanged(PlayerBackend::PlaybackState::PlayingState);
      break;

    case MPV_EVENT_END_FILE: {
      mpv_event_end_file* end_file = (mpv_event_end_file*)event->data;
      processEndFile(end_file);
      break;
    }

    case MPV_EVENT_COMMAND_REPLY:
      break;

    case MPV_EVENT_VIDEO_RECONFIG:
      break;

    case MPV_EVENT_LOG_MESSAGE: {
      mpv_event_log_message* msg = (mpv_event_log_message*)event->data;
      processLogMessage(msg);
    }

    case MPV_EVENT_SHUTDOWN: {
      destroyHandle();
      emit closed();
      break;
    }

    default:
      // Ignore uninteresting or unknown events.
      break;
  }
}

const char* LibMpvBackend::mpvDecodeString(void* data) const {
  return *(char**)data;
}

bool LibMpvBackend::mpvDecodeBool(void* data) const {
  return mpvDecodeInt(data) != 0;
}

int LibMpvBackend::mpvDecodeInt(void* data) const {
  return *(int*)data;
}

double LibMpvBackend::mpvDecodeDouble(void* data) const {
  return *(double*)data;
}

QString LibMpvBackend::mpvEncodeKeyboardButton(int btn) const {
  return QString((QChar)btn);
}

QString LibMpvBackend::errorToString(mpv_error error) const {
  switch (mpv_error(error)) {
    case mpv_error::MPV_ERROR_EVENT_QUEUE_FULL:
      return tr("App restart required");

    case mpv_error::MPV_ERROR_NOMEM:
      return tr("Out of memory");

    case mpv_error::MPV_ERROR_UNINITIALIZED:
      return tr("Not initialized yet");

    case mpv_error::MPV_ERROR_INVALID_PARAMETER:
      return tr("Invalid parameter");

    case mpv_error::MPV_ERROR_OPTION_NOT_FOUND:
      return tr("Option not found");

    case mpv_error::MPV_ERROR_OPTION_FORMAT:
      return tr("Option badly formatted");

    case mpv_error::MPV_ERROR_OPTION_ERROR:
      return tr("Cannot set option");

    case mpv_error::MPV_ERROR_PROPERTY_NOT_FOUND:
      return tr("Property does not existing");

    case mpv_error::MPV_ERROR_PROPERTY_FORMAT:
      return tr("Property badly formatted");

    case mpv_error::MPV_ERROR_PROPERTY_UNAVAILABLE:
      return tr("Property N/A");

    case mpv_error::MPV_ERROR_PROPERTY_ERROR:
      return tr("Cannot set property");

    case mpv_error::MPV_ERROR_COMMAND:
      return tr("Cannot run command");

    case mpv_error::MPV_ERROR_LOADING_FAILED:
      return tr("Loading failed");

    case mpv_error::MPV_ERROR_AO_INIT_FAILED:
      return tr("Cannot initialize audio");

    case mpv_error::MPV_ERROR_VO_INIT_FAILED:
      return tr("Cannot initialize video");

    case mpv_error::MPV_ERROR_NOTHING_TO_PLAY:
      return tr("Not a media file");

    case mpv_error::MPV_ERROR_UNKNOWN_FORMAT:
      return tr("Unknown file format");

    case mpv_error::MPV_ERROR_UNSUPPORTED:
      return tr("Unsupported file format");

    case mpv_error::MPV_ERROR_NOT_IMPLEMENTED:
    default:
      return tr("Unknown error (%1)").arg(error);
  }
}

void LibMpvBackend::onMpvEvents() {
  while (m_mpvHandle != nullptr) {
    mpv_event* event = mpv_wait_event(m_mpvHandle, 0);

    if (event->event_id == MPV_EVENT_NONE) {
      break;
    }

    handleMpvEvent(event);
  }
}

void LibMpvBackend::processEndFile(mpv_event_end_file* end_file) {
  switch (end_file->reason) {
    case MPV_END_FILE_REASON_STOP:
      emit statusChanged(tr("Stopped"));
      emit playbackStateChanged(PlayerBackend::PlaybackState::StoppedState);
      break;

    case MPV_END_FILE_REASON_EOF:
    case MPV_END_FILE_REASON_QUIT:
      emit statusChanged(tr("File ended"));
      emit playbackStateChanged(PlayerBackend::PlaybackState::StoppedState);
      break;

    case MPV_END_FILE_REASON_ERROR:
      emit errorOccurred(errorToString(mpv_error(end_file->error)));
      emit playbackStateChanged(PlayerBackend::PlaybackState::StoppedState);
      break;

    case MPV_END_FILE_REASON_REDIRECT:
    default:
      break;
  }
}

void LibMpvBackend::processTracks(const QJsonDocument& json) {
  QVariantList vars = json.array().toVariantList();
  auto linq = boolinq::from(vars);

  bool any_audio_track = linq.any([](const QVariant& var) {
    return var.toHash().value("type") == QSL("audio");
  });

  bool any_video_track = linq.any([](const QVariant& var) {
    return var.toHash().value("type") == QSL("video");
  });

  emit audioAvailable(any_audio_track);
  emit videoAvailable(any_video_track);
}

void LibMpvBackend::processPropertyChange(mpv_event_property* prop, uint64_t property_code) {
  if (prop == nullptr || prop->data == nullptr) {
    return;
  }

  switch (property_code) {
    case EVENT_CODE_FS:
      emit fullscreenChanged(mpvDecodeBool(prop->data));
      break;

    case EVENT_CODE_VOLUME: {
      int volume = mpvDecodeInt(prop->data);
      emit volumeChanged(volume);
      break;
    }

    case EVENT_CODE_POSITION: {
      int pos = mpvDecodeInt(prop->data);
      emit positionChanged(pos);
      break;
    }

    case EVENT_CODE_DURATION: {
      int dr = mpvDecodeInt(prop->data);
      emit durationChanged(dr);
      break;
    }

    case EVENT_CODE_IDLE: {
      /*
      bool idle = mpvDecodeBool(prop->data);

      if (idle) {
        emit playbackStateChanged(PlayerBackend::PlaybackState::StoppedState);
      }
*/
      break;
    }

    case EVENT_CODE_PAUSE: {
      bool paused = mpvDecodeBool(prop->data);

      if (paused) {
        emit playbackStateChanged(PlayerBackend::PlaybackState::PausedState);
      }
      else {
        emit playbackStateChanged(PlayerBackend::PlaybackState::PlayingState);
      }

      break;
    }

    case EVENT_CODE_SEEKABLE:
      emit seekableChanged(mpvDecodeBool(prop->data));
      break;

    case EVENT_CODE_SPEED: {
      double sp = mpvDecodeDouble(prop->data);
      emit speedChanged(int(sp * 100));
      break;
    }

    case EVENT_CODE_TRACKS: {
      if (prop->format == MPV_FORMAT_NODE) {
        QVariant v = mpv::qt::node_to_variant((mpv_node*)prop->data);
        QJsonDocument d = QJsonDocument::fromVariant(v);

        processTracks(d);
      }

      break;
    }

    case EVENT_CODE_MUTE:
      emit mutedChanged(mpvDecodeBool(prop->data));
      break;

    default:
      break;
  }
}

void LibMpvBackend::processLogMessage(mpv_event_log_message* msg) {
  std::stringstream ss;
  ss << "[" << msg->prefix << "] " << msg->level << ": " << msg->text;

  appendLog(QString::fromStdString(ss.str()));
}

void LibMpvBackend::appendLog(const QString& text) {
  qDebugNN << LOGSEC_MPV << text;
}

bool LibMpvBackend::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

  if (event->type() == QEvent::Type::ShortcutOverride) {
    // NOTE: If user presses key which is application-wide assigned to some
    // action, do not propagate the shortcut to application.
    event->accept();
    return true;
  }

  if (m_mpvHandle != nullptr) {
    if (event->type() == QEvent::Type::Wheel && watched == this) {
      auto* mouse_event = dynamic_cast<QWheelEvent*>(event);
      bool is_up = mouse_event->angleDelta().y() >= 0;

      qDebugNN << LOGSEC_MPV << "Wheel:" << QUOTE_W_SPACE_DOT(is_up);

      const char* args[] = {"keypress", is_up ? "MOUSE_BTN3" : "MOUSE_BTN4", nullptr};

      mpv_command_async(m_mpvHandle, 0, args);
      event->accept();
      return true;
    }

    if ((event->type() == QEvent::Type::MouseButtonRelease || event->type() == QEvent::Type::MouseButtonPress) &&
        watched == this) {
      bool press = event->type() == QEvent::Type::MouseButtonPress;

      qDebugNN << LOGSEC_MPV << "Mouse press/release.";

      const char* args[] = {press ? "keydown" : "keyup", "MOUSE_BTN0", nullptr};

      mpv_command_async(m_mpvHandle, 0, args);
      event->accept();
      return true;
    }

    if (event->type() == QEvent::Type::MouseButtonDblClick && watched == this) {
      qDebugNN << LOGSEC_MPV << "Mouse double-click.";

      const char* args[] = {"keypress", "MOUSE_BTN0_DBL", nullptr};

      mpv_command_async(m_mpvHandle, 0, args);
      event->accept();
      return true;
    }

    if (event->type() == QEvent::Type::MouseMove && watched == this) {
      auto* mouse_event = dynamic_cast<QMouseEvent*>(event);
      auto position = mouse_event->pos();
      auto x_str = QString::number(position.x()).toLocal8Bit();
      auto y_str = QString::number(position.y()).toLocal8Bit();
      const char* x = x_str.constData();
      const char* y = y_str.constData();

      const char* args[] = {"mouse", x, y, nullptr};

      mpv_command_async(m_mpvHandle, 0, args);
    }

    if (event->type() == QEvent::Type::KeyRelease) {
      // We catch all keypresses (even from surrounding widgets).
      QKeyEvent* key_event = dynamic_cast<QKeyEvent*>(event);
      QString keys =
        QKeySequence(key_event->key() | key_event->modifiers()).toString(QKeySequence::SequenceFormat::PortableText);
      QByteArray byte_named_key = keys.toLocal8Bit();

      const char* args[] = {"keypress", byte_named_key.constData(), nullptr};

      mpv_command_async(m_mpvHandle, 0, args);
      event->accept();
      return true;
    }
  }

  return false;
}

void LibMpvBackend::playUrl(const QUrl& url) {
  char* str;

  mpv_get_property(m_mpvHandle, "ytdl_path", MPV_FORMAT_STRING, &str);

  m_url = url;

  if (m_mpvHandle != nullptr) {
    auto eb = url.toString().toLocal8Bit();
    const char* css = eb.data();
    const char* args[] = {"loadfile", css, nullptr};
    mpv_command_async(m_mpvHandle, 0, args);
  }
}

void LibMpvBackend::playPause() {
  int idle;
  mpv_get_property(m_mpvHandle, "idle-active", MPV_FORMAT_FLAG, &idle);

  if (idle) {
    playUrl(m_url);
  }
  else {
    int paused;
    mpv_get_property(m_mpvHandle, "pause", MPV_FORMAT_FLAG, &paused);

    paused = paused == 0 ? 1 : 0;

    mpv_set_property_async(m_mpvHandle, EVENT_CODE_PAUSE, "pause", MPV_FORMAT_FLAG, &paused);
  }
}

void LibMpvBackend::pause() {
  int in = 1;
  mpv_set_property_async(m_mpvHandle, EVENT_CODE_PAUSE, "pause", MPV_FORMAT_FLAG, &in);
}

void LibMpvBackend::stop() {
  const char* args[] = {"stop", nullptr};
  mpv_command_async(m_mpvHandle, EVENT_CODE_STOP, args);
}

void LibMpvBackend::setPlaybackSpeed(int speed) {
  if (m_mpvHandle != nullptr) {
    double sp = speed / 100.0;
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_SPEED, "speed", MPV_FORMAT_DOUBLE, &sp);
  }
}

void LibMpvBackend::setVolume(int volume) {
  if (m_mpvHandle != nullptr) {
    uint64_t vol = volume;
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_VOLUME, "volume", MPV_FORMAT_INT64, &vol);
  }
}

void LibMpvBackend::setPosition(int position) {
  if (m_mpvHandle != nullptr) {
    uint64_t pos = position;
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_POSITION, "time-pos", MPV_FORMAT_INT64, &pos);
  }
}

void LibMpvBackend::setFullscreen(bool fullscreen) {
  if (m_mpvHandle != nullptr) {
    const char* fs = fullscreen ? "yes" : "no";
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_FS, "fullscreen", MPV_FORMAT_STRING, &fs);
  }
}

QUrl LibMpvBackend::url() const {
  return m_url;
}

int LibMpvBackend::position() const {
  uint64_t out;
  mpv_get_property(m_mpvHandle, "time-pos", MPV_FORMAT_INT64, &out);

  return out;
}

int LibMpvBackend::duration() const {
  uint64_t out;
  mpv_get_property(m_mpvHandle, "duration", MPV_FORMAT_INT64, &out);

  return out;
}

void LibMpvBackend::installCustomConfig(const QString& directory) {
  QDir().mkpath(directory);
  QDir config_dir(directory);

  for (const QString& cfg_file : QStringList{QSL(CONFIG_MAIN_NAME), QSL(CONFIG_INPUT_NAME)}) {
    if (!config_dir.exists(cfg_file)) {
      qDebugNN << LOGSEC_MPV << "Copying sample" << QUOTE_W_SPACE(cfg_file) << "to"
               << QUOTE_W_SPACE_DOT(config_dir.absolutePath());
      IOFactory::copyFile(QSL(":/scripts/mpv/%1").arg(cfg_file), config_dir.absoluteFilePath(cfg_file));
    }
    else {
      qDebugNN << LOGSEC_MPV << "Configuration file" << QUOTE_W_SPACE(cfg_file) << "already exists.";
    }
  }
}

void LibMpvBackend::setMuted(bool muted) {
  if (m_mpvHandle != nullptr) {
    const char* mtd = muted ? "yes" : "no";
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_MUTE, "mute", MPV_FORMAT_STRING, &mtd);
  }
}
