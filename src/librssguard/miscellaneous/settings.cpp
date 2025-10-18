// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/settings.h"

#include "gui/messagesview.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"

#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QPointer>

DKEY FileDialogPaths::ID = "file_dialog_paths";
DKEY DialogGeometries::ID = "dialog_geometries";

// Media player.
KEY VideoPlayer::ID = "media_player";

DKEY VideoPlayer::MpvUseCustomConfigFolder = "mpv_use_custom_config_folder";
DVALUE(bool) VideoPlayer::MpvUseCustomConfigFolderDef = true;

DKEY VideoPlayer::MpvCustomConfigFolder = "mpv_config_folder";
DVALUE(QString) VideoPlayer::MpvCustomConfigFolderDef = "%data%/mpv";

// Network.
DKEY Network::ID = "network";

DKEY Network::SendDNT = "send_dnt";
VALUE(bool) Network::SendDNTDef = false;

DKEY Network::EnableHttp2 = "http2_enabled";
DVALUE(bool) Network::EnableHttp2Def = false;

DKEY Network::CustomUserAgent = "user_agent";
DVALUE(QString) Network::CustomUserAgentDef = QString();

// Feeds.
DKEY Feeds::ID = "feeds";

DKEY Feeds::UpdateTimeout = "feed_update_timeout";
DVALUE(int) Feeds::UpdateTimeoutDef = DOWNLOAD_TIMEOUT;

DKEY Feeds::CountFormat = "count_format";
DVALUE(char*) Feeds::CountFormatDef = "%unread-%all";

DKEY Feeds::EnableTooltipsFeedsMessages = "show_tooltips";
DVALUE(bool) Feeds::EnableTooltipsFeedsMessagesDef = true;

DKEY Feeds::StrikethroughDisabledFeeds = "strikethrough_disabled_feeds";
DVALUE(bool) Feeds::StrikethroughDisabledFeedsDef = true;

DKEY Feeds::DontAskWhenMarkinAllRead = "dont_ask_when_marking_all_read";
DVALUE(bool) Feeds::DontAskWhenMarkinAllReadDef = false;

DKEY Feeds::PauseFeedFetching = "pause_feed_fetching";
DVALUE(bool) Feeds::PauseFeedFetchingDef = false;

DKEY Feeds::AutoUpdateInterval = "auto_update_interval";
DVALUE(int) Feeds::AutoUpdateIntervalDef = DEFAULT_AUTO_UPDATE_INTERVAL;

DKEY Feeds::AutoUpdateEnabled = "auto_update_enabled";
DVALUE(bool) Feeds::AutoUpdateEnabledDef = false;

DKEY Feeds::FastAutoUpdate = "auto_update_fast";
DVALUE(bool) Feeds::FastAutoUpdateDef = false;

DKEY Feeds::AutoUpdateOnlyUnfocused = "auto_update_only_unfocused";
DVALUE(bool) Feeds::AutoUpdateOnlyUnfocusedDef = false;

DKEY Feeds::FeedsUpdateOnStartup = "feeds_update_on_startup";
DVALUE(bool) Feeds::FeedsUpdateOnStartupDef = false;

DKEY Feeds::FeedsUpdateStartupDelay = "feeds_update_on_startup_delay";
DVALUE(double) Feeds::FeedsUpdateStartupDelayDef = STARTUP_UPDATE_DELAY;

DKEY Feeds::SortAlphabetically = "sort_alphabetically";
DVALUE(bool) Feeds::SortAlphabeticallyDef = false;

DKEY Feeds::ShowTreeBranches = "show_tree_branches";
DVALUE(bool) Feeds::ShowTreeBranchesDef = true;

DKEY Feeds::HideCountsIfNoUnread = "hide_counts_if_no_unread";
DVALUE(bool) Feeds::HideCountsIfNoUnreadDef = false;

DKEY Feeds::UpdateFeedListDuringFetching = "update_feed_list_during_fetching";
DVALUE(bool) Feeds::UpdateFeedListDuringFetchingDef = false;

DKEY Feeds::AutoExpandOnSelection = "auto_expand_on_selection";
DVALUE(bool) Feeds::AutoExpandOnSelectionDef = false;

