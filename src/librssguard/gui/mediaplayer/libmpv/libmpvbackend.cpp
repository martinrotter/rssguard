// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvbackend.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "gui/mediaplayer/libmpv/qthelper.h"

#include <mpv/client.h>

#include <clocale>
#include <sstream>
#include <stdexcept>

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

static void wakeup(void* ctx) {
  // This callback is invoked from any mpv thread (but possibly also
  // recursively from a thread that is calling the mpv API). Just notify
  // the Qt GUI thread to wake up (so that it can process events with
  // mpv_wait_event()), and return as quickly as possible.
  LibMpvBackend* backend = (LibMpvBackend*)ctx;
  emit backend->launchMpvEvents();
}

LibMpvBackend::LibMpvBackend(QWidget* parent)
  : PlayerBackend(parent), m_mpvContainer(new QWidget(this)), m_mpvHandle(nullptr) {
  installEventFilter(this);

  m_mpvHandle = mpv_create();

  if (m_mpvHandle == nullptr) {
    qFatal("cannot create mpv instance");
  }

  // Create a video child window. Force Qt to create a native window, and
  // pass the window ID to the mpv wid option. Works on: X11, win32, Cocoa.
  m_mpvContainer->setAttribute(Qt::WidgetAttribute::WA_DontCreateNativeAncestors);
  m_mpvContainer->setAttribute(Qt::WidgetAttribute::WA_NativeWindow);

  m_mpvContainer->setMouseTracking(true);
  setMouseTracking(true);

  layout()->addWidget(m_mpvContainer);

  auto raw_wid = m_mpvContainer->winId();

#if defined(Q_OS_WIN)
  // Truncate to 32-bit, as all Windows handles are. This also ensures
  // it doesn't go negative.
  int64_t wid = static_cast<uint32_t>(raw_wid);
#else
  int64_t wid = raw_wid;
#endif

  mpv_set_option(m_mpvHandle, "wid", MPV_FORMAT_INT64, &wid);
  mpv_set_option_string(m_mpvHandle, "input-default-bindings", "yes");
  // mpv_set_option_string(m_mpvHandle, "input-builtin-bindings", "no");
  // mpv_set_option_string(m_mpvHandle, "input-test", "yes");
  mpv_set_option_string(m_mpvHandle, "msg-level", "all=v");
  mpv_set_option_string(m_mpvHandle, "terminal", "yes");
  mpv_set_option_string(m_mpvHandle, "keep-open", "yes");

  // mpv_set_option_string(m_mpvHandle, "no-resume-playback", "yes");
  mpv_set_option_string(m_mpvHandle, "watch-later-dir", "mpv");
  mpv_set_option_string(m_mpvHandle, "config-dir", "mpv");
  mpv_set_option_string(m_mpvHandle, "config", "yes");

  // mpv_set_option_string(m_mpvHandle, "input-terminal", "yes");
  mpv_set_option_string(m_mpvHandle, "hwdec", "auto");
  mpv_set_option_string(m_mpvHandle, "osd-playing-msg", "${media-title}");
  mpv_set_option_string(m_mpvHandle, "osc", "yes");
  mpv_set_option_string(m_mpvHandle, "input-cursor", "yes");

  // mpv_set_option_string(m_mpvHandle, "osd-on-seek", "msg-bar");

  // mpv::qt::set_option_variant(m_mpvHandle, "hwdec", "auto");

  // Enable keyboard input on the X11 window. For the messy details, see
  // --input-vo-keyboard on the manpage.
  // mpv_set_option_string(mpv, "input-vo-keyboard", "yes");

  // Observe some properties.
  // mpv_observe_property(m_mpvHandle, 0, "time-pos", MPV_FORMAT_DOUBLE);
  // mpv_observe_property(m_mpvHandle, 0, "track-list", MPV_FORMAT_NODE);
  // mpv_observe_property(m_mpvHandle, 0, "chapter-list", MPV_FORMAT_NODE);
  // mpv_observe_property(m_mpvHandle, 0, "duration", MPV_FORMAT_NODE);
  // mpv_observe_property(m_mpvHandle, 0, "volume", MPV_FORMAT_NODE);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_FS, "fullscreen", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_VOLUME, "volume", MPV_FORMAT_INT64);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_DURATION, "duration", MPV_FORMAT_INT64);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_MUTE, "mute", MPV_FORMAT_FLAG);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_POSITION, "time-pos", MPV_FORMAT_INT64);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_SPEED, "speed", MPV_FORMAT_DOUBLE);
  mpv_observe_property(m_mpvHandle, EVENT_CODE_SEEKABLE, "seekable", MPV_FORMAT_FLAG);
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

void LibMpvBackend::onMpvEvents() {
  while (m_mpvHandle != nullptr) {
    mpv_event* event = mpv_wait_event(m_mpvHandle, 0);

    if (event->event_id == MPV_EVENT_NONE) {
      break;
    }

    handleMpvEvent(event);
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

  int a = 7;
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

    case EVENT_CODE_SEEKABLE:
      emit seekableChanged(mpvDecodeBool(prop->data));
      break;

    case EVENT_CODE_SPEED: {
      double sp = mpvDecodeDouble(prop->data);
      emit speedChanged(std::min(100, int(sp * 100)));
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

  /*
  if (strcmp(prop->name, "time-pos") == 0) {
    if (prop->format == MPV_FORMAT_DOUBLE) {
      double time = *(double*)prop->data;
      std::stringstream ss;
      ss << "At: " << time;
    }
    else if (prop->format == MPV_FORMAT_NONE) {
    }
  }
  else if (strcmp(prop->name, "chapter-list") == 0 || strcmp(prop->name, "track-list") == 0) {
    // Dump the properties as JSON for demo purposes.
    if (prop->format == MPV_FORMAT_NODE) {
      QVariant v = mpv::qt::node_to_variant((mpv_node*)prop->data);
      // Abuse JSON support for easily printing the mpv_node contents.
      QJsonDocument d = QJsonDocument::fromVariant(v);
      appendLog("Change property " + QString(prop->name) + ":\n");
      appendLog(d.toJson().data());
    }
  }
  */
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

    if (event->type() == QEvent::Type::KeyPress) {
      // We catch all keypresses (even from surrounding widgets).
      QKeyEvent* key_event = dynamic_cast<QKeyEvent*>(event);
      char txt = (char)key_event->key();
      char str[2];

      str[0] = txt;
      str[1] = '\0';

      const char* args[] = {"keypress", str, nullptr};

      mpv_command_async(m_mpvHandle, 0, args);
      event->accept();
      return true;
    }
  }

  return false;
}

void LibMpvBackend::playUrl(const QUrl& url) {
  if (m_mpvHandle != nullptr) {
    auto eb = url.toString().toLocal8Bit();
    const char* css = eb.data();
    const char* args[] = {"loadfile", css, nullptr};
    mpv_command_async(m_mpvHandle, 0, args);
  }
}

void LibMpvBackend::playPause() {}

void LibMpvBackend::pause() {}

void LibMpvBackend::stop() {}

void LibMpvBackend::setPlaybackSpeed(int speed) {}

void LibMpvBackend::setVolume(int volume) {
  if (m_mpvHandle != nullptr) {
    uint64_t vol = volume;
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_VOLUME, "volume", MPV_FORMAT_INT64, &vol);
  }
}

void LibMpvBackend::setPosition(int position) {}

void LibMpvBackend::setFullscreen(bool fullscreen) {
  if (m_mpvHandle != nullptr) {
    const char* fs = fullscreen ? "yes" : "no";
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_FS, "fullscreen", MPV_FORMAT_STRING, &fs);
  }
}

QUrl LibMpvBackend::url() const {
  return {};
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

void LibMpvBackend::setMuted(bool muted) {
  if (m_mpvHandle != nullptr) {
    const char* mtd = muted ? "yes" : "no";
    mpv_set_property_async(m_mpvHandle, EVENT_CODE_MUTE, "mute", MPV_FORMAT_STRING, &mtd);
  }
}
