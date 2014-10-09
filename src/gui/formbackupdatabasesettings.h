// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FORMBACKUPDATABASECONFIG_H
#define FORMBACKUPDATABASECONFIG_H

#include <QDialog>

#include "ui_formbackupdatabasesettings.h"


namespace Ui {
  class FormBackupDatabaseSettings;
}

class FormBackupDatabaseSettings : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors
    explicit FormBackupDatabaseSettings(QWidget *parent = 0);
    virtual ~FormBackupDatabaseSettings();


  private slots:
    void performBackup();
    void selectFolder(QString path = QString());
    void checkBackupNames(const QString &name);
    void checkOkButton();

  private:
    Ui::FormBackupDatabaseSettings *m_ui;
};

#endif // FORMBACKUPDATABASECONFIG_H
