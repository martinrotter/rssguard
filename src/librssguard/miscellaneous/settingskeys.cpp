// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/settingskeys.h"

#include "gui/messagesview.h"
#include "gui/notifications/toastnotificationsmanager.h"

DKEY Cookies::ID = QSL("cookies");
DKEY WebEngineAttributes::ID = QSL("web_engine_attributes");
DKEY FileDialogPaths::ID = QSL("file_dialog_paths");
DKEY DialogGeometries::ID = QSL("dialog_geometries");
DKEY MessageBoxDontShows::ID = QSL("msg_box_dont_show");

// Web.
DKEY Web::ID = QSL("web");

DKEY Web::FollowLinks = QSL("follow_links");
DVALUE(bool) Web::FollowLinksDef = true;

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
DKEY Web::WebEngineChromiumFlags = QSL("webengine_flags");

#if defined(Q_OS_WIN)
// Windows.
DVALUE(QString) Web::WebEngineChromiumFlagsDef = QSL("--enable-smooth-scrolling");
#else
// Everywhere else (Flatpak, Linux, macOS, OS/2, whatever).
DVALUE(QString) Web::WebEngineChromiumFlagsDef = QSL("--enable-smooth-scrolling");
#endif
#endif

// Media player.
KEY VideoPlayer::ID = QSL("media_player");

DKEY VideoPlayer::MpvUseCustomConfigFolder = QSL("mpv_use_custom_config_folder");
DVALUE(bool) VideoPlayer::MpvUseCustomConfigFolderDef = true;

DKEY VideoPlayer::MpvCustomConfigFolder = QSL("mpv_config_folder");
DVALUE(QString) VideoPlayer::MpvCustomConfigFolderDef = QSL("%data%/mpv");

// Network.
DKEY Network::ID = QSL("network");

DKEY Network::SendDNT = QSL("send_dnt");
VALUE(bool) Network::SendDNTDef = false;

DKEY Network::EnableHttp2 = QSL("http2_enabled");
DVALUE(bool) Network::EnableHttp2Def = false;

DKEY Network::CustomUserAgent = QSL("user_agent");
DVALUE(QString) Network::CustomUserAgentDef = QString();

DKEY Network::IgnoreAllCookies = QSL("ignore_all_cookies");
DVALUE(bool) Network::IgnoreAllCookiesDef = false;

// Feeds.
DKEY Feeds::ID = QSL("feeds");

DKEY Feeds::UpdateTimeout = QSL("feed_update_timeout");
DVALUE(int) Feeds::UpdateTimeoutDef = DOWNLOAD_TIMEOUT;

DKEY Feeds::CountFormat = QSL("count_format");
DVALUE(char*) Feeds::CountFormatDef = "%unread-%all";

DKEY Feeds::CountAlignment = QSL("count_alignment");
DVALUE(Qt::AlignmentFlag) Feeds::CountAlignmentDef = Qt::AlignmentFlag::AlignCenter;

DKEY Feeds::KeepCursorInCenter = QSL("keep_cursor_center");
DVALUE(bool) Feeds::KeepCursorInCenterDef = false;

DKEY Feeds::EnableTooltipsFeedsMessages = QSL("show_tooltips");
DVALUE(bool) Feeds::EnableTooltipsFeedsMessagesDef = true;

DKEY Feeds::StrikethroughDisabledFeeds = QSL("strikethrough_disabled_feeds");
DVALUE(bool) Feeds::StrikethroughDisabledFeedsDef = true;

DKEY Feeds::DontAskWhenMarkinAllRead = QSL("dont_ask_when_marking_all_read");
DVALUE(bool) Feeds::DontAskWhenMarkinAllReadDef = false;

DKEY Feeds::FetchOnlyWhenNotGameMode = QSL("fetch_only_when_not_gamemode");
DVALUE(bool) Feeds::FetchOnlyWhenNotGameModeDef = false;

