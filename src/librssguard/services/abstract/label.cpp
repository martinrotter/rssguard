// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/label.h"

#include <QPainter>
#include <QPainterPath>

Label::Label(const QString& name, const QColor& color, RootItem* parent_item) : RootItem(parent_item) {
  setColor(color);
  setTitle(name);
}

Label::Label(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Label);
}

QColor Label::color() const {
  return m_color;
}

void Label::setColor(const QColor& color) {
  setIcon(generateIcon(color));
  m_color = color;
}

QIcon Label::generateIcon(const QColor& color) {
  QPixmap pxm(64, 64);

  pxm.fill(Qt::GlobalColor::transparent);

  QPainter paint(&pxm);
  QPainterPath path;

  path.addRoundedRect(QRectF(pxm.rect()), 10, 10);
  paint.fillPath(path, color);

  return pxm;
}
