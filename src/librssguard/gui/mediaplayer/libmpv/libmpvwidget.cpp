// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvwidget.h"

#include <mpv/client.h>

static void wakeup(void* ctx) {
  // This callback is invoked from any mpv thread (but possibly also
  // recursively from a thread that is calling the mpv API). Just notify
  // the Qt GUI thread to wake up (so that it can process events with
  // mpv_wait_event()), and return as quickly as possible.
  LibMpvWidget* backend = (LibMpvWidget*)ctx;
  emit backend->launchMpvEvents();
}

LibMpvWidget::LibMpvWidget(mpv_handle* mpv_handle, QWidget* parent) : QWidget(parent), m_mpvHandle(mpv_handle) {
  setAttribute(Qt::WidgetAttribute::WA_DontCreateNativeAncestors);
  setAttribute(Qt::WidgetAttribute::WA_NativeWindow);
  setMouseTracking(true);
}

void LibMpvWidget::bind() {
  auto raw_wid = winId();

#if defined(Q_OS_WIN)
  // Truncate to 32-bit, as all Windows handles are. This also ensures
  // it doesn't go negative.
  int64_t wid = static_cast<uint32_t>(raw_wid);
#else
  int64_t wid = raw_wid;
#endif

  mpv_set_option(m_mpvHandle, "wid", MPV_FORMAT_INT64, &wid);

  mpv_set_wakeup_callback(m_mpvHandle, wakeup, this);
}