DKEY Feeds::FetchOnlyWhenNetworkConnected = QSL("fetch_only_when_network");
DVALUE(bool) Feeds::FetchOnlyWhenNetworkConnectedDef = false;

DKEY Feeds::PauseFeedFetching = QSL("pause_feed_fetching");
DVALUE(bool) Feeds::PauseFeedFetchingDef = false;

DKEY Feeds::AutoUpdateInterval = QSL("auto_update_interval");
DVALUE(int) Feeds::AutoUpdateIntervalDef = DEFAULT_AUTO_UPDATE_INTERVAL;

DKEY Feeds::PropagateFeedListStates = QSL("propagate_feed_list_states");
DVALUE(bool) Feeds::PropagateFeedListStatesDef = true;

DKEY Feeds::AutoUpdateEnabled = QSL("auto_update_enabled");
DVALUE(bool) Feeds::AutoUpdateEnabledDef = false;

DKEY Feeds::FastAutoUpdate = QSL("auto_update_fast");
DVALUE(bool) Feeds::FastAutoUpdateDef = false;

DKEY Feeds::AutoUpdateOnlyUnfocused = QSL("auto_update_only_unfocused");
DVALUE(bool) Feeds::AutoUpdateOnlyUnfocusedDef = false;

DKEY Feeds::FeedsUpdateOnStartup = QSL("feeds_update_on_startup");
DVALUE(bool) Feeds::FeedsUpdateOnStartupDef = false;

DKEY Feeds::FeedsUpdateStartupDelay = QSL("feeds_update_on_startup_delay");
DVALUE(double) Feeds::FeedsUpdateStartupDelayDef = STARTUP_UPDATE_DELAY;

DKEY Feeds::SortAlphabetically = QSL("sort_alphabetically");
DVALUE(bool) Feeds::SortAlphabeticallyDef = false;

DKEY Feeds::ShowTreeBranches = QSL("show_tree_branches");
DVALUE(bool) Feeds::ShowTreeBranchesDef = true;

DKEY Feeds::HideCountsIfNoUnread = QSL("hide_counts_if_no_unread");
DVALUE(bool) Feeds::HideCountsIfNoUnreadDef = false;

DKEY Feeds::AlwaysShowCountsLabelsImportant = QSL("hide_counts_if_no_unread_labels_important");
DVALUE(bool) Feeds::AlwaysShowCountsLabelsImportantDef = false;

DKEY Feeds::UpdateFeedListDuringFetching = QSL("update_feed_list_during_fetching");
DVALUE(bool) Feeds::UpdateFeedListDuringFetchingDef = false;

DKEY Feeds::AutoExpandOnSelection = QSL("auto_expand_on_selection");
DVALUE(bool) Feeds::AutoExpandOnSelectionDef = false;

DKEY Feeds::OnlyBasicShortcutsInLists = QSL("only_basic_shortcuts_in_lists");
DVALUE(bool) Feeds::OnlyBasicShortcutsInListsDef = false;

DKEY Feeds::CustomizeListFont = QSL("customize_list_font");
DVALUE(bool) Feeds::CustomizeListFontDef = false;

DKEY Feeds::ListFont = QSL("list_font");

// Messages.
DKEY Messages::ID = QSL("messages");

DKEY Messages::LimitArticleImagesHeight = QSL("message_head_image_height");
DVALUE(int) Messages::LimitArticleImagesHeightDef = 0;

DKEY Messages::LimitEnclosureImagesHeight = QSL("enclosure_image_height");
DVALUE(int) Messages::LimitEnclosureImagesHeightDef = 72;

DKEY Messages::DisplayEnclosuresInMessage = QSL("show_enclosures_in_message");
DVALUE(bool) Messages::DisplayEnclosuresInMessageDef = false;

DKEY Messages::AvoidOldArticles = QSL("avoid_old_articles");
DVALUE(bool) Messages::AvoidOldArticlesDef = false;

