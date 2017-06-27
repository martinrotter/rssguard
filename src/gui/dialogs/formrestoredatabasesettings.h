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

#ifndef FORMRESTOREDATABASESETTINGS_H
#define FORMRESTOREDATABASESETTINGS_H

#include <QDialog>

#include "ui_formrestoredatabasesettings.h"


class FormRestoreDatabaseSettings : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormRestoreDatabaseSettings(QWidget *parent = 0);
    virtual ~FormRestoreDatabaseSettings();

    bool shouldRestart() const {
      return m_shouldRestart;
    }

  private slots:
    void performRestoration();
    void checkOkButton();
    void selectFolderWithGui();
    void selectFolder(QString folder = QString());

  private:
    QScopedPointer<Ui::FormRestoreDatabaseSettings> m_ui;
    QPushButton *m_btnRestart;

    bool m_shouldRestart;
};

#endif // FORMRESTOREDATABASESETTINGS_H
