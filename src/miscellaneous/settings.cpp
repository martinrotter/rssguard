// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "miscellaneous/settings.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"

#include <QDebug>
#include <QDir>
#include <QPointer>
#include <QWebSettings>
#include <QLocale>


// Feeds.
DKEY Feeds::ID                            = "feeds";

DKEY Feeds::UpdateTimeout                 = "feed_update_timeout";
DVALUE(int) Feeds::UpdateTimeoutDef       = DOWNLOAD_TIMEOUT;

DKEY Feeds::CountFormat                   = "count_format";
DVALUE(char*) Feeds::CountFormatDef       = "(%unread)";

DKEY Feeds::AutoUpdateInterval            = "auto_update_interval";
DVALUE(int) Feeds::AutoUpdateIntervalDef  = DEFAULT_AUTO_UPDATE_INTERVAL;

DKEY Feeds::AutoUpdateEnabled             = "auto_update_enabled";
DVALUE(bool) Feeds::AutoUpdateEnabledDef  = false;

DKEY Feeds::FeedsUpdateOnStartup            = "feeds_update_on_startup";
DVALUE(bool) Feeds::FeedsUpdateOnStartupDef = false;

// Messages.
DKEY Messages::ID                            = "messages";

DKEY Messages::UseCustomDate                 = "use_custom_date";
DVALUE(bool) Messages::UseCustomDateDef      = false;

DKEY Messages::CustomDateFormat              = "custom_date_format";
DVALUE(char*) Messages::CustomDateFormatDef  = "";

DKEY Messages::ClearReadOnExit               = "clear_read_on_exit";
DVALUE(bool) Messages::ClearReadOnExitDef    = false;

DKEY Messages::KeepCursorInCenter               = "keep_cursor_center";
DVALUE(bool) Messages::KeepCursorInCenterDef    = false;

DKEY Messages::RemoveDuplicates               = "remove_duplicates";
DVALUE(bool) Messages::RemoveDuplicatesDef    = false;

// GUI.
DKEY GUI::ID                                      = "gui";

DKEY GUI::SplitterFeeds                           = "splitter_feeds";
DVALUE(char*) GUI::SplitterFeedsDef               = "";

DKEY GUI::SplitterMessages                        = "splitter_messages";
DVALUE(char*) GUI::SplitterMessagesDef            = "";

DKEY GUI::ToolbarStyle                            = "toolbar_style";
DVALUE(Qt::ToolButtonStyle) GUI::ToolbarStyleDef  = Qt::ToolButtonIconOnly;

DKEY GUI::FeedsToolbarActions                     = "feeds_toolbar";
DVALUE(char*) GUI::FeedsToolbarActionsDef         = "m_actionUpdateAllFeeds,m_actionMarkAllFeedsRead";

DKEY GUI::MainWindowInitialSize        = "window_size";
DKEY GUI::MainWindowInitialPosition    = "window_position";

DKEY GUI::MainWindowStartsFullscreen             = "start_in_fullscreen";
DVALUE(bool) GUI::MainWindowStartsFullscreenDef  = false;

DKEY GUI::MainWindowStartsHidden             = "start_hidden";
DVALUE(bool) GUI::MainWindowStartsHiddenDef  = false;

DKEY GUI::MainWindowStartsMaximized             = "window_is_maximized";
DVALUE(bool) GUI::MainWindowStartsMaximizedDef  = false;

DKEY GUI::MainMenuVisible                 = "main_menu_visible";
DVALUE(bool) GUI::MainMenuVisibleDef      = true;

DKEY GUI::ToolbarsVisible                 = "enable_toolbars";
DVALUE(bool) GUI::ToolbarsVisibleDef      = true;

DKEY GUI::ListHeadersVisible              = "enable_list_headers";
DVALUE(bool) GUI::ListHeadersVisibleDef   = true;

DKEY GUI::HideMainWindowWhenMinimized             = "hide_when_minimized";
DVALUE(bool) GUI::HideMainWindowWhenMinimizedDef  = false;

DKEY GUI::UseTrayIcon            = "use_tray_icon";
DVALUE(bool) GUI::UseTrayIconDef = true;

DKEY GUI::TabCloseMiddleClick                  = "tab_close_mid_button";
DVALUE(bool) GUI::TabCloseMiddleClickDef       = true;

DKEY GUI::TabCloseDoubleClick                  = "tab_close_double_button";
DVALUE(bool) GUI::TabCloseDoubleClickDef       = true;

DKEY GUI::TabNewDoubleClick                    = "tab_new_double_button";
DVALUE(bool) GUI::TabNewDoubleClickDef         = true;

DKEY GUI::HideTabBarIfOnlyOneTab               = "hide_tabbar_one_tab";
DVALUE(bool) GUI::HideTabBarIfOnlyOneTabDef    = true;

