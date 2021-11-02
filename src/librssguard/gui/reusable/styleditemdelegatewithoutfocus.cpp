// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/styleditemdelegatewithoutfocus.h"

#include "miscellaneous/application.h"

StyledItemDelegateWithoutFocus::StyledItemDelegateWithoutFocus(const QString& row_height_settings_key, QObject* parent)
  : QStyledItemDelegate(parent), m_rowHeightSettingsKey(row_height_settings_key) {}

void StyledItemDelegateWithoutFocus::paint(QPainter* painter,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const {
  QStyleOptionViewItem itemOption(option);

  if ((itemOption.state & QStyle::StateFlag::State_HasFocus) == QStyle::StateFlag::State_HasFocus) {
    itemOption.state = itemOption.state ^ QStyle::StateFlag::State_HasFocus;
  }

  QStyledItemDelegate::paint(painter, itemOption, index);
}

QSize StyledItemDelegateWithoutFocus::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
  auto row_height = qApp->settings()->value(GROUP(GUI), m_rowHeightSettingsKey).toInt();
  auto original_hint = QStyledItemDelegate::sizeHint(option, index);

  if (row_height <= 0) {
    return original_hint;
  }
  else {
    return QSize(original_hint.width(), row_height);
  }
}
