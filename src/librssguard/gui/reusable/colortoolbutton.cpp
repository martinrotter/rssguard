// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/colortoolbutton.h"

#include "definitions/definitions.h"

#include <QColorDialog>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

ColorToolButton::ColorToolButton(QWidget* parent) : QToolButton(parent), m_color(Qt::GlobalColor::black) {
  setToolTip(tr("Click me to change color!"));

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
  bool changed = m_color != color;

  m_color = color;

  if (changed) {
    emit colorChanged(m_color);
  }

  repaint();
}

void ColorToolButton::setRandomColor() {
  auto rnd_color = QRandomGenerator::global()->bounded(0xFFFFFF);
  auto rnd_color_name = QSL("#%1").arg(QString::number(rnd_color, 16));

  setColor(rnd_color_name);
  emit colorChanged(rnd_color_name);
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
