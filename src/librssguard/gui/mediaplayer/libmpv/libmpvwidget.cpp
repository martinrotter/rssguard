// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/mediaplayer/libmpv/libmpvwidget.h"

#include <mpv/client.h>

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
}
