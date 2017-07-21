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

#include "gui/treeviewcolumnsmenu.h"

#include <QHeaderView>


TreeViewColumnsMenu::TreeViewColumnsMenu(QHeaderView* parent) : QMenu(parent) {
	connect(this, &TreeViewColumnsMenu::aboutToShow, this, &TreeViewColumnsMenu::prepareMenu);
}

TreeViewColumnsMenu::~TreeViewColumnsMenu() {
}

void TreeViewColumnsMenu::prepareMenu() {
	QHeaderView* header_view = header();

	for (int i = 0; i < header_view->count(); i++) {
		QAction* act = addAction(header_view->model()->headerData(i, Qt::Horizontal, Qt::EditRole).toString());
		act->setData(i);
		act->setCheckable(true);
		act->setChecked(!header_view->isSectionHidden(i));
		connect(act, &QAction::toggled, this, &TreeViewColumnsMenu::actionTriggered);
	}
}

void TreeViewColumnsMenu::actionTriggered(bool toggle) {
	Q_UNUSED(toggle)
	QAction* send_act = qobject_cast<QAction*>(sender());
	header()->setSectionHidden(send_act->data().toInt(), !send_act->isChecked());
}

QHeaderView* TreeViewColumnsMenu::header() {
	return qobject_cast<QHeaderView*>(parent());
}