DKEY Feeds::OnlyBasicShortcutsInLists = "only_basic_shortcuts_in_lists";
DVALUE(bool) Feeds::OnlyBasicShortcutsInListsDef = false;

DKEY Feeds::CustomizeListFont = "customize_list_font";
DVALUE(bool) Feeds::CustomizeListFontDef = false;

DKEY Feeds::ListFont = "list_font";

// Messages.
DKEY Messages::ID = "messages";

DKEY Messages::LimitArticleImagesHeight = "message_head_image_height";
DVALUE(int) Messages::LimitArticleImagesHeightDef = 72;

DKEY Messages::DisplayEnclosuresInMessage = "show_enclosures_in_message";
DVALUE(bool) Messages::DisplayEnclosuresInMessageDef = false;

DKEY Messages::AvoidOldArticles = "avoid_old_articles";
DVALUE(bool) Messages::AvoidOldArticlesDef = false;

DKEY Messages::FontAa = "font_aa";
DVALUE(bool) Messages::FontAaDef = true;

DKEY Messages::ShapeAa = "shape_aa";
DVALUE(bool) Messages::ShapeAaDef = true;

DKEY Messages::ArticleListLazyLoading = "article_list_lazy_loading";
DVALUE(bool) Messages::ArticleListLazyLoadingDef = false;

DKEY Messages::DateTimeToAvoidArticle = "datetime_to_avoid_article";
DVALUE(QDateTime) Messages::DateTimeToAvoidArticleDef = QDateTime::currentDateTime();

DKEY Messages::HoursToAvoidArticle = "hours_to_avoid_article";
DVALUE(int) Messages::HoursToAvoidArticleDef = 0;

DKEY Messages::LimitDoNotRemoveUnread = "limit_dont_remove_unread";
DVALUE(bool) Messages::LimitDoNotRemoveUnreadDef = true;

DKEY Messages::LimitDoNotRemoveStarred = "limit_dont_remove_starred";
DVALUE(bool) Messages::LimitDoNotRemoveStarredDef = true;

DKEY Messages::LimitRecycleInsteadOfPurging = "limit_recycle_dont_purge";
DVALUE(bool) Messages::LimitRecycleInsteadOfPurgingDef = false;

DKEY Messages::LimitCountOfArticles = "limit_count_of_articles";
DVALUE(int) Messages::LimitCountOfArticlesDef = 0;

DKEY Messages::AlwaysDisplayItemPreview = "always_display_preview";
DVALUE(bool) Messages::AlwaysDisplayItemPreviewDef = true;

DKEY Messages::EnableMessagePreview = "enable_message_preview";
DVALUE(bool) Messages::EnableMessagePreviewDef = true;

DKEY Messages::ShowResourcesInArticles = "enable_message_resources";
DVALUE(bool) Messages::ShowResourcesInArticlesDef = true;

DKEY Messages::Zoom = "zoom";
DVALUE(qreal) Messages::ZoomDef = double(1.0);

DKEY Messages::FixupFutureArticleDateTimes = "fixup_future_datetimes";
DVALUE(bool) Messages::FixupFutureArticleDateTimesDef = false;

DKEY Messages::UseCustomDate = "use_custom_date";
DVALUE(bool) Messages::UseCustomDateDef = false;

DKEY Messages::CustomDateFormat = "custom_date_format";
DVALUE(char*) Messages::CustomDateFormatDef = "";

DKEY Messages::CustomFormatForDatesOnly = "custom_date_format_for_dates_only";
DVALUE(char*) Messages::CustomFormatForDatesOnlyDef = "";

DKEY Messages::UseCustomFormatForDatesOnly = "use_custom_date_for_dates_only";
DVALUE(bool) Messages::UseCustomFormatForDatesOnlyDef = false;

DKEY Messages::RelativeTimeForNewerArticles = "relative_time_for_new_articles";
DVALUE(int) Messages::RelativeTimeForNewerArticlesDef = -1;

DKEY Messages::ArticleMarkOnSelection = "mark_message_on_selected";
DVALUE(int) Messages::ArticleMarkOnSelectionDef = int(MessagesView::ArticleMarkingPolicy::MarkImmediately);

