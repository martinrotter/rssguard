// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/plaintoolbutton.h"

#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"

#include <QAction>
#include <QColor>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStyle>
#include <QStyleOption>
#include <QToolButton>
#include <QtGlobal>

PlainToolButton::PlainToolButton(QWidget* parent)
  : QToolButton(parent), m_padding(0), m_attentionBorderVisible(false) {}

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

  if (m_attentionBorderVisible) {
    QColor badge_color = qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError).value<QColor>();

    if (!badge_color.isValid()) {
      badge_color = QColor(220, 35, 35);
    }

    badge_color.setAlpha(215);

    const qreal badge_size = qMax<qreal>(6.0, qMin(width(), height()) * 0.27);
    const qreal badge_margin = qMax<qreal>(2.0, badge_size * 0.35);
    const QRectF badge_rect(badge_margin, badge_margin, badge_size, badge_size);

    p.setOpacity(1.0);
    p.setPen(QPen(QColor(255, 255, 255, 190), 1.0));
    p.setBrush(badge_color);
    p.drawEllipse(badge_rect);
  }
}

int PlainToolButton::padding() const {
  return m_padding;
}

void PlainToolButton::setPadding(int padding) {
  m_padding = padding;
  repaint();
}

bool PlainToolButton::attentionBorderVisible() const {
  return m_attentionBorderVisible;
}

void PlainToolButton::setAttentionBorderVisible(bool visible) {
  if (visible == m_attentionBorderVisible) {
    return;
  }

  m_attentionBorderVisible = visible;
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