DKEY Messages::CopyArticlePattern = QSL("copy_article_pattern");
DVALUE(QString) Messages::CopyArticlePatternDef = QSL("%6% - %8%");

DKEY Messages::CopyArticleEscapeCsv = QSL("copy_article_escape_csv");
DVALUE(bool) Messages::CopyArticleEscapeCsvDef = false;

DKEY Messages::FontAa = QSL("font_aa");
DVALUE(bool) Messages::FontAaDef = true;

DKEY Messages::ShapeAa = QSL("shape_aa");
DVALUE(bool) Messages::ShapeAaDef = true;

DKEY Messages::ArticleListLazyLoading = QSL("article_list_lazy_loading");
DVALUE(bool) Messages::ArticleListLazyLoadingDef = false;

DKEY Messages::ArticleListColumnProfiles = QSL("article_list_column_profiles");
DVALUE(bool) Messages::ArticleListColumnProfilesDef = false;

DKEY Messages::DateTimeToAvoidArticle = QSL("datetime_to_avoid_article");
DVALUE(QDateTime) Messages::DateTimeToAvoidArticleDef = QDateTime::currentDateTime();

DKEY Messages::HoursToAvoidArticle = QSL("hours_to_avoid_article");
DVALUE(int) Messages::HoursToAvoidArticleDef = 0;

DKEY Messages::LimitDoNotRemoveUnread = QSL("limit_dont_remove_unread");
DVALUE(bool) Messages::LimitDoNotRemoveUnreadDef = true;

DKEY Messages::LimitDoNotRemoveStarred = QSL("limit_dont_remove_starred");
DVALUE(bool) Messages::LimitDoNotRemoveStarredDef = true;

DKEY Messages::LimitRecycleInsteadOfPurging = QSL("limit_recycle_dont_purge");
DVALUE(bool) Messages::LimitRecycleInsteadOfPurgingDef = false;

DKEY Messages::LimitCountOfArticles = QSL("limit_count_of_articles");
DVALUE(int) Messages::LimitCountOfArticlesDef = 0;

DKEY Messages::AlwaysDisplayItemPreview = QSL("always_display_preview");
DVALUE(bool) Messages::AlwaysDisplayItemPreviewDef = true;

DKEY Messages::EnableMessagePreview = QSL("enable_message_preview");
DVALUE(bool) Messages::EnableMessagePreviewDef = true;

DKEY Messages::ShowResourcesInArticles = QSL("enable_message_resources");
DVALUE(bool) Messages::ShowResourcesInArticlesDef = true;

DKEY Messages::Zoom = QSL("zoom");
DVALUE(qreal) Messages::ZoomDef = double(1.0);

DKEY Messages::FixupFutureArticleDateTimes = QSL("fixup_future_datetimes");
DVALUE(bool) Messages::FixupFutureArticleDateTimesDef = false;

DKEY Messages::UseCustomDate = QSL("use_custom_date");
DVALUE(bool) Messages::UseCustomDateDef = false;

DKEY Messages::CustomDateFormat = QSL("custom_date_format");
DVALUE(char*) Messages::CustomDateFormatDef = "";

DKEY Messages::CustomFormatForDatesOnly = QSL("custom_date_format_for_dates_only");
DVALUE(char*) Messages::CustomFormatForDatesOnlyDef = "";

DKEY Messages::UseCustomFormatForDatesOnly = QSL("use_custom_date_for_dates_only");
DVALUE(bool) Messages::UseCustomFormatForDatesOnlyDef = false;

DKEY Messages::RelativeTimeForNewerArticles = QSL("relative_time_for_new_articles");
DVALUE(int) Messages::RelativeTimeForNewerArticlesDef = -1;

DKEY Messages::ArticleMarkOnSelection = QSL("mark_message_on_selected");
DVALUE(int) Messages::ArticleMarkOnSelectionDef = int(MessagesView::ArticleMarkingPolicy::MarkImmediately);

