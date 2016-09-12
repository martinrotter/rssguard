// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef SETTINGSGUI_H
#define SETTINGSGUI_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsgui.h"


class SettingsGui : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsGui(Settings *settings, QWidget *parent = 0);
    virtual ~SettingsGui();

    inline QString title() const {
      return tr("User interface");
    }

    void loadSettings();
    void saveSettings();

  protected:
    // Does check of controls before dialog can be submitted.
    bool eventFilter(QObject *obj, QEvent *e);

  private:
    Ui::SettingsGui *m_ui;
};

#endif // SETTINGSGUI_H
