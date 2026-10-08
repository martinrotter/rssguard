// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/webviewers/qtextbrowser/textbrowserimagehandler.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <QPainter>
#include <QTextImageFormat>

TextBrowserImageHandler::TextBrowserImageHandler(QTextObjectInterface* native_handler, QObject* parent)
  : QObject(parent), m_nativeHandler(native_handler) {}

void TextBrowserImageHandler::setZoomFactor(qreal zoom_factor) {
  m_zoomFactor = zoom_factor;
}

QSizeF TextBrowserImageHandler::intrinsicSize(QTextDocument* document, int position, const QTextFormat& format) {
  QTextImageFormat preferred_format = format.toImageFormat();

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  // Fit the final, zoomed size. Scaling Qt's already fitted size would overflow
  // percentage limits when zooming in and shrink it too far when zooming out.
  preferred_format.clearProperty(QTextFormat::ImageMaxWidth);
#endif

  QSizeF size = m_nativeHandler->intrinsicSize(document, position, preferred_format);
  const qreal maximum_height = format.property(MaximumHeightProperty).toDouble();

  if (maximum_height > 0.0 && std::isfinite(maximum_height) && size.height() > 0.0) {
    QTextImageFormat height_probe(preferred_format);
    height_probe.setWidth(1.0);
    height_probe.setHeight(maximum_height);

    // Native sizing applies the paint device's DPI, including on print clones.
    const qreal height_limit = m_nativeHandler->intrinsicSize(document, position, height_probe).height();

    if (height_limit >= 0.0 && std::isfinite(height_limit)) {
      size *= std::min(qreal(1.0), height_limit / size.height());
    }
  }

  size *= m_zoomFactor;

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
  if (format.hasProperty(QTextFormat::ImageMaxWidth) && size.width() > 0.0) {
    QTextImageFormat width_probe = format.toImageFormat();
    width_probe.setWidth(std::numeric_limits<int>::max() / 2);
    width_probe.setHeight(1.0);

    qreal width_limit = m_nativeHandler->intrinsicSize(document, position, width_probe).width();

    if (width_probe.maximumWidth().type() != QTextLength::PercentageLength) {
      width_limit *= m_zoomFactor;
    }

    if (width_limit >= 0.0 && std::isfinite(width_limit)) {
      size *= std::min(qreal(1.0), width_limit / size.width());
    }
  }
#endif

  return size;
}

void TextBrowserImageHandler::drawObject(QPainter* painter,
                                         const QRectF& rect,
                                         QTextDocument* document,
                                         int position,
                                         const QTextFormat& format) {
  painter->save();
  painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
  m_nativeHandler->drawObject(painter, rect, document, position, format);
  painter->restore();
}
