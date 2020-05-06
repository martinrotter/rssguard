// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/styleditemdelegatewithoutfocus.h"

StyledItemDelegateWithoutFocus::StyledItemDelegateWithoutFocus(QObject* parent) : QStyledItemDelegate(parent) {}

void StyledItemDelegateWithoutFocus::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const {
  QStyleOptionViewItem itemOption(option);

  if ((itemOption.state & QStyle::StateFlag::State_HasFocus) == QStyle::StateFlag::State_HasFocus) {
    itemOption.state = itemOption.state ^ QStyle::StateFlag::State_HasFocus;
  }

  QStyledItemDelegate::paint(painter, itemOption, index);
}
