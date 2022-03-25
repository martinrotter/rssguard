// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/squeezelabel.h"

SqueezeLabel::SqueezeLabel(QWidget* parent) : QLabel(parent) {}

void SqueezeLabel::paintEvent(QPaintEvent* event) {
  if (m_squeezedTextCache != text()) {
    m_squeezedTextCache = text();
    QFontMetrics fm = fontMetrics();

    if (fm.horizontalAdvance(m_squeezedTextCache) > contentsRect().width()) {
      setText(fm.elidedText(text(), Qt::ElideMiddle, width()));
    }
  }

  QLabel::paintEvent(event);
}