DKEY Messages::ArticleMarkOnSelectionDelay = QSL("mark_message_on_selected_delay");
DVALUE(int) Messages::ArticleMarkOnSelectionDelayDef = 3000;

DKEY Messages::ArticleListPadding = QSL("article_list_padding");
DVALUE(int) Messages::ArticleListPaddingDef = -1;

DKEY Messages::ShowFeedIconInFeedColumn = QSL("show_feed_icon_in_feed_column");
DVALUE(bool) Messages::ShowFeedIconInFeedColumnDef = false;

DKEY Messages::MultilineArticleList = QSL("multiline_article_list");
DVALUE(bool) Messages::MultilineArticleListDef = false;

DKEY Messages::SwitchArticleListRtl = QSL("switch_article_list_rtl");
DVALUE(bool) Messages::SwitchArticleListRtlDef = true;

DKEY Messages::UseCustomTime = QSL("use_custom_time");
DVALUE(bool) Messages::UseCustomTimeDef = false;

DKEY Messages::CustomTimeFormat = QSL("custom_time_format");
DVALUE(QString) Messages::CustomTimeFormatDef = {};

DKEY Messages::ClearReadOnExit = QSL("clear_read_on_exit");
DVALUE(bool) Messages::ClearReadOnExitDef = false;

DKEY Messages::IgnoreContentsChanges = QSL("ignore_contents_changes");
DVALUE(bool) Messages::IgnoreContentsChangesDef = true;

DKEY Messages::MarkUnreadOnUpdated = QSL("mark_unread_on_update");
DVALUE(bool) Messages::MarkUnreadOnUpdatedDef = false;

DKEY Messages::MarkReadAfterOpenedExtInt = QSL("mark_read_after_opened_ext_int");
DVALUE(bool) Messages::MarkReadAfterOpenedExtIntDef = false;

DKEY Messages::UnreadIconType = QSL("unread_icons_in_message_list");
DVALUE(int) Messages::UnreadIconTypeDef = 1; /* MessagesModel::MessageUnreadIcon::Dot */

DKEY Messages::BringAppToFrontAfterMessageOpenedExternally = QSL("bring_app_to_front_after_msg_opened");
DVALUE(bool) Messages::BringAppToFrontAfterMessageOpenedExternallyDef = false;

DKEY Messages::KeepCursorInCenter = QSL("keep_cursor_center");
DVALUE(bool) Messages::KeepCursorInCenterDef = false;

DKEY Messages::ShowOnlyUnreadMessages = QSL("show_only_unread_messages");
DVALUE(bool) Messages::ShowOnlyUnreadMessagesDef = false;

DKEY Messages::PreviewerFontStandard = QSL("previewer_font_standard");
NON_CONST_DVALUE(QString) Messages::PreviewerFontStandardDef = QString();

DKEY Messages::CustomizeListFont = QSL("customize_list_font");
DVALUE(bool) Messages::CustomizeListFontDef = false;

DKEY Messages::ListFont = QSL("list_font");

// Custom skin colors.
DKEY CustomSkinColors::ID = QSL("custom_skin_colors");

DKEY CustomSkinColors::Enabled = QSL("enabled");
DVALUE(bool) CustomSkinColors::EnabledDef = false;

// GUI.
DKEY GUI::ID = QSL("gui");

DKEY GUI::SettingsSelectedSection = QSL("settings_selected_section");
DVALUE(QString) GUI::SettingsSelectedSectionDef = QString();

DKEY GUI::FeedViewState = QSL("feed_view_state");
DVALUE(QString) GUI::FeedViewStateDef = QString();

DKEY GUI::MessageViewState = QSL("msg_view_state");
DVALUE(QString) GUI::MessageViewStateDef = QString();

DKEY GUI::MessageViewStateFeed = QSL("msg_view_state_feed");
DVALUE(QString) GUI::MessageViewStateFeedDef = QString();

