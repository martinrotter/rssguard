// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/styleditemdelegatewithoutfocus.h"

#include "definitions/definitions.h"

StyledItemDelegateWithoutFocus::StyledItemDelegateWithoutFocus(int height_row, int padding_row, QObject* parent)
  : QStyledItemDelegate(parent), m_rowHeight(height_row), m_rowPadding(padding_row) {}

void StyledItemDelegateWithoutFocus::paint(QPainter* painter,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const {
  QStyleOptionViewItem item_option(option);

  if ((item_option.state & QStyle::StateFlag::State_HasFocus) == QStyle::StateFlag::State_HasFocus) {
    item_option.state = item_option.state ^ QStyle::StateFlag::State_HasFocus;
  }

  bool rtl = index.data(TEXT_DIRECTION_ROLE).value<Qt::LayoutDirection>() == Qt::LayoutDirection::RightToLeft;

  if (rtl) {
    item_option.direction = Qt::LayoutDirection::RightToLeft;
  }

  if ((item_option.state & QStyle::StateFlag::State_Selected) == QStyle::StateFlag::State_Selected &&
      index.data(Qt::ItemDataRole::ForegroundRole).isValid()) {
    item_option.palette.setColor(QPalette::ColorRole::HighlightedText,
                                 index.data(HIGHLIGHTED_FOREGROUND_TITLE_ROLE).value<QColor>());
  }

  QStyledItemDelegate::paint(painter, item_option, index);
}

QSize StyledItemDelegateWithoutFocus::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  auto original_hint = QStyledItemDelegate::sizeHint(option, index);
  QSize new_hint;

  if (m_rowHeight <= 0) {
    new_hint = original_hint;
  }
  else {
    new_hint = QSize(original_hint.width(), m_rowHeight);
  }

  if (m_rowPadding > 0) {
    new_hint.setHeight(new_hint.height() + (2 * m_rowPadding));
  }

  return new_hint;
}
