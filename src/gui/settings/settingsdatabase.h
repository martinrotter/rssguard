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

#ifndef SETTINGSDATABASE_H
#define SETTINGSDATABASE_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsdatabase.h"


class SettingsDatabase : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsDatabase(Settings *settings, QWidget *parent = 0);
    virtual ~SettingsDatabase();

    inline QString title() const {
      return tr("Data storage");
    }

    void loadSettings();
    void saveSettings();

  private:
    void mysqlTestConnection();
    void onMysqlHostnameChanged(const QString &new_hostname);
    void onMysqlUsernameChanged(const QString &new_username);
    void onMysqlPasswordChanged(const QString &new_password);
    void onMysqlDatabaseChanged(const QString &new_database);
    void selectSqlBackend(int index);
    void switchMysqlPasswordVisiblity(bool visible);

    Ui::SettingsDatabase *m_ui;
};

#endif // SETTINGSDATABASE_H