DKEY Messages::ArticleMarkOnSelectionDelay = "mark_message_on_selected_delay";
DVALUE(int) Messages::ArticleMarkOnSelectionDelayDef = 3000;

DKEY Messages::ArticleListPadding = "article_list_padding";
DVALUE(int) Messages::ArticleListPaddingDef = -1;

DKEY Messages::MultilineArticleList = "multiline_article_list";
DVALUE(bool) Messages::MultilineArticleListDef = false;

DKEY Messages::SwitchArticleListRtl = "switch_article_list_rtl";
DVALUE(bool) Messages::SwitchArticleListRtlDef = true;

DKEY Messages::UseCustomTime = "use_custom_time";
DVALUE(bool) Messages::UseCustomTimeDef = false;

DKEY Messages::CustomTimeFormat = "custom_time_format";
DVALUE(QString) Messages::CustomTimeFormatDef = {};

DKEY Messages::ClearReadOnExit = "clear_read_on_exit";
DVALUE(bool) Messages::ClearReadOnExitDef = false;

DKEY Messages::IgnoreContentsChanges = "ignore_contents_changes";
DVALUE(bool) Messages::IgnoreContentsChangesDef = true;

DKEY Messages::MarkUnreadOnUpdated = "mark_unread_on_update";
DVALUE(bool) Messages::MarkUnreadOnUpdatedDef = false;

DKEY Messages::UnreadIconType = "unread_icons_in_message_list";
DVALUE(int) Messages::UnreadIconTypeDef = 1; /* MessagesModel::MessageUnreadIcon::Dot */

DKEY Messages::BringAppToFrontAfterMessageOpenedExternally = "bring_app_to_front_after_msg_opened";
DVALUE(bool) Messages::BringAppToFrontAfterMessageOpenedExternallyDef = false;

DKEY Messages::KeepCursorInCenter = "keep_cursor_center";
DVALUE(bool) Messages::KeepCursorInCenterDef = false;

DKEY Messages::ShowOnlyUnreadMessages = "show_only_unread_messages";
DVALUE(bool) Messages::ShowOnlyUnreadMessagesDef = false;

DKEY Messages::PreviewerFontStandard = "previewer_font_standard";
NON_CONST_DVALUE(QString) Messages::PreviewerFontStandardDef = QString();

DKEY Messages::CustomizeListFont = "customize_list_font";
DVALUE(bool) Messages::CustomizeListFontDef = false;

DKEY Messages::ListFont = "list_font";

// Custom skin colors.
DKEY CustomSkinColors::ID = "custom_skin_colors";

DKEY CustomSkinColors::Enabled = "enabled";
DVALUE(bool) CustomSkinColors::EnabledDef = false;

// GUI.
DKEY GUI::ID = "gui";

DKEY GUI::FeedViewState = "feed_view_state";
DVALUE(QString) GUI::FeedViewStateDef = QString();

DKEY GUI::MessageViewState = "msg_view_state";
DVALUE(QString) GUI::MessageViewStateDef = QString();

DKEY GUI::SplitterFeeds = "splitter_feeds";
DVALUE(QList<QVariant>) GUI::SplitterFeedsDef = {};

DKEY GUI::SplitterMessagesIsVertical = "splitter_messages_is_vertical";
DVALUE(bool) GUI::SplitterMessagesIsVerticalDef = true;

DKEY GUI::SplitterMessagesVertical = "splitter_messages_vertical";
DVALUE(QList<QVariant>) GUI::SplitterMessagesVerticalDef = {};

DKEY GUI::SplitterMessagesHorizontal = "splitter_messages_horizontal";
DVALUE(QList<QVariant>) GUI::SplitterMessagesHorizontalDef = {};

DKEY GUI::ToolbarIconSize = "toolbar_icon_size";
DVALUE(int) GUI::ToolbarIconSizeDef = 0;

DKEY GUI::ToolbarStyle = "toolbar_style";
DVALUE(Qt::ToolButtonStyle) GUI::ToolbarStyleDef = Qt::ToolButtonIconOnly;

