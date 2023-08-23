// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/plaintoolbutton.h"

#include <QAction>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>

PlainToolButton::PlainToolButton(QWidget* parent) : QToolButton(parent), m_padding(0) {}

void PlainToolButton::paintEvent(QPaintEvent* e) {
  Q_UNUSED(e)
  QPainter p(this);
  QRect rect(QPoint(0, 0), size());

  // Set padding.
  rect.adjust(m_padding, m_padding, -m_padding, -m_padding);

  if (isEnabled()) {
    if (underMouse() || isChecked()) {
      p.setOpacity(0.7);
    }
  }
  else {
    p.setOpacity(0.3);
  }

  icon().paint(&p, rect);

  if (menu() != nullptr) {
    // Draw "dropdown" triangle.
    QPainterPath path;

    const int triangle_width = int(rect.width() * 0.4);
    const int triangle_height = int(triangle_width * 0.5);
    const auto triangle_origin = rect.bottomRight() - QPoint(triangle_width, triangle_height);

    path.moveTo(triangle_origin);
    path.lineTo(QPoint(rect.right(), triangle_origin.y()));
    path.lineTo(triangle_origin + QPoint(triangle_width / 2, triangle_height));
    path.lineTo(triangle_origin);

    p.fillPath(path, QBrush(Qt::GlobalColor::black));
  }
}

int PlainToolButton::padding() const {
  return m_padding;
}

void PlainToolButton::setPadding(int padding) {
  m_padding = padding;
  repaint();
}

void PlainToolButton::setChecked(bool checked) {
  QToolButton::setChecked(checked);
  repaint();
}

void PlainToolButton::reactOnActionChange(QAction* action) {
  if (action != nullptr) {
    setEnabled(action->isEnabled());
    setCheckable(action->isCheckable());
    setChecked(action->isChecked());
    setIcon(action->icon());
    setToolTip(action->toolTip());
  }
}

void PlainToolButton::reactOnSenderActionChange() {
  reactOnActionChange(qobject_cast<QAction*>(sender()));
}
