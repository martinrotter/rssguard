// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvwidget.h"

#include "miscellaneous/application.h"

#include <mpv/client.h>

#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
#include <QOpenGLContext>
#endif

static void wakeup(void* ctx) {
  // This callback is invoked from any mpv thread (but possibly also
  // recursively from a thread that is calling the mpv API). Just notify
  // the Qt GUI thread to wake up (so that it can process events with
  // mpv_wait_event()), and return as quickly as possible.
  LibMpvWidget* backend = (LibMpvWidget*)ctx;
  emit backend->launchMpvEvents();
}

LibMpvWidget::LibMpvWidget(mpv_handle* mpv_handle, QWidget* parent)
  : BASE_WIDGET(parent), m_mpvHandle(mpv_handle)
#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
    ,
    m_mpvGL(nullptr)
#endif
{
#if !defined(MEDIAPLAYER_LIBMPV_OPENGL)
  setAttribute(Qt::WidgetAttribute::WA_DontCreateNativeAncestors);
  setAttribute(Qt::WidgetAttribute::WA_NativeWindow);
#endif

  setMouseTracking(true);
}

LibMpvWidget::~LibMpvWidget() {
  destroyHandle();
}

void LibMpvWidget::bind() {
#if !defined(MEDIAPLAYER_LIBMPV_OPENGL)
  auto raw_wid = winId();

#if defined(Q_OS_WIN)
  // Truncate to 32-bit, as all Windows handles are. This also ensures
  // it doesn't go negative.
  int64_t wid = static_cast<uint32_t>(raw_wid);
#else
  int64_t wid = raw_wid;
#endif

  mpv_set_option(m_mpvHandle, "wid", MPV_FORMAT_INT64, &wid);
#endif

  mpv_set_wakeup_callback(m_mpvHandle, wakeup, this);
}

void LibMpvWidget::destroyHandle() {
#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
  makeCurrent();

  if (m_mpvGL != nullptr) {
    mpv_render_context_free(m_mpvGL);
    m_mpvGL = nullptr;
  }

  doneCurrent();
#endif
}

#if defined(MEDIAPLAYER_LIBMPV_OPENGL)
static void* get_proc_address(void* ctx, const char* name) {
  Q_UNUSED(ctx);

  QOpenGLContext* glctx = QOpenGLContext::currentContext();

  if (!glctx) {
    return nullptr;
  }

  return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
}

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

void LibMpvWidget::onMpvRedraw(void* ctx) {
  QMetaObject::invokeMethod((LibMpvWidget*)ctx, &LibMpvWidget::maybeUpdate, Qt::ConnectionType::QueuedConnection);
}

void LibMpvWidget::initializeGL() {
  mpv_opengl_init_params gl_init_params[1] = {get_proc_address, nullptr};
  mpv_render_param display{MPV_RENDER_PARAM_INVALID, nullptr};

#if QT_VERSION_MAJOR == 6 && defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN) && !defined(Q_OS_ANDROID)
#if defined(QT_FEATURE_xcb) && QT_CONFIG(xcb)
  if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
    display.type = MPV_RENDER_PARAM_X11_DISPLAY;
    display.data = qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
  }
#endif

#if defined(QT_FEATURE_wayland) && QT_CONFIG(wayland)
  if (qApp->isWayland()) {
    display.type = MPV_RENDER_PARAM_WL_DISPLAY;
    display.data = qApp->nativeInterface<QNativeInterface::QWaylandApplication>()->display();
  }
#endif
#endif

  mpv_render_param params[]{{MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
                            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
                            display,
                            {MPV_RENDER_PARAM_INVALID, nullptr}};

  if (mpv_render_context_create(&m_mpvGL, m_mpvHandle, params) < 0) {
    qFatal("failed to initialize mpv GL context");
  }

  mpv_render_context_set_update_callback(m_mpvGL, LibMpvWidget::onMpvRedraw, reinterpret_cast<void*>(this));
}

void LibMpvWidget::paintGL() {
  qreal scaling_ratio = window()->devicePixelRatio();
  mpv_opengl_fbo mpfbo{static_cast<int>(defaultFramebufferObject()),
                       int(width() * scaling_ratio),
                       int(height() * scaling_ratio),
                       0};
  int flip_y{1};

  mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
                               {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
                               {MPV_RENDER_PARAM_INVALID, nullptr}};

  mpv_render_context_render(m_mpvGL, params);
}
#endif
