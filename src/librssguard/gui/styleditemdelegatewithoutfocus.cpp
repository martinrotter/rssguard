// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/styleditemdelegatewithoutfocus.h"

StyledItemDelegateWithoutFocus::StyledItemDelegateWithoutFocus(QObject* parent) : QStyledItemDelegate(parent) {}

StyledItemDelegateWithoutFocus::~StyledItemDelegateWithoutFocus() {}

void StyledItemDelegateWithoutFocus::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const {
  QStyleOptionViewItem itemOption(option);

  if (itemOption.state & QStyle::State_HasFocus) {
    itemOption.state = itemOption.state ^ QStyle::State_HasFocus;
  }

  QStyledItemDelegate::paint(painter, itemOption, index);
}