DKEY GUI::HeightRowMessages = "height_row_messages";
DVALUE(int) GUI::HeightRowMessagesDef = -1;

DKEY GUI::HeightRowFeeds = "height_row_feeds";
DVALUE(int) GUI::HeightRowFeedsDef = -1;

DKEY GUI::FeedsToolbarActions = "feeds_toolbar";
DVALUE(char*)
GUI::FeedsToolbarActionsDef = "m_actionUpdateAllItems,m_actionStopRunningItemsUpdate,m_actionPauseFeedFetching,m_"
                              "actionMarkAllItemsRead,filter,spacer,search";

DKEY GUI::StatusbarActions = "status_bar";
DVALUE(char*)
GUI::StatusbarActionsDef = "m_barProgressDownloadAction,m_barProgressFeedsAction,m_actionUpdateAllItems,m_"
                           "actionUpdateSelectedItems,m_actionStopRunningItemsUpdate,m_actionFullscreen,m_actionQuit";

DKEY GUI::MainWindowInitialSize = "window_size";
DKEY GUI::MainWindowInitialPosition = "window_position";

DKEY GUI::IsMainWindowMaximizedBeforeFullscreen = "is_window_maximized_before_fullscreen";
DVALUE(bool) GUI::IsMainWindowMaximizedBeforeFullscreenDef = false;

DKEY GUI::MainWindowStartsFullscreen = "start_in_fullscreen";
DVALUE(bool) GUI::MainWindowStartsFullscreenDef = false;

DKEY GUI::MainWindowStartsHidden = "start_hidden";
DVALUE(bool) GUI::MainWindowStartsHiddenDef = false;

DKEY GUI::MainWindowStartsMaximized = "window_is_maximized";
DVALUE(bool) GUI::MainWindowStartsMaximizedDef = false;

DKEY GUI::AlternateRowColorsInLists = "alternate_colors_in_lists";
DVALUE(bool) GUI::AlternateRowColorsInListsDef = false;

DKEY GUI::MainMenuVisible = "main_menu_visible";
DVALUE(bool) GUI::MainMenuVisibleDef = true;

DKEY GUI::ToolbarsVisible = "enable_toolbars";
DVALUE(bool) GUI::ToolbarsVisibleDef = true;

DKEY GUI::ListHeadersVisible = "enable_list_headers";
DVALUE(bool) GUI::ListHeadersVisibleDef = true;

DKEY GUI::MessageViewerToolbarsVisible = "message_viewer_toolbars";
DVALUE(bool) GUI::MessageViewerToolbarsVisibleDef = true;

DKEY GUI::StatusBarVisible = "enable_status_bar";
DVALUE(bool) GUI::StatusBarVisibleDef = true;

DKEY GUI::EnableNotifications = "enable_notifications";
DVALUE(bool) GUI::EnableNotificationsDef = true;

DKEY GUI::UseToastNotifications = "use_toast_notifications";
DVALUE(bool) GUI::UseToastNotificationsDef = true;

DKEY GUI::ToastNotificationsPosition = "toast_notifications_position";
DVALUE(ToastNotificationsManager::NotificationPosition)
GUI::ToastNotificationsPositionDef = ToastNotificationsManager::NotificationPosition::BottomRight;

DKEY GUI::ToastNotificationsScreen = "toast_notifications_screen";
DVALUE(int) GUI::ToastNotificationsScreenDef = -1;

DKEY GUI::ToastNotificationsMargin = "toast_notifications_margin";
DVALUE(int) GUI::ToastNotificationsMarginDef = NOTIFICATIONS_MARGIN;

DKEY GUI::ToastNotificationsOpacity = "toast_notifications_opacity";
DVALUE(double) GUI::ToastNotificationsOpacityDef = 0.9;

DKEY GUI::ToastNotificationsWidth = "toast_notifications_width";
DVALUE(int) GUI::ToastNotificationsWidthDef = NOTIFICATIONS_WIDTH;

DKEY GUI::HideMainWindowWhenMinimized = "hide_when_minimized";
DVALUE(bool) GUI::HideMainWindowWhenMinimizedDef = false;

