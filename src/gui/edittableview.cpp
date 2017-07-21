// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/edittableview.h"

#include <QKeyEvent>


EditTableView::EditTableView(QWidget* parent) : QTableView(parent) {
}

void EditTableView::keyPressEvent(QKeyEvent* event) {
	if (model() && event->key() == Qt::Key_Delete) {
		removeSelected();
		event->accept();
	}

	else {
		QAbstractItemView::keyPressEvent(event);
	}
}

void EditTableView::removeSelected() {
	if (!model() || !selectionModel() || !selectionModel()->hasSelection()) {
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
