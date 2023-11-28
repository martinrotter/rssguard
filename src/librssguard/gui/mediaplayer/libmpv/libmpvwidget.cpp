// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvwidget.h"

#include "gui/mediaplayer/libmpv/qthelper.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <stdexcept>

#include <QKeyEvent>
#include <QOpenGLContext>

static void wakeup(void* ctx) {
  QMetaObject::invokeMethod((LibMpvWidget*)ctx, "on_mpv_events", Qt::QueuedConnection);
}

static void* get_proc_address(void* ctx, const char* name) {
  Q_UNUSED(ctx);
  QOpenGLContext* glctx = QOpenGLContext::currentContext();
  if (!glctx)
    return nullptr;
  return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
}

LibMpvWidget::LibMpvWidget(QWidget* parent, Qt::WindowFlags f) : QOpenGLWidget(parent, f) {
  mpv = mpv_create();
  if (!mpv)
    throw std::runtime_error("could not create mpv context");

  mpv_set_option_string(mpv, "terminal", "yes");
  mpv_set_option_string(mpv, "msg-level", "all=v");

  mpv_set_option_string(mpv, "input-default-bindings", "yes");
  /*
  mpv_set_option_string(
      mpv, "config-dir",
      "c:\\Users\\rotter\\Downloads\\mpv-examples-master\\mpv-examples-"
      "master\\libmpv\\build-qt_opengl-Desktop_Qt_6_6_0_MSVC2017_64bit-"
      "Debug\\debug\\");
*/
  /*
  mpv_set_option_string(mpv, "input-conf", "input.conf");
  */

  if (mpv_initialize(mpv) < 0)
    throw std::runtime_error("could not initialize mpv context");

  // Request hw decoding, just for testing.
  mpv::qt::set_option_variant(mpv, "hwdec", "auto");

  mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_DOUBLE);
  mpv_observe_property(mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
  mpv_set_wakeup_callback(mpv, wakeup, this);

  installEventFilter(this);
}

LibMpvWidget::~LibMpvWidget() {
  makeCurrent();
  if (mpv_gl)
    mpv_render_context_free(mpv_gl);
  mpv_terminate_destroy(mpv);
}

void LibMpvWidget::command(const QVariant& params) {
  mpv::qt::command_variant(mpv, params);
}

void LibMpvWidget::setProperty(const QString& name, const QVariant& value) {
  mpv::qt::set_property_variant(mpv, name, value);
}

QVariant LibMpvWidget::getProperty(const QString& name) const {
  return mpv::qt::get_property_variant(mpv, name);
}

void LibMpvWidget::initializeGL() {
  mpv_opengl_init_params gl_init_params[1] = {get_proc_address, nullptr};
  mpv_render_param params[]{{MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
                            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
                            {MPV_RENDER_PARAM_INVALID, nullptr}};

  if (mpv_render_context_create(&mpv_gl, mpv, params) < 0)
    throw std::runtime_error("failed to initialize mpv GL context");
  mpv_render_context_set_update_callback(mpv_gl, LibMpvWidget::on_update, reinterpret_cast<void*>(this));
}

void LibMpvWidget::paintGL() {
  mpv_opengl_fbo mpfbo{static_cast<int>(defaultFramebufferObject()), width(), height(), 0};
  int flip_y{1};

  mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
                               {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                               {MPV_RENDER_PARAM_INVALID, nullptr}};
  // See render_gl.h on what OpenGL environment mpv expects, and
  // other API details.
  mpv_render_context_render(mpv_gl, params);
}

void LibMpvWidget::on_mpv_events() {
  // Process all events, until the event queue is empty.
  while (mpv) {
    mpv_event* event = mpv_wait_event(mpv, 0);
    if (event->event_id == MPV_EVENT_NONE) {
      break;
    }
    handle_mpv_event(event);
  }
}

void LibMpvWidget::handle_mpv_event(mpv_event* event) {
  switch (event->event_id) {
    case MPV_EVENT_PROPERTY_CHANGE: {
      mpv_event_property* prop = (mpv_event_property*)event->data;
      if (strcmp(prop->name, "time-pos") == 0) {
        if (prop->format == MPV_FORMAT_DOUBLE) {
          double time = *(double*)prop->data;
          Q_EMIT positionChanged(time);
        }
      }
      else if (strcmp(prop->name, "duration") == 0) {
        if (prop->format == MPV_FORMAT_DOUBLE) {
          double time = *(double*)prop->data;
          Q_EMIT durationChanged(time);
        }
      }
      break;
    }
    default:;
      // Ignore uninteresting or unknown events.
  }
}

// Make Qt invoke mpv_render_context_render() to draw a new/updated video frame.
void LibMpvWidget::maybeUpdate() {
  // If the Qt window is not visible, Qt's update() will just skip rendering.
  // This confuses mpv's render API, and may lead to small occasional
  // freezes due to video rendering timing out.
  // Handle this by manually redrawing.
  // Note: Qt doesn't seem to provide a way to query whether update() will
  //       be skipped, and the following code still fails when e.g. switching
  //       to a different workspace with a reparenting window manager.
  if (window()->isMinimized()) {
    makeCurrent();
    paintGL();
    context()->swapBuffers(context()->surface());
    doneCurrent();
  }
  else {
    update();
  }
}

void LibMpvWidget::on_update(void* ctx) {
  QMetaObject::invokeMethod((LibMpvWidget*)ctx, "maybeUpdate");
}

void LibMpvWidget::keyPressEvent(QKeyEvent* event) {
  mpv_set_option_string(mpv, "keypress", event->text().toLocal8Bit().constData());
}

bool LibMpvWidget::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::Type::KeyPress) {
    QString txt = dynamic_cast<QKeyEvent*>(event)->text();

    command(QStringList() << "keypress" << txt.toLocal8Bit().constData());

    return true;
  }

  return false;
}