DKEY GUI::MonochromeTrayIcon = "monochrome_tray_icon";
DVALUE(bool) GUI::MonochromeTrayIconDef = false;

DKEY GUI::CustomizeAppFont = "custom_app_font";
DVALUE(bool) GUI::CustomizeAppFontDef = false;

DKEY GUI::AppFont = "app_font";

DKEY GUI::FontAntialiasing = "font_antialiasing";
DVALUE(bool) GUI::FontAntialiasingDef = true;

DKEY GUI::ColoredBusyTrayIcon = "colored_busy_tray_icon";
DVALUE(bool) GUI::ColoredBusyTrayIconDef = false;

DKEY GUI::ForcedSkinColors = "forced_skin_colors";
DVALUE(bool) GUI::ForcedSkinColorsDef = true;

DKEY GUI::UnreadNumbersInTrayIcon = "show_unread_numbers_in_tray_icon";
DVALUE(bool) GUI::UnreadNumbersInTrayIconDef = true;

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)) || defined(Q_OS_WIN)
DKEY GUI::UnreadNumbersOnTaskBar = "show_unread_numbers_on_task_bar";
DVALUE(bool) GUI::UnreadNumbersOnTaskBarDef = true;
#endif

DKEY GUI::UnreadNumbersOnWindow = "show_unread_numbers_on_window";
DVALUE(bool) GUI::UnreadNumbersOnWindowDef = true;

DKEY GUI::UseTrayIcon = "use_tray_icon";
DVALUE(bool) GUI::UseTrayIconDef = true;

DKEY GUI::TabCloseMiddleClick = "tab_close_mid_button";
DVALUE(bool) GUI::TabCloseMiddleClickDef = true;

DKEY GUI::TabCloseDoubleClick = "tab_close_double_button";
DVALUE(bool) GUI::TabCloseDoubleClickDef = true;

DKEY GUI::TabNewDoubleClick = "tab_new_double_button";
DVALUE(bool) GUI::TabNewDoubleClickDef = true;

DKEY GUI::HideTabBarIfOnlyOneTab = "hide_tabbar_one_tab";
DVALUE(bool) GUI::HideTabBarIfOnlyOneTabDef = false;

DKEY GUI::MessagesToolbarDefaultButtons = "messages_toolbar";
DVALUE(char*)
GUI::MessagesToolbarDefaultButtonsDef =
  "m_actionMarkSelectedMessagesAsRead,m_actionMarkSelectedMessagesAsUnread,m_actionSwitchImportanceOfSelectedMessages,"
  "separator,highlighter,filter,m_actionLoadAllArticles,spacer,search";

DKEY GUI::DefaultSortColumnFeeds = "default_sort_column_feeds";
DVALUE(int) GUI::DefaultSortColumnFeedsDef = FDS_MODEL_TITLE_INDEX;

DKEY GUI::DefaultSortOrderFeeds = "default_sort_order_feeds";
DVALUE(Qt::SortOrder) GUI::DefaultSortOrderFeedsDef = Qt::AscendingOrder;

DKEY GUI::IconTheme = "icon_theme_name";
DVALUE(char*) GUI::IconThemeDef = APP_ICON_THEME_DEFAULT;

DKEY GUI::Skin = "skin";
DVALUE(char*) GUI::SkinDef = APP_SKIN_DEFAULT;

DKEY GUI::Style = "style";
DVALUE(char*) GUI::StyleDef = APP_STYLE_DEFAULT;

// General.
DKEY General::ID = "main";

DKEY General::UpdateOnStartup = "update_on_start";
DVALUE(bool) General::UpdateOnStartupDef = false;

DKEY General::FirstRun = "first_run";
DVALUE(bool) General::FirstRunDef = true;

DKEY General::Language = "language";
DVALUE(QString) General::LanguageDef = QLocale::system().name();

// Proxy.
DKEY Proxy::ID = "proxy";

DKEY Proxy::Type = "proxy_type";
DVALUE(QNetworkProxy::ProxyType) Proxy::TypeDef = QNetworkProxy::NoProxy;

DKEY Proxy::Host = "host";
DVALUE(QString) Proxy::HostDef = QString();