DKEY GUI::MessagesToolbarDefaultButtons             = "messages_toolbar";
DVALUE(char*) GUI::MessagesToolbarDefaultButtonsDef = "m_actionMarkSelectedMessagesAsRead,m_actionMarkSelectedMessagesAsUnread,m_actionSwitchImportanceOfSelectedMessages,separator,highlighter,spacer,search";

DKEY GUI::DefaultSortColumnMessages                = "default_sort_column_messages";
DVALUE(int) GUI::DefaultSortColumnMessagesDef      = MSG_DB_DCREATED_INDEX;

DKEY GUI::DefaultSortOrderMessages                       = "default_sort_order_messages";
DVALUE(Qt::SortOrder) GUI::DefaultSortOrderMessagesDef   = Qt::DescendingOrder;

DKEY GUI::DefaultSortColumnFeeds                     = "default_sort_column_feeds";
DVALUE(int) GUI::DefaultSortColumnFeedsDef           = FDS_MODEL_TITLE_INDEX;

DKEY GUI::DefaultSortOrderFeeds                      = "default_sort_order_feeds";
DVALUE(Qt::SortOrder) GUI::DefaultSortOrderFeedsDef  = Qt::AscendingOrder;

DKEY GUI::IconTheme              = "icon_theme";
DVALUE(char*) GUI::IconThemeDef  = APP_THEME_DEFAULT;

DKEY GUI::Skin              = "skin";
DVALUE(char*) GUI::SkinDef  = APP_SKIN_DEFAULT;

// General.
DKEY General::ID                              = "main";

DKEY General::UpdateOnStartup                 = "update_on_start";
DVALUE(bool) General::UpdateOnStartupDef      = true;

DKEY General::RemoveTrolltechJunk             = "remove_trolltech_junk";
DVALUE(bool) General::RemoveTrolltechJunkDef  = false;

DKEY General::Language               = "language";
DVALUE(QString) General::LanguageDef = QLocale::system().name();

// Downloads.
DKEY Downloads::ID                                    = "download_manager";

DKEY Downloads::AlwaysPromptForFilename               = "prompt_for_filename";
DVALUE(bool) Downloads::AlwaysPromptForFilenameDef    = false;

DKEY Downloads::TargetDirectory                       = "target_directory";
DVALUE(QString) Downloads::TargetDirectoryDef         = IOFactory::getSystemFolder(SYSTEM_FOLDER_ENUM::DesktopLocation);

DKEY Downloads::RemovePolicy             = "remove_policy";
DVALUE(int) Downloads::RemovePolicyDef   = DownloadManager::Never;

DKEY Downloads::ItemUrl                 = "download_%1_url";
DKEY Downloads::ItemLocation            = "download_%1_location";
DKEY Downloads::ItemDone                = "download_%1_done";

// Proxy.
DKEY Proxy::ID                              = "proxy";

DKEY Proxy::Type                                 = "proxy_type";
DVALUE(QNetworkProxy::ProxyType) Proxy::TypeDef  = QNetworkProxy::NoProxy;

DKEY Proxy::Host                    = "host";
DVALUE(char*) Proxy::HostDef        = "";

DKEY Proxy::Username                = "username";
DVALUE(char*) Proxy::UsernameDef    = "";

DKEY Proxy::Password                = "password";
DVALUE(char*) Proxy::PasswordDef    = "";

DKEY Proxy::Port                    = "port";
DVALUE(int) Proxy::PortDef          = 80;

// Database.
DKEY Database::ID                       = "database";

DKEY Database::UseInMemory              = "use_in_memory_db";
DVALUE(bool) Database::UseInMemoryDef   = false;

DKEY Database::MySQLHostname              = "mysql_hostname";
DVALUE(char*) Database::MySQLHostnameDef  = "";

DKEY Database::MySQLUsername              = "mysql_username";
DVALUE(char*) Database::MySQLUsernameDef  = "";

DKEY Database::MySQLPassword              = "mysql_password";
DVALUE(char*) Database::MySQLPasswordDef  = "";

DKEY Database::MySQLPort                  = "mysql_port";
DVALUE(int) Database::MySQLPortDef        = APP_DB_MYSQL_PORT;

DKEY Database::ActiveDriver               = "database_driver";
DVALUE(char*) Database::ActiveDriverDef   = APP_DB_SQLITE_DRIVER;

// Keyboard.
DKEY Keyboard::ID                           = "keyboard";

// Web browser.
DKEY Browser::ID                            = "browser";

DKEY Browser::GesturesEnabled               = "gestures_enabled";
DVALUE(bool) Browser::GesturesEnabledDef    = true;

