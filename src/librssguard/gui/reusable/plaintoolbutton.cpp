// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/plaintoolbutton.h"

#include <QAction>
#include <QGuiApplication>
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

  p.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
  p.setRenderHint(QPainter::RenderHint::Antialiasing, true);

  QRect rect(QPoint(0, 0), size());

  // Set padding.
  rect.adjust(m_padding, m_padding, -m_padding, -m_padding);

  if (isEnabled()) {
    if (underMouse() || isChecked()) {
      p.setOpacity(0.8);
    }
  }
  else {
    p.setOpacity(0.2);
  }

  icon().paint(&p, rect);

  if (menu() != nullptr) {
    // Draw "dropdown" triangle.
    QPainterPath path;

    const auto triangle_width = rect.width() * 0.35;
    const auto triangle_height = triangle_width * 0.4;
    const auto triangle_origin = QPointF(rect.bottomRight()) - QPointF(triangle_width, triangle_height);

    path.moveTo(triangle_origin);
    path.lineTo(QPointF(rect.right(), triangle_origin.y()));
    path.lineTo(triangle_origin + QPointF(triangle_width / 2.0, triangle_height));
    path.lineTo(triangle_origin);

    p.fillPath(path, QGuiApplication::palette().color(QPalette::ColorRole::Text));
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
