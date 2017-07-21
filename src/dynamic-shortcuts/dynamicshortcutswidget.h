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

#ifndef DYNAMICSHORTCUTSWIDGET_H
#define DYNAMICSHORTCUTSWIDGET_H

#include <QWidget>


class QGridLayout;
class ShortcutCatcher;

typedef QPair<QAction*, ShortcutCatcher*> ActionBinding;

class DynamicShortcutsWidget : public QWidget {
		Q_OBJECT

	public:
		// Constructors and destructors.
		explicit DynamicShortcutsWidget(QWidget* parent = 0);
		virtual ~DynamicShortcutsWidget();

		// Updates shortcuts of all actions according to changes.
		// NOTE: No access to settings is done here.
		// Shortcuts are fetched from settings when applications starts
		// and stored back to settings when application quits.
		void updateShortcuts();

		// Returns true if all shortcuts are unique,
		// otherwise false.
		bool areShortcutsUnique() const;

		// Populates this widget with shortcut widgets for given actions.
		// NOTE: This gets initial shortcut for each action from its properties, NOT from
		// the application settings, so shortcuts from settings need to be
		// assigned to actions before calling this method.
		void populate(QList<QAction*> actions);

	signals:
		void setupChanged();

	private:
		static bool lessThan(QAction* lhs, QAction* rhs);

	private:
		QGridLayout* m_layout;
		QList<ActionBinding> m_actionBindings;
};

#endif // DYNAMICSHORTCUTSOVERVIEW_H