DKEY GUI::MessageViewStateCategory = QSL("msg_view_state_category");
DVALUE(QString) GUI::MessageViewStateCategoryDef = QString();

DKEY GUI::MessageViewStateAccount = QSL("msg_view_state_account");
DVALUE(QString) GUI::MessageViewStateAccountDef = QString();

DKEY GUI::MessageViewStateLabels = QSL("msg_view_state_labels");
DVALUE(QString) GUI::MessageViewStateLabelsDef = QString();

DKEY GUI::MessageViewStateLabel = QSL("msg_view_state_label");
DVALUE(QString) GUI::MessageViewStateLabelDef = QString();

DKEY GUI::MessageViewStateProbes = QSL("msg_view_state_probes");
DVALUE(QString) GUI::MessageViewStateProbesDef = QString();

DKEY GUI::MessageViewStateProbe = QSL("msg_view_state_probe");
DVALUE(QString) GUI::MessageViewStateProbeDef = QString();

DKEY GUI::MessageViewStateImportant = QSL("msg_view_state_important");
DVALUE(QString) GUI::MessageViewStateImportantDef = QString();

DKEY GUI::MessageViewStateUnread = QSL("msg_view_state_unread");
DVALUE(QString) GUI::MessageViewStateUnreadDef = QString();

DKEY GUI::MessageViewStateBin = QSL("msg_view_state_bin");
DVALUE(QString) GUI::MessageViewStateBinDef = QString();

DKEY GUI::SplitterFeeds = QSL("splitter_feeds");
DVALUE(QList<QVariant>) GUI::SplitterFeedsDef = {};

DKEY GUI::SplitterMessagesIsVertical = QSL("splitter_messages_is_vertical");
DVALUE(bool) GUI::SplitterMessagesIsVerticalDef = true;

DKEY GUI::SplitterMessagesVertical = QSL("splitter_messages_vertical");
DVALUE(QList<QVariant>) GUI::SplitterMessagesVerticalDef = {};

DKEY GUI::SplitterMessagesHorizontal = QSL("splitter_messages_horizontal");
DVALUE(QList<QVariant>) GUI::SplitterMessagesHorizontalDef = {};

DKEY GUI::ToolbarIconSize = QSL("toolbar_icon_size");
DVALUE(int) GUI::ToolbarIconSizeDef = 0;

DKEY GUI::ToolbarStyle = QSL("toolbar_style");
DVALUE(Qt::ToolButtonStyle) GUI::ToolbarStyleDef = Qt::ToolButtonIconOnly;

DKEY GUI::HeightRowMessages = QSL("height_row_messages");
DVALUE(int) GUI::HeightRowMessagesDef = -1;

DKEY GUI::HeightRowFeeds = QSL("height_row_feeds");
DVALUE(int) GUI::HeightRowFeedsDef = -1;

DKEY GUI::FeedsToolbarActions = QSL("feeds_toolbar");
DVALUE(char*)
GUI::FeedsToolbarActionsDef = "m_actionUpdateAllItems,m_actionStopRunningItemsUpdate,m_actionPauseFeedFetching,m_"
                              "actionMarkAllItemsRead,filter,spacer,search";

DKEY GUI::StatusbarActions = QSL("status_bar");
DVALUE(char*)
GUI::StatusbarActionsDef = "m_barProgressDownloadAction,m_barProgressFeedsAction,m_actionUpdateAllItems,m_"
                           "actionUpdateSelectedItems,m_actionStopRunningItemsUpdate,m_actionFullscreen,m_actionQuit";

DKEY GUI::MainWindowInitialSize = QSL("window_size");
DKEY GUI::MainWindowInitialPosition = QSL("window_position");

DKEY GUI::IsMainWindowMaximizedBeforeFullscreen = QSL("is_window_maximized_before_fullscreen");
DVALUE(bool) GUI::IsMainWindowMaximizedBeforeFullscreenDef = false;