DKEY Proxy::Username = "username";
DVALUE(QString) Proxy::UsernameDef = QString();

DKEY Proxy::Password = "password";
DVALUE(QString) Proxy::PasswordDef = QString();

DKEY Proxy::Port = "port";
DVALUE(int) Proxy::PortDef = 80;

// Database.
DKEY Database::ID = "database";

DKEY Database::UseInMemory = "use_in_memory_db";
DVALUE(bool) Database::UseInMemoryDef = false;

DKEY Database::MySQLHostname = "mysql_hostname";
DVALUE(QString) Database::MySQLHostnameDef = QString();

DKEY Database::MySQLUsername = "mysql_username";
DVALUE(QString) Database::MySQLUsernameDef = QString();

DKEY Database::MySQLPassword = "mysql_password";
DVALUE(QString) Database::MySQLPasswordDef = QString();

DKEY Database::MySQLDatabase = "mysql_database";
DVALUE(char*) Database::MySQLDatabaseDef = APP_LOW_NAME;

DKEY Database::MySQLPort = "mysql_port";
DVALUE(int) Database::MySQLPortDef = APP_DB_MYSQL_PORT;

DKEY Database::ActiveDriver = "database_driver";
DVALUE(char*) Database::ActiveDriverDef = APP_DB_SQLITE_DRIVER;

// Keyboard.
DKEY Keyboard::ID = "keyboard";

// Notifications.
DKEY Notifications::ID = "notifications";

// Web browser.
DKEY Browser::ID = "browser";

DKEY Browser::LoadExternalResources = "load_external_resources";
DVALUE(bool) Browser::LoadExternalResourcesDef = true;

DKEY Browser::OpenLinksInExternalBrowserRightAway = "open_link_externally_wo_confirmation";
DVALUE(bool) Browser::OpenLinksInExternalBrowserRightAwayDef = true;

DKEY Browser::CustomExternalBrowserEnabled = "custom_external_browser";
DVALUE(bool) Browser::CustomExternalBrowserEnabledDef = false;

DKEY Browser::CustomExternalBrowserExecutable = "external_browser_executable";
DVALUE(QString) Browser::CustomExternalBrowserExecutableDef = QString();

DKEY Browser::CustomExternalBrowserArguments = "external_browser_arguments";
DVALUE(char*) Browser::CustomExternalBrowserArgumentsDef = "\"%1\"";

DKEY Browser::CustomExternalEmailEnabled = "custom_external_email";
DVALUE(bool) Browser::CustomExternalEmailEnabledDef = false;

DKEY Browser::CustomExternalEmailExecutable = "external_email_executable";
DVALUE(QString) Browser::CustomExternalEmailExecutableDef = QString();

DKEY Browser::CustomExternalEmailArguments = "external_email_arguments";
DVALUE(char*) Browser::CustomExternalEmailArgumentsDef = "";

DKEY Browser::ExternalTools = "external_tools";
DVALUE(QStringList) Browser::ExternalToolsDef = QStringList();

// Categories.
DKEY CategoriesExpandStates::ID = "categories_expand_states";

Settings::Settings(const QString& file_name, Format format, SettingsProperties::SettingsType type, QObject* parent)
  : QSettings(file_name, format, parent), m_lock(QReadWriteLock(QReadWriteLock::RecursionMode::Recursive)),
    m_initializationStatus(type) {
  Messages::PreviewerFontStandardDef = QFont(QApplication::font().family(), 16).toString();
}

Settings::~Settings() = default;

QStringList Settings::allKeys(const QString& section) {
  if (!section.isEmpty()) {
    beginGroup(section);
    auto keys = QSettings::allKeys();

    endGroup();
    return keys;
  }
  else {
    return QSettings::allKeys();
  }
}

QString Settings::pathName() const {
  return QFileInfo(fileName()).absolutePath();
}

QSettings::Status Settings::checkSettings() {
  qDebugNN << LOGSEC_CORE << "Syncing settings.";
  sync();
  return status();
}

bool Settings::initiateRestoration(const QString& settings_backup_file_path) {
  return IOFactory::copyFile(settings_backup_file_path,
                             QFileInfo(fileName()).absolutePath() + QDir::separator() + BACKUP_NAME_SETTINGS +
                               BACKUP_SUFFIX_SETTINGS);
}

