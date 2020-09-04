// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/label.h"

Label::Label(RootItem* parent_item) : RootItem(parent_item), m_name(QString()), m_backColor(QColor()) {
  setKind(RootItem::Kind::Labels);
}
