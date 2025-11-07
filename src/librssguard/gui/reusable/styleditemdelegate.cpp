// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/styleditemdelegate.h"

#include "definitions/definitions.h"
#include "definitions/globals.h"

#include <QPainter>
#include <QPropertyAnimation>
#include <QTreeView>

StyledItemDelegate::StyledItemDelegate(int height_row, int padding_row, QObject* parent)
  : QStyledItemDelegate(parent), m_flashProgress(0.0), m_rowHeight(height_row), m_rowPadding(padding_row) {}

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

  if (m_index.isValid() && index.parent() == m_index.parent() && index.row() == m_index.row() &&
      m_flashProgress >= 0.0) {
    qDebugNN << "paint " << m_flashProgress;

    const QTreeView* tree = qobject_cast<const QTreeView*>(option.widget);
    QRect rowRect(0, option.rect.top(), tree->viewport()->width(), option.rect.height());

    QColor c = QColor(255, 30, 30, static_cast<int>(180 * m_flashProgress));
    painter->save();
    painter->fillRect(rowRect, c);
    painter->restore();
  }
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

void StyledItemDelegate::flashItem(const QModelIndex& index, QTreeView* view) {
  m_index = index;

  QPropertyAnimation* anim = new QPropertyAnimation(this, "flashProgress");

  anim->setStartValue(1.0);
  anim->setEndValue(0.0);
  anim->setDuration(3000);
  anim->setEasingCurve(QEasingCurve::Type::OutCubic);

  connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
  connect(anim, &QPropertyAnimation::finished, this, [view, this]() {
    view->viewport()->update(rowRectForIndex(view, m_index));
    m_index = QModelIndex();
  });
  connect(anim, &QPropertyAnimation::valueChanged, view, [view, this]() {
    if (m_index.isValid()) {
      view->viewport()->update(rowRectForIndex(view, m_index));
    }
  });

  anim->start(QAbstractAnimation::DeletionPolicy::DeleteWhenStopped);
}

qreal StyledItemDelegate::flashProgress() const {
  return m_flashProgress;
}

void StyledItemDelegate::setFlashProgress(qreal v) {
  m_flashProgress = v;
  emit flashProgressChanged();
}

QRect StyledItemDelegate::rowRectForIndex(QTreeView* view, const QModelIndex& idx) const {
  const QRect cell_rect = view->visualRect(idx);

  if (!cell_rect.isValid()) {
    return QRect();
  }

  QRect row_rect(0, cell_rect.top(), view->viewport()->width(), cell_rect.height());
  return row_rect;
}