DKEY GUI::MainWindowStartsFullscreen = QSL("start_in_fullscreen");
DVALUE(bool) GUI::MainWindowStartsFullscreenDef = false;

DKEY GUI::MainWindowStartsHidden = QSL("start_hidden");
DVALUE(bool) GUI::MainWindowStartsHiddenDef = false;

DKEY GUI::MainWindowStartsMaximized = QSL("window_is_maximized");
DVALUE(bool) GUI::MainWindowStartsMaximizedDef = false;

DKEY GUI::AlternateRowColorsInLists = QSL("alternate_colors_in_lists");
DVALUE(bool) GUI::AlternateRowColorsInListsDef = false;

DKEY GUI::MainMenuVisible = QSL("main_menu_visible");
DVALUE(bool) GUI::MainMenuVisibleDef = true;

DKEY GUI::ToolbarsVisible = QSL("enable_toolbars");
DVALUE(bool) GUI::ToolbarsVisibleDef = true;

DKEY GUI::ListHeadersVisible = QSL("enable_list_headers");
DVALUE(bool) GUI::ListHeadersVisibleDef = true;

DKEY GUI::MessageViewerToolbarsVisible = QSL("message_viewer_toolbars");
DVALUE(bool) GUI::MessageViewerToolbarsVisibleDef = true;

DKEY GUI::StatusBarVisible = QSL("enable_status_bar");
DVALUE(bool) GUI::StatusBarVisibleDef = true;

DKEY GUI::EnableNotifications = QSL("enable_notifications");
DVALUE(bool) GUI::EnableNotificationsDef = true;

DKEY GUI::UseToastNotifications = QSL("use_toast_notifications");
DVALUE(bool) GUI::UseToastNotificationsDef = true;

DKEY GUI::ToastNotificationsPosition = QSL("toast_notifications_position");
DVALUE(int)
GUI::ToastNotificationsPositionDef = int(ToastNotificationsManager::NotificationPosition::BottomRight);

DKEY GUI::ToastNotificationsDuration = QSL("toast_notifications_duration");
DVALUE(int) GUI::ToastNotificationsDurationDef = NOTIFICATIONS_TIMEOUT;

DKEY GUI::ToastNotificationsScreen = QSL("toast_notifications_screen");
DVALUE(int) GUI::ToastNotificationsScreenDef = -1;

DKEY GUI::ToastNotificationsMargin = QSL("toast_notifications_margin");
DVALUE(int) GUI::ToastNotificationsMarginDef = NOTIFICATIONS_MARGIN;

DKEY GUI::ToastNotificationsOpacity = QSL("toast_notifications_opacity");
DVALUE(double) GUI::ToastNotificationsOpacityDef = 0.9;

DKEY GUI::ToastNotificationsWidth = QSL("toast_notifications_width");
DVALUE(int) GUI::ToastNotificationsWidthDef = NOTIFICATIONS_WIDTH;

DKEY GUI::HideMainWindowWhenMinimized = QSL("hide_when_minimized");
DVALUE(bool) GUI::HideMainWindowWhenMinimizedDef = false;

DKEY GUI::MonochromeTrayIcon = QSL("monochrome_tray_icon");
DVALUE(bool) GUI::MonochromeTrayIconDef = false;

DKEY GUI::CustomColoredTrayIcon = QSL("custom_colored_tray_icon");
DVALUE(bool) GUI::CustomColoredTrayIconDef = false;

DKEY GUI::CustomColoredTrayIconBackground = QSL("custom_colored_tray_icon_background");
DVALUE(char*) GUI::CustomColoredTrayIconBackgroundDef = "#d96a00";

DKEY GUI::CustomColoredTrayIconText = QSL("custom_colored_tray_icon_text");
DVALUE(char*) GUI::CustomColoredTrayIconTextDef = "#ffffff";

