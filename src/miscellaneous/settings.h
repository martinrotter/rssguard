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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

#include "definitions/definitions.h"

#include <QPointer>

#define KEY static const char*
#define VALUE(x) static const x
#define SETTING(x) x, x##Def
#define GROUP(x) x::ID

// Feeds.
namespace Feeds {
  KEY ID                            = "feeds";

  KEY UpdateTimeout                 = "feed_update_timeout";
  VALUE(int) UpdateTimeoutDef       = DOWNLOAD_TIMEOUT;

  KEY CountFormat                   = "count_format";
  VALUE(char*) CountFormatDef       = "(%unread)";

  KEY AutoUpdateInterval            = "auto_update_interval";
  VALUE(int) AutoUpdateIntervalDef  = DEFAULT_AUTO_UPDATE_INTERVAL;

  KEY AutoUpdateEnabled             = "auto_update_enabled";
  VALUE(bool) AutoUpdateEnabledDef  = false;

  KEY FeedsUpdateOnStartup            = "feeds_update_on_startup";
  VALUE(bool) FeedsUpdateOnStartupDef = false;
}

// Messages.
namespace Messages {
  KEY ID                            = "messages";

  KEY UseCustomDate                 = "use_custom_date";
  VALUE(bool) UseCustomDateDef      = false;

  KEY CustomDateFormat              = "custom_date_format";
  VALUE(char*) CustomDateFormatDef  = "";

  KEY ClearReadOnExit               = "clear_read_on_exit";
  VALUE(bool) ClearReadOnExitDef    = false;
}

// GUI.
namespace GUI {
  KEY ID                                      = "gui";

  KEY SplitterFeeds                           = "splitter_feeds";
  VALUE(char*) SplitterFeedsDef               = "";

  KEY SplitterMessages                        = "splitter_messages";
  VALUE(char*) SplitterMessagesDef            = "";

  KEY ToolbarStyle                            = "toolbar_style";
  VALUE(Qt::ToolButtonStyle) ToolbarStyleDef  = Qt::ToolButtonIconOnly;

  KEY FeedsToolbarActions                     = "feeds_toolbar";
  VALUE(char*) FeedsToolbarActionsDef         = "m_actionUpdateAllFeeds,m_actionMarkAllFeedsRead";
}

// General.
namespace General {
  KEY ID                            = "main";
}

// Proxy.
namespace Proxy {
  KEY ID                            = "proxy";
}

// Database.
namespace Database {
  KEY ID                            = "database";
}

// Keyboard.
namespace Keyboard {
  KEY ID                            = "keyboard";
}

// Web browser.
namespace Browser {
  KEY ID                            = "browser";
}

// Categories.
namespace Categories {
  KEY ID                            = "categories_expand_states";
}


class Settings : public QSettings {
    Q_OBJECT

  public:
    // Describes possible types of loaded settings.
    enum Type {
      Portable,
      NonPortable
    };

    // Destructor.
    virtual ~Settings();

    // Type of used settings.
    inline Type type() const {
      return m_initializationStatus;
    }

    // Getter/setter for settings values.
    inline QVariant value(const QString &section, const QString &key, const QVariant &default_value = QVariant()) {
      return QSettings::value(QString("%1/%2").arg(section, key), default_value);
    }

    inline void setValue(const QString &section,  const QString &key, const QVariant &value) {
      QSettings::setValue(QString("%1/%2").arg(section, key), value);
    }

    // Synchronizes settings.
    QSettings::Status checkSettings();

    bool initiateRestoration(const QString &settings_backup_file_path);
    static void finishRestoration(const QString &desired_settings_file_path);

    // Creates settings file in correct location.
    static Settings *setupSettings(QObject *parent);

  private:
    // Constructor.
    explicit Settings(const QString &file_name, Format format, const Type &type, QObject *parent = 0);

    Type m_initializationStatus;
};

#endif // SETTINGS_H
