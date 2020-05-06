// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/edittableview.h"

#include <QKeyEvent>

EditTableView::EditTableView(QWidget* parent) : QTableView(parent) {}

void EditTableView::keyPressEvent(QKeyEvent* event) {
  if (model() != nullptr && event->key() == Qt::Key::Key_Delete) {
    removeSelected();
    event->accept();
  }
  else {
    QAbstractItemView::keyPressEvent(event);
  }
}

void EditTableView::removeSelected() {
  if (model() != nullptr || selectionModel() != nullptr || !selectionModel()->hasSelection()) {
    return;
  }

  const QModelIndexList selected_rows = selectionModel()->selectedRows();

  if (selected_rows.isEmpty()) {
    return;
  }

  const int new_selected_row = selected_rows.at(0).row();

  for (int i = selected_rows.count() - 1; i >= 0; i--) {
    QModelIndex idx = selected_rows.at(i);

    model()->removeRow(idx.row(), rootIndex());
  }

  QModelIndex new_selected_index = model()->index(new_selected_row, 0, rootIndex());

  if (!new_selected_index.isValid()) {
    new_selected_index = model()->index(new_selected_row - 1, 0, rootIndex());
  }

  selectionModel()->select(new_selected_index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
  setCurrentIndex(new_selected_index);
}

void EditTableView::removeAll() {
  if (model() != nullptr) {
    model()->removeRows(0, model()->rowCount(rootIndex()), rootIndex());
  }
}
