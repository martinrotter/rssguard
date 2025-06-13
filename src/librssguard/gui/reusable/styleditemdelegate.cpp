// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/styleditemdelegate.h"

#include "definitions/definitions.h"
#include "definitions/globals.h"

StyledItemDelegate::StyledItemDelegate(int height_row, int padding_row, QObject* parent)
  : QStyledItemDelegate(parent), m_rowHeight(height_row), m_rowPadding(padding_row) {}

void StyledItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
  QStyleOptionViewItem item_option(option);

  // NOTE: Yes, display focus rectangles too, they are actually useful in some cases.
  /*
  if (Globals::hasFlag(item_option.state, QStyle::StateFlag::State_HasFocus)) {
    item_option.state = item_option.state ^ QStyle::StateFlag::State_HasFocus;
  }
  */

  bool rtl = index.data(TEXT_DIRECTION_ROLE).value<Qt::LayoutDirection>() == Qt::LayoutDirection::RightToLeft;

  if (rtl) {
    item_option.direction = Qt::LayoutDirection::RightToLeft;
  }

  if (Globals::hasFlag(item_option.state, QStyle::StateFlag::State_Selected) &&
      index.data(Qt::ItemDataRole::ForegroundRole).isValid()) {
    item_option.palette.setColor(QPalette::ColorRole::HighlightedText,
                                 index.data(HIGHLIGHTED_FOREGROUND_TITLE_ROLE).value<QColor>());
  }

  QStyledItemDelegate::paint(painter, item_option, index);
}

QSize StyledItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
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
