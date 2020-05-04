// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/squeezelabel.h"

SqueezeLabel::SqueezeLabel(QWidget* parent) : QLabel(parent) {}

void SqueezeLabel::paintEvent(QPaintEvent* event) {
  if (m_squeezedTextCache != text()) {
    m_squeezedTextCache = text();
    QFontMetrics fm = fontMetrics();

#if QT_VERSION >= 0x050B00 // Qt >= 5.11.0
    if (fm.horizontalAdvance(m_squeezedTextCache) > contentsRect().width()) {
#else
    if (fm.width(m_squeezedTextCache) > contentsRect().width()) {
#endif
      setText(fm.elidedText(text(), Qt::ElideMiddle, width()));
    }
  }

  QLabel::paintEvent(event);
}
