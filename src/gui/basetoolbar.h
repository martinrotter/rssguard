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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>


class BaseBar {
  public:
    // Returns all actions which can be added to the toolbar.
    virtual QList<QAction*> availableActions() const = 0;

    // Returns all changeable actions which are currently included
    // in the toolbar.
    virtual QList<QAction*> changeableActions() const = 0;

    // Sets new "actions" to the toolbar and perhaps saves the toolbar
    // state into the settings.
    virtual void saveChangeableActions(const QStringList &actions) = 0;

    // Loads the toolbar state from settings.
    virtual void loadChangeableActions() = 0;

  protected:
    QAction *findMatchingAction(const QString &action, const QList<QAction*> actions) const;
};

class BaseToolBar : public QToolBar, public BaseBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseToolBar(const QString &title, QWidget *parent = 0);
    virtual ~BaseToolBar();
};

#endif // TOOLBAR_H
