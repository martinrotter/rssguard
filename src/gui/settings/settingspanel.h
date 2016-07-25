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


#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>


class Settings;

class SettingsPanel : public QWidget {
    Q_OBJECT

  public:
    explicit SettingsPanel(Settings *settings, QWidget *parent = 0);

    // Call this base implementation in the end of your subclass implementation.
    virtual void loadSettings();

    // Call this base implementation in the end of your subclass implementation.
    virtual void saveSettings();

  protected:
    // Sets this settings panel as dirty (some settings are changed) and emits the signal.
    void dirtifySettings();

    // These methods should not be probably called by subclasses.
    bool isDirty() const;
    void setIsDirty(bool is_dirty);

    // Settings to use to save/load.
    Settings *settings() const;

  signals:
    void settingsChanged();

  private:
    bool m_isDirty;
    Settings *m_settings;
};

#endif // SETTINGSPANEL_H