void Settings::finishRestoration(const QString& desired_settings_file_path) {
  const QString backup_settings_file = QFileInfo(desired_settings_file_path).absolutePath() + QDir::separator() +
                                       BACKUP_NAME_SETTINGS + BACKUP_SUFFIX_SETTINGS;

  if (QFile::exists(backup_settings_file)) {
    qWarningNN << LOGSEC_CORE << "Backup settings file" << QUOTE_W_SPACE(QDir::toNativeSeparators(backup_settings_file))
               << "was detected. Restoring it.";

    if (IOFactory::copyFile(backup_settings_file, desired_settings_file_path)) {
      QFile::remove(backup_settings_file);
      qDebugNN << LOGSEC_CORE << "Settings file was restored successully.";
    }
    else {
      qCriticalNN << LOGSEC_CORE << "Settings file was NOT restored due to error when copying the file.";
    }
  }
}

Settings* Settings::setupSettings(QObject* parent) {
  Settings* new_settings;

  // If settings file exists (and is writable) in executable file working directory
  // (in subdirectory APP_CFG_PATH), then use it (portable settings).
  // Otherwise use settings file stored in home path.
  const SettingsProperties properties = determineProperties();

  finishRestoration(properties.m_absoluteSettingsFileName);

  // Portable settings are available, use them.
  new_settings = new Settings(properties.m_absoluteSettingsFileName, QSettings::IniFormat, properties.m_type, parent);

  if (properties.m_type == SettingsProperties::SettingsType::Portable) {
    qDebugNN << LOGSEC_CORE << "Initializing settings in"
             << QUOTE_W_SPACE(QDir::toNativeSeparators(properties.m_absoluteSettingsFileName)) << "(portable way).";
  }
  else if (properties.m_type == SettingsProperties::SettingsType::Custom) {
    qDebugNN << LOGSEC_CORE << "Initializing settings in"
             << QUOTE_W_SPACE(QDir::toNativeSeparators(properties.m_absoluteSettingsFileName)) << "(custom way).";
  }
  else {
    qDebugNN << LOGSEC_CORE << "Initializing settings in"
             << QUOTE_W_SPACE(QDir::toNativeSeparators(properties.m_absoluteSettingsFileName)) << "(non-portable way).";
  }

  return new_settings;
}

SettingsProperties Settings::determineProperties() {
  SettingsProperties properties;

  properties.m_settingsSuffix = QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE);

  const QString app_path = qApp->userDataAppFolder();
  const QString home_path = qApp->userDataHomeFolder();
  const QString custom_path = qApp->customDataFolder();

  if (!custom_path.isEmpty()) {
    // User wants to have his user data in custom folder, okay.
    properties.m_type = SettingsProperties::SettingsType::Custom;
    properties.m_baseDirectory = custom_path;
  }
  else {
    // We will use PORTABLE settings only if it is available and NON-PORTABLE
    // settings was not initialized before.
#if defined(Q_OS_UNIX) || defined(BUILD_MSYS2)
    // DO NOT use portable settings for *nix or MSYS2, it is really not used on those platforms.
    const bool will_we_use_portable_settings = false;
#else
    const QString exe_path = qApp->applicationDirPath();
    const QString home_path_file = home_path + properties.m_settingsSuffix;
    const bool portable_settings_available = IOFactory::isFolderWritable(exe_path);
    const bool non_portable_settings_exist = QFile::exists(home_path_file);
    const bool will_we_use_portable_settings = portable_settings_available && !non_portable_settings_exist;
#endif

    if (will_we_use_portable_settings) {
      properties.m_type = SettingsProperties::SettingsType::Portable;
      properties.m_baseDirectory = QDir::toNativeSeparators(app_path);
    }
    else {
      properties.m_type = SettingsProperties::SettingsType::NonPortable;
      properties.m_baseDirectory = QDir::toNativeSeparators(home_path);
    }
  }

  properties.m_absoluteSettingsFileName = properties.m_baseDirectory + properties.m_settingsSuffix;
  return properties;
}
