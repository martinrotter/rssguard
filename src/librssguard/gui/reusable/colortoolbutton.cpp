// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/colortoolbutton.h"

#include "miscellaneous/textfactory.h"

#include <QColorDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

ColorToolButton::ColorToolButton(QWidget* parent) : QToolButton(parent), m_color(Qt::GlobalColor::black) {
  setToolTip(tr("Click me to change color!"));

  connect(this, &ColorToolButton::clicked, this, [this]() {
    auto new_color = QColorDialog::getColor(m_color,
                                            parentWidget(),
                                            tr("Select new color"),
                                            QColorDialog::ColorDialogOption::DontUseNativeDialog |
                                              QColorDialog::ColorDialogOption::ShowAlphaChannel);

    if (new_color.isValid()) {
      setColor(new_color);
    }
  });
}

QColor ColorToolButton::color() const {
  return m_color;
}

void ColorToolButton::setColor(const QColor& color, bool inform_about_changes) {
  m_color = color;
  repaint();

  if (inform_about_changes) {
    emit colorChanged(m_color);
  }
}

void ColorToolButton::setRandomColor() {
  auto clr = TextFactory::generateRandomColor();

  setColor(clr);
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

QColor ColorToolButton::alternateColor() const {
  return m_alternateColor;
}

void ColorToolButton::setAlternateColor(const QColor& alt_color) {
  m_alternateColor = alt_color;
}

void ColorToolButton::mouseReleaseEvent(QMouseEvent* event) {
  QToolButton::mouseReleaseEvent(event);

  if (event->button() == Qt::MouseButton::RightButton) {
    setColor(m_alternateColor);
  }
}