DKEY GUI::CustomColoredTrayIconAsAppIcon = QSL("custom_colored_tray_icon_as_app_icon");
DVALUE(bool) GUI::CustomColoredTrayIconAsAppIconDef = false;

DKEY GUI::CustomizeAppFont = QSL("custom_app_font");
DVALUE(bool) GUI::CustomizeAppFontDef = false;

DKEY GUI::AppFont = QSL("app_font");

DKEY GUI::FontAntialiasing = QSL("font_antialiasing");
DVALUE(bool) GUI::FontAntialiasingDef = true;

DKEY GUI::ColoredBusyTrayIcon = QSL("colored_busy_tray_icon");
DVALUE(bool) GUI::ColoredBusyTrayIconDef = false;

DKEY GUI::ForcedSkinColors = QSL("forced_skin_colors");
DVALUE(bool) GUI::ForcedSkinColorsDef = true;

DKEY GUI::UnreadNumbersInTrayIcon = QSL("show_unread_numbers_in_tray_icon");
DVALUE(bool) GUI::UnreadNumbersInTrayIconDef = true;

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)) || defined(Q_OS_WIN)
DKEY GUI::UnreadNumbersOnTaskBar = QSL("show_unread_numbers_on_task_bar");
DVALUE(bool) GUI::UnreadNumbersOnTaskBarDef = true;
#endif

#if defined(Q_OS_WIN)
DKEY GUI::TaskbarThumbnailButtons = QSL("show_taskbar_thumbnail_buttons");
DVALUE(bool) GUI::TaskbarThumbnailButtonsDef = true;

DKEY GUI::TaskbarErrorProgress = QSL("show_taskbar_error_progress");
DVALUE(bool) GUI::TaskbarErrorProgressDef = true;
#endif

DKEY GUI::UnreadNumbersOnWindow = QSL("show_unread_numbers_on_window");
DVALUE(bool) GUI::UnreadNumbersOnWindowDef = true;

DKEY GUI::UseTrayIcon = QSL("use_tray_icon");
DVALUE(bool) GUI::UseTrayIconDef = true;

DKEY GUI::TabCloseMiddleClick = QSL("tab_close_mid_button");
DVALUE(bool) GUI::TabCloseMiddleClickDef = true;

DKEY GUI::TabCloseDoubleClick = QSL("tab_close_double_button");
DVALUE(bool) GUI::TabCloseDoubleClickDef = true;

DKEY GUI::TabNewDoubleClick = QSL("tab_new_double_button");
DVALUE(bool) GUI::TabNewDoubleClickDef = true;

DKEY GUI::HideTabBarIfOnlyOneTab = QSL("hide_tabbar_one_tab");
DVALUE(bool) GUI::HideTabBarIfOnlyOneTabDef = false;

DKEY GUI::MessagesToolbarDefaultButtons = QSL("messages_toolbar");
DVALUE(char*)
GUI::MessagesToolbarDefaultButtonsDef =
  "m_actionMarkSelectedMessagesAsRead,m_actionMarkSelectedMessagesAsUnread,m_actionSwitchImportanceOfSelectedMessages,"
  "separator,highlighter,filter,m_actionLoadAllArticles,spacer,search";

DKEY GUI::DefaultSortColumnFeeds = QSL("default_sort_column_feeds");
DVALUE(int) GUI::DefaultSortColumnFeedsDef = FDS_MODEL_TITLE_INDEX;

DKEY GUI::DefaultSortOrderFeeds = QSL("default_sort_order_feeds");
DVALUE(Qt::SortOrder) GUI::DefaultSortOrderFeedsDef = Qt::AscendingOrder;

DKEY GUI::IconTheme = QSL("icon_theme_name");

DKEY GUI::Skin = QSL("skin");

DKEY GUI::Style = QSL("style");
DVALUE(char*) GUI::StyleDef = APP_STYLE_DEFAULT;