DKEY Browser::JavascriptEnabled             = "enable_javascript";
DVALUE(bool) Browser::JavascriptEnabledDef  = true;

DKEY Browser::ImagesEnabled                 = "enable_images";
DVALUE(bool) Browser::ImagesEnabledDef      = true;

DKEY Browser::PluginsEnabled                = "enable_plugins";
DVALUE(bool) Browser::PluginsEnabledDef     = false;

DKEY Browser::CustomExternalBrowserEnabled                = "custom_external_browser";
DVALUE(bool) Browser::CustomExternalBrowserEnabledDef     = false;

DKEY Browser::CustomExternalBrowserExecutable             = "external_browser_executable";
DVALUE(char*) Browser::CustomExternalBrowserExecutableDef = "";

DKEY Browser::CustomExternalBrowserArguments              = "external_browser_arguments";
DVALUE(char*) Browser::CustomExternalBrowserArgumentsDef  = "%1";

DKEY Browser::QueueTabs             = "queue_tabs";
DVALUE(bool) Browser::QueueTabsDef  = true;

// Categories.
DKEY Categories::ID                         = "categories_expand_states";

Settings::Settings(const QString &file_name, Format format,
                   const Type &status, QObject *parent)
  : QSettings(file_name, format, parent), m_initializationStatus(status) {
}

Settings::~Settings() {  
  checkSettings();
  qDebug("Deleting Settings instance.");
}

QSettings::Status Settings::checkSettings() {
  qDebug("Syncing settings.");

  sync();
  return status();
}

bool Settings::initiateRestoration(const QString &settings_backup_file_path) {
  return IOFactory::copyFile(settings_backup_file_path,
                             QFileInfo(fileName()).absolutePath() + QDir::separator() +
                             BACKUP_NAME_SETTINGS + BACKUP_SUFFIX_SETTINGS);
}

void Settings::finishRestoration(const QString &desired_settings_file_path) {
  QString backup_settings_file = QFileInfo(desired_settings_file_path).absolutePath() + QDir::separator() +
                                 BACKUP_NAME_SETTINGS + BACKUP_SUFFIX_SETTINGS;

  if (QFile::exists(backup_settings_file)) {
    qWarning("Backup settings file '%s' was detected. Restoring it.", qPrintable(QDir::toNativeSeparators(backup_settings_file)));

    if (IOFactory::copyFile(backup_settings_file, desired_settings_file_path)) {
      QFile::remove(backup_settings_file);
      qDebug("Settings file was restored successully.");
    }
    else {
      qCritical("Settings file was NOT restored due to error when copying the file.");
    }
  }
}

Settings *Settings::setupSettings(QObject *parent) {
  Settings *new_settings;

  // If settings file exists in executable file working directory
  // (in subdirectory APP_CFG_PATH), then use it (portable settings).
  // Otherwise use settings file stored in home path.
  QString relative_path = QDir::separator() + QString(APP_CFG_PATH) + QDir::separator() + QString(APP_CFG_FILE);
  QString app_path = qApp->applicationDirPath();
  QString app_path_file = app_path + relative_path;
  QString home_path = qApp->homeFolderPath() + QDir::separator() + QString(APP_LOW_H_NAME);
  QString home_path_file = home_path + relative_path;

  bool portable_settings_available = QFileInfo(app_path).isWritable();
  bool non_portable_settings_exist = QFile::exists(home_path_file);

  // We will use PORTABLE settings only and only if it is available and NON-PORTABLE
  // settings was not initialized before.
  bool will_we_use_portable_settings = portable_settings_available && !non_portable_settings_exist;

  // Check if portable settings are available.
  if (will_we_use_portable_settings) {
    finishRestoration(app_path_file);

    // Portable settings are available, use them.
    new_settings = new Settings(app_path_file, QSettings::IniFormat, Settings::Portable, parent);

    // Construct icon cache in the same path.
    QString web_path = app_path + QDir::separator() + QString(APP_DB_WEB_PATH);
    QDir(web_path).mkpath(web_path);
    QWebSettings::setIconDatabasePath(web_path);

    qDebug("Initializing settings in '%s' (portable way).", qPrintable(QDir::toNativeSeparators(app_path_file)));
  }
  else {
    finishRestoration(home_path_file);

    // Portable settings are NOT available, store them in
    // user's home directory.
    new_settings = new Settings(home_path_file, QSettings::IniFormat, Settings::NonPortable, parent);

    // Construct icon cache in the same path.
    QString web_path = home_path + QDir::separator() + QString(APP_DB_WEB_PATH);
    QDir(web_path).mkpath(web_path);
    QWebSettings::setIconDatabasePath(web_path);

    qDebug("Initializing settings in '%s' (non-portable way).", qPrintable(QDir::toNativeSeparators(home_path_file)));
  }

  return new_settings;
}
