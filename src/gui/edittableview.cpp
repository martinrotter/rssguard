// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "edittableview.h"

#include <qevent.h>

EditTableView::EditTableView(QWidget *parent)
  : QTableView(parent)
{
}

void EditTableView::keyPressEvent(QKeyEvent *event)
{
  if (model() && event->key() == Qt::Key_Delete) {
    removeSelected();
    event->setAccepted(true);
  } else {
    QAbstractItemView::keyPressEvent(event);
  }
}

void EditTableView::removeSelected()
{
  if (!model() || !selectionModel() || !selectionModel()->hasSelection())
    return;

  QModelIndexList selectedRows = selectionModel()->selectedRows();
  if (selectedRows.isEmpty())
    return;

  int newSelectedRow = selectedRows.at(0).row();
  for (int i = selectedRows.count() - 1; i >= 0; --i) {
    QModelIndex idx = selectedRows.at(i);
    model()->removeRow(idx.row(), rootIndex());
  }

  // select the item at the same position
  QModelIndex newSelectedIndex = model()->index(newSelectedRow, 0, rootIndex());
  // if that was the last item
  if (!newSelectedIndex.isValid())
    newSelectedIndex = model()->index(newSelectedRow - 1, 0, rootIndex());

  selectionModel()->select(newSelectedIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
  setCurrentIndex(newSelectedIndex);
}

void EditTableView::removeAll()
{
  if (!model())
    return;

  model()->removeRows(0, model()->rowCount(rootIndex()), rootIndex());
}

