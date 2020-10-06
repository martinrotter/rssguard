// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/colortoolbutton.h"

#include <QColorDialog>
#include <QPainter>
#include <QPainterPath>

ColorToolButton::ColorToolButton(QWidget* parent) : QToolButton(parent), m_color(Qt::GlobalColor::black) {
  connect(this, &ColorToolButton::clicked, this, [this]() {
    auto new_color = QColorDialog::getColor(m_color, parentWidget(), tr("Select new color"),
                                            QColorDialog::ColorDialogOption::DontUseNativeDialog |
                                            QColorDialog::ColorDialogOption::ShowAlphaChannel);

    if (new_color.isValid()) {
      setColor(new_color);
      emit colorChanged(new_color);
    }
  });
}

QColor ColorToolButton::color() const {
  return m_color;
}

void ColorToolButton::setColor(const QColor& color) {
  m_color = color;

  repaint();
}

void ColorToolButton::paintEvent(QPaintEvent* e) {
  Q_UNUSED(e)
  QPainter p(this);
  QRect rect(QPoint(0, 0), size());

  if (isEnabled()) {
    if (underMouse() || isChecked()) {
      p.setOpacity(0.7);
    }
  }
  else {
    p.setOpacity(0.3);
  }

  QPainterPath path;

  path.addRoundedRect(QRectF(rect), 3, 3);

  p.fillPath(path, m_color);
}
