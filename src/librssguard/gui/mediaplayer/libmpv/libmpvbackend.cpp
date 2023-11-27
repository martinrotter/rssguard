// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvbackend.h"

#include "definitions/definitions.h"
#include "gui/mediaplayer/libmpv/qthelper.h"

#include <mpv/client.h>

#include <clocale>
#include <sstream>
#include <stdexcept>

#include <QJsonDocument>
#include <QKeyEvent>
#include <QLayout>

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
  m_mpvHandle = mpv_create();

  if (m_mpvHandle == nullptr) {
    qFatal("cannot create mpv instance");
  }

  // Create a video child window. Force Qt to create a native window, and
  // pass the window ID to the mpv wid option. Works on: X11, win32, Cocoa
  m_mpvContainer->setAttribute(Qt::WidgetAttribute::WA_DontCreateNativeAncestors);
  m_mpvContainer->setAttribute(Qt::WidgetAttribute::WA_NativeWindow);

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
  mpv_set_option_string(m_mpvHandle, "msg-level", "all=v");
  mpv::qt::set_option_variant(m_mpvHandle, "hwdec", "auto");

  // Enable keyboard input on the X11 window. For the messy details, see
  // --input-vo-keyboard on the manpage.
  // mpv_set_option_string(mpv, "input-vo-keyboard", "yes");

  // Observe some properties.
  mpv_observe_property(m_mpvHandle, 0, "time-pos", MPV_FORMAT_DOUBLE);
  mpv_observe_property(m_mpvHandle, 0, "track-list", MPV_FORMAT_NODE);
  mpv_observe_property(m_mpvHandle, 0, "chapter-list", MPV_FORMAT_NODE);

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

LibMpvBackend::~LibMpvBackend() {
  if (m_mpvHandle != nullptr) {
    mpv_terminate_destroy(m_mpvHandle);
  }
}

void LibMpvBackend::handle_mpv_event(mpv_event* event) {
  switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
      mpv_event_property* prop = (mpv_event_property*)event->data;
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
          append_log("Change property " + QString(prop->name) + ":\n");
          append_log(d.toJson().data());
        }
      }
      break;
    }

    case MPV_EVENT_VIDEO_RECONFIG: {
      // Retrieve the new video size.
      int64_t w, h;
      if (mpv_get_property(m_mpvHandle, "dwidth", MPV_FORMAT_INT64, &w) >= 0 &&
          mpv_get_property(m_mpvHandle, "dheight", MPV_FORMAT_INT64, &h) >= 0 && w > 0 && h > 0) {
        // Note that the MPV_EVENT_VIDEO_RECONFIG event doesn't necessarily
        // imply a resize, and you should check yourself if the video
        // dimensions really changed.
        // mpv itself will scale/letter box the video to the container size
        // if the video doesn't fit.
        std::stringstream ss;
        ss << "Reconfig: " << w << " " << h;
        // statusBar()->showMessage(QString::fromStdString(ss.str()));
      }
      break;
    }

    case MPV_EVENT_LOG_MESSAGE: {
      struct mpv_event_log_message* msg = (struct mpv_event_log_message*)event->data;
      std::stringstream ss;
      ss << "[" << msg->prefix << "] " << msg->level << ": " << msg->text;
      qDebugNN << LOGSEC_MPV << QString::fromStdString(ss.str());
      break;
    }

    case MPV_EVENT_SHUTDOWN: {
      mpv_terminate_destroy(m_mpvHandle);
      m_mpvHandle = NULL;
      break;
    }

    default:
      // Ignore uninteresting or unknown events.
      break;
  }
}

void LibMpvBackend::onMpvEvents() {
  while (m_mpvHandle != nullptr) {
    mpv_event* event = mpv_wait_event(m_mpvHandle, 0);

    if (event->event_id == MPV_EVENT_NONE) {
      break;
    }

    handle_mpv_event(event);
  }
}

void LibMpvBackend::append_log(const QString& text) {
  qDebugNN << LOGSEC_MPV << text;
}

bool LibMpvBackend::eventFilter(QObject* watched, QEvent* event) {
  Q_UNUSED(watched)

  if (event->type() == QEvent::Type::KeyPress) {
    char txt = (char)dynamic_cast<QKeyEvent*>(event)->key();
    char str[2];

    str[0] = txt;
    str[1] = '\0';

    const char* args[] = {"keypress", str, NULL};

    mpv_command_async(m_mpvHandle, 0, args);
    return true;
  }

  return false;
}

void LibMpvBackend::playUrl(const QUrl& url) {
  auto eb = url.toString().toLocal8Bit();
  const char* css = eb.data();
  const char* args[] = {"loadfile", css, NULL};
  mpv_command_async(m_mpvHandle, 0, args);
}

void LibMpvBackend::playPause() {}

void LibMpvBackend::pause() {}

void LibMpvBackend::stop() {}

void LibMpvBackend::setPlaybackSpeed(int speed) {}

void LibMpvBackend::setVolume(int volume) {}

void LibMpvBackend::setPosition(int position) {}

QUrl LibMpvBackend::url() const {
  return {};
}

int LibMpvBackend::position() const {
  return 0;
}

int LibMpvBackend::duration() const {
  return 0;
}