// General.
DKEY General::ID = QSL("main");

DKEY General::DisableDebugOutput = QSL("disable_debug_output");
DVALUE(bool) General::DisableDebugOutputDef = false;

DKEY General::UpdateOnStartup = QSL("update_on_start");
DVALUE(bool) General::UpdateOnStartupDef = false;

DKEY General::ShowSplashScreen = QSL("show_splash_screen");
DVALUE(bool) General::ShowSplashScreenDef = false;

DKEY General::FirstRun = QSL("first_run");
DVALUE(bool) General::FirstRunDef = true;

DKEY General::Language = QSL("language");
DVALUE(QString) General::LanguageDef = QLocale::system().name();

// Proxy.
DKEY Proxy::ID = QSL("proxy");

DKEY Proxy::Type = QSL("proxy_type");
DVALUE(QNetworkProxy::ProxyType) Proxy::TypeDef = QNetworkProxy::DefaultProxy;

DKEY Proxy::Host = QSL("host");
DVALUE(QString) Proxy::HostDef = QString();

DKEY Proxy::Username = QSL("username");
DVALUE(QString) Proxy::UsernameDef = QString();

DKEY Proxy::Password = QSL("password");
DVALUE(QString) Proxy::PasswordDef = QString();

DKEY Proxy::Port = QSL("port");
DVALUE(int) Proxy::PortDef = 80;

// Database.
DKEY Database::ID = QSL("database");

DKEY Database::MySQLHostname = QSL("mysql_hostname");
DVALUE(QString) Database::MySQLHostnameDef = QString();

DKEY Database::MySQLUsername = QSL("mysql_username");
DVALUE(QString) Database::MySQLUsernameDef = QString();

DKEY Database::MySQLPassword = QSL("mysql_password");
DVALUE(QString) Database::MySQLPasswordDef = QString();

DKEY Database::MySQLDatabase = QSL("mysql_database");
DVALUE(char*) Database::MySQLDatabaseDef = APP_LOW_NAME;

DKEY Database::MySQLPort = QSL("mysql_port");
DVALUE(int) Database::MySQLPortDef = APP_DB_MYSQL_PORT;

DKEY Database::ActiveDriver = QSL("database_driver");
DVALUE(char*) Database::ActiveDriverDef = APP_DB_SQLITE_DRIVER;

// Keyboard.
DKEY Keyboard::ID = QSL("keyboard");

// Notifications.
DKEY Notifications::ID = QSL("notifications");

// External tools.
DKEY ExternalTools::ID = QSL("external_tools");

// Web browser.
DKEY Browser::ID = QSL("browser");

DKEY Browser::LoadExternalResources = QSL("load_external_resources");
DVALUE(bool) Browser::LoadExternalResourcesDef = true;

DKEY Browser::CustomExternalBrowserEnabled = QSL("custom_external_browser");
DVALUE(bool) Browser::CustomExternalBrowserEnabledDef = false;

DKEY Browser::CustomExternalBrowserExecutable = QSL("external_browser_executable");
DVALUE(QString) Browser::CustomExternalBrowserExecutableDef = QString();

DKEY Browser::CustomExternalBrowserArguments = QSL("external_browser_arguments");
DVALUE(char*) Browser::CustomExternalBrowserArgumentsDef = "\"%1\"";

DKEY Browser::CustomExternalEmailEnabled = QSL("custom_external_email");
DVALUE(bool) Browser::CustomExternalEmailEnabledDef = false;

DKEY Browser::CustomExternalEmailExecutable = QSL("external_email_executable");
DVALUE(QString) Browser::CustomExternalEmailExecutableDef = QString();

DKEY Browser::CustomExternalEmailArguments = QSL("external_email_arguments");
DVALUE(char*) Browser::CustomExternalEmailArgumentsDef = "";

// Categories.
DKEY CategoriesExpandStates::ID = QSL("categories_expand_states");
