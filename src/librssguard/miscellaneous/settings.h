// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGS_H
#define SETTINGS_H

#include "definitions/definitions.h"
#include "gui/messagesview.h"
#include "gui/notifications/toastnotificationsmanager.h"
#include "miscellaneous/settingsproperties.h"
#include "miscellaneous/textfactory.h"

#include <QByteArray>
#include <QColor>
#include <QDateTime>
#include <QNetworkProxy>
#include <QReadWriteLock>
#include <QSettings>
#include <QStringList>
#include <QWriteLocker>

#define KEY  RSSGUARD_DLLSPEC extern const QString
#define DKEY const QString

#define VALUE(x)           RSSGUARD_DLLSPEC extern const x
#define NON_CONST_VALUE(x) extern x

#define DVALUE(x)           const x
#define NON_CONST_DVALUE(x) x

#define SETTING(x)       x, x##Def
#define DEFAULT_VALUE(x) x##Def
#define GROUP(x)         x::ID

#if defined(NO_LITE)
namespace WebEngineAttributes {
  KEY ID;
}
#endif

namespace Cookies {
  KEY ID;
}

namespace DialogGeometries {
  KEY ID;
}

namespace Node {
  KEY ID;

  KEY NodeJsExecutable;
  VALUE(QString) NodeJsExecutableDef;

  KEY NpmExecutable;
  VALUE(QString) NpmExecutableDef;

  KEY PackageFolder;
  VALUE(QString) PackageFolderDef;
} // namespace Node

namespace VideoPlayer {
  KEY ID;

  KEY MpvUseCustomConfigFolder;
  VALUE(bool) MpvUseCustomConfigFolderDef;

  KEY MpvCustomConfigFolder;
  VALUE(QString) MpvCustomConfigFolderDef;
} // namespace VideoPlayer

namespace AdBlock {
  KEY ID;

  KEY AdBlockEnabled;
  VALUE(bool) AdBlockEnabledDef;

  KEY FilterLists;
  VALUE(QStringList) FilterListsDef;

  KEY CustomFilters;
  VALUE(QStringList) CustomFiltersDef;
} // namespace AdBlock

// Feeds.
namespace Feeds {
  KEY ID;

  KEY UpdateTimeout;
  VALUE(int) UpdateTimeoutDef;

  KEY CountFormat;
  VALUE(char*) CountFormatDef;

  KEY EnableTooltipsFeedsMessages;
  VALUE(bool) EnableTooltipsFeedsMessagesDef;

  KEY PauseFeedFetching;
  VALUE(bool) PauseFeedFetchingDef;

  KEY AutoUpdateInterval;
  VALUE(int) AutoUpdateIntervalDef;

  KEY AutoUpdateEnabled;
  VALUE(bool) AutoUpdateEnabledDef;

  KEY FastAutoUpdate;
  VALUE(bool) FastAutoUpdateDef;

  KEY AutoUpdateOnlyUnfocused;
  VALUE(bool) AutoUpdateOnlyUnfocusedDef;

  KEY FeedsUpdateOnStartup;
  VALUE(bool) FeedsUpdateOnStartupDef;

  KEY FeedsUpdateStartupDelay;
  VALUE(double) FeedsUpdateStartupDelayDef;

  KEY ShowOnlyUnreadFeeds;
  VALUE(bool) ShowOnlyUnreadFeedsDef;

  KEY SortAlphabetically;
  VALUE(bool) SortAlphabeticallyDef;

  KEY ShowTreeBranches;
  VALUE(bool) ShowTreeBranchesDef;

  KEY HideCountsIfNoUnread;
  VALUE(bool) HideCountsIfNoUnreadDef;

  KEY UpdateFeedListDuringFetching;
  VALUE(bool) UpdateFeedListDuringFetchingDef;

  KEY AutoExpandOnSelection;
  VALUE(bool) AutoExpandOnSelectionDef;

  KEY OnlyBasicShortcutsInLists;
  VALUE(bool) OnlyBasicShortcutsInListsDef;

  KEY CustomizeListFont;
  VALUE(bool) CustomizeListFontDef;

  KEY ListFont;
} // namespace Feeds

// Messages.
namespace Messages {
  KEY ID;

  KEY LimitArticleImagesHeight;
  VALUE(int) LimitArticleImagesHeightDef;

  KEY UseLegacyArticleFormat;
  VALUE(bool) UseLegacyArticleFormatDef;

  KEY DisplayEnclosuresInMessage;
  VALUE(bool) DisplayEnclosuresInMessageDef;

  KEY AvoidOldArticles;
  VALUE(bool) AvoidOldArticlesDef;

  KEY ArticleMarkOnSelection;
  VALUE(int) ArticleMarkOnSelectionDef;

  KEY ArticleMarkOnSelectionDelay;
  VALUE(int) ArticleMarkOnSelectionDelayDef;

  KEY DateTimeToAvoidArticle;
  VALUE(QDateTime) DateTimeToAvoidArticleDef;

  KEY HoursToAvoidArticle;
  VALUE(int) HoursToAvoidArticleDef;

  KEY LimitDoNotRemoveUnread;
  VALUE(bool) LimitDoNotRemoveUnreadDef;

  KEY LimitDoNotRemoveStarred;
  VALUE(bool) LimitDoNotRemoveStarredDef;

  KEY LimitRecycleInsteadOfPurging;
  VALUE(bool) LimitRecycleInsteadOfPurgingDef;

  KEY LimitCountOfArticles;
  VALUE(int) LimitCountOfArticlesDef;

  KEY AlwaysDisplayItemPreview;
  VALUE(bool) AlwaysDisplayItemPreviewDef;

  KEY EnableMessagePreview;
  VALUE(bool) EnableMessagePreviewDef;

  KEY ShowResourcesInArticles;
  VALUE(bool) ShowResourcesInArticlesDef;

  KEY Zoom;
  VALUE(qreal) ZoomDef;

  KEY FixupFutureArticleDateTimes;
  VALUE(bool) FixupFutureArticleDateTimesDef;

  KEY UseCustomDate;
  VALUE(bool) UseCustomDateDef;

  KEY CustomDateFormat;
  VALUE(char*) CustomDateFormatDef;

  KEY UseCustomTime;
  VALUE(bool) UseCustomTimeDef;

  KEY CustomFormatForDatesOnly;
  VALUE(char*) CustomFormatForDatesOnlyDef;

  KEY UseCustomFormatForDatesOnly;
  VALUE(bool) UseCustomFormatForDatesOnlyDef;

  KEY RelativeTimeForNewerArticles;
  VALUE(int) RelativeTimeForNewerArticlesDef;

  KEY ArticleListPadding;
  VALUE(int) ArticleListPaddingDef;

  KEY MultilineArticleList;
  VALUE(bool) MultilineArticleListDef;

  KEY SwitchArticleListRtl;
  VALUE(bool) SwitchArticleListRtlDef;

  KEY CustomTimeFormat;
  VALUE(QString) CustomTimeFormatDef;

  KEY ClearReadOnExit;
  VALUE(bool) ClearReadOnExitDef;

  KEY IgnoreContentsChanges;
  VALUE(bool) IgnoreContentsChangesDef;

  KEY UnreadIconType;
  VALUE(int) UnreadIconTypeDef;

  KEY BringAppToFrontAfterMessageOpenedExternally;
  VALUE(bool) BringAppToFrontAfterMessageOpenedExternallyDef;

  KEY KeepCursorInCenter;
  VALUE(bool) KeepCursorInCenterDef;

  KEY ShowOnlyUnreadMessages;
  VALUE(bool) ShowOnlyUnreadMessagesDef;

  KEY PreviewerFontStandard;
  NON_CONST_VALUE(QString) PreviewerFontStandardDef;

  KEY CustomizeListFont;
  VALUE(bool) CustomizeListFontDef;

  KEY ListFont;
} // namespace Messages

// Custom skin colors.
namespace CustomSkinColors {
  KEY ID;

  KEY Enabled;
  VALUE(bool) EnabledDef;

  KEY CustomSkinColors;
} // namespace CustomSkinColors

// GUI.
namespace GUI {
  KEY ID;

  KEY EnableNotifications;
  VALUE(bool) EnableNotificationsDef;

  KEY UseToastNotifications;
  VALUE(bool) UseToastNotificationsDef;

  KEY ToastNotificationsPosition;
  VALUE(ToastNotificationsManager::NotificationPosition) ToastNotificationsPositionDef;

  KEY ToastNotificationsScreen;
  VALUE(int) ToastNotificationsScreenDef;

  KEY ToastNotificationsMargin;
  VALUE(int) ToastNotificationsMarginDef;

  KEY ToastNotificationsOpacity;
  VALUE(double) ToastNotificationsOpacityDef;

  KEY ToastNotificationsWidth;
  VALUE(int) ToastNotificationsWidthDef;

  KEY MessageViewState;
  VALUE(QString) MessageViewStateDef;

  KEY SplitterFeeds;
  VALUE(QList<QVariant>) SplitterFeedsDef;

  KEY SplitterMessagesIsVertical;
  VALUE(bool) SplitterMessagesIsVerticalDef;

  KEY SplitterMessagesVertical;
  VALUE(QList<QVariant>) SplitterMessagesVerticalDef;

  KEY SplitterMessagesHorizontal;
  VALUE(QList<QVariant>) SplitterMessagesHorizontalDef;

  KEY ToolbarIconSize;
  VALUE(int) ToolbarIconSizeDef;

  KEY ToolbarStyle;
  VALUE(Qt::ToolButtonStyle) ToolbarStyleDef;

  KEY FeedsToolbarActions;
  VALUE(char*) FeedsToolbarActionsDef;

  KEY StatusbarActions;
  VALUE(char*) StatusbarActionsDef;

  KEY MainWindowInitialSize;
  KEY MainWindowInitialPosition;

  KEY IsMainWindowMaximizedBeforeFullscreen;
  VALUE(bool) IsMainWindowMaximizedBeforeFullscreenDef;

  KEY MainWindowStartsFullscreen;
  VALUE(bool) MainWindowStartsFullscreenDef;

  KEY MainWindowStartsHidden;
  VALUE(bool) MainWindowStartsHiddenDef;

  KEY MainWindowStartsMaximized;
  VALUE(bool) MainWindowStartsMaximizedDef;

  KEY MainMenuVisible;
  VALUE(bool) MainMenuVisibleDef;

  KEY ToolbarsVisible;
  VALUE(bool) ToolbarsVisibleDef;

  KEY ListHeadersVisible;
  VALUE(bool) ListHeadersVisibleDef;

  KEY MessageViewerToolbarsVisible;
  VALUE(bool) MessageViewerToolbarsVisibleDef;

  KEY StatusBarVisible;
  VALUE(bool) StatusBarVisibleDef;

  KEY HideMainWindowWhenMinimized;
  VALUE(bool) HideMainWindowWhenMinimizedDef;

  KEY ForcedSkinColors;
  VALUE(bool) ForcedSkinColorsDef;

  KEY AlternateRowColorsInLists;
  VALUE(bool) AlternateRowColorsInListsDef;

  KEY UseTrayIcon;
  VALUE(bool) UseTrayIconDef;

  KEY MonochromeTrayIcon;
  VALUE(bool) MonochromeTrayIconDef;

  KEY ColoredBusyTrayIcon;
  VALUE(bool) ColoredBusyTrayIconDef;

  KEY UnreadNumbersInTrayIcon;
  VALUE(bool) UnreadNumbersInTrayIconDef;

#if (defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)) || defined(Q_OS_WIN)
  KEY UnreadNumbersOnTaskBar;
  VALUE(bool) UnreadNumbersOnTaskBarDef;
#endif

  KEY TabCloseMiddleClick;
  VALUE(bool) TabCloseMiddleClickDef;

  KEY TabCloseDoubleClick;
  VALUE(bool) TabCloseDoubleClickDef;

  KEY TabNewDoubleClick;
  VALUE(bool) TabNewDoubleClickDef;

  KEY HideTabBarIfOnlyOneTab;
  VALUE(bool) HideTabBarIfOnlyOneTabDef;

  KEY MessagesToolbarDefaultButtons;
  VALUE(char*) MessagesToolbarDefaultButtonsDef;

  KEY DefaultSortColumnFeeds;
  VALUE(int) DefaultSortColumnFeedsDef;

  KEY HeightRowMessages;
  VALUE(int) HeightRowMessagesDef;

  KEY HeightRowFeeds;
  VALUE(int) HeightRowFeedsDef;

  KEY DefaultSortOrderFeeds;
  VALUE(Qt::SortOrder) DefaultSortOrderFeedsDef;

  KEY IconTheme;
  VALUE(char*) IconThemeDef;

  KEY Skin;
  VALUE(char*) SkinDef;

  KEY Style;
  VALUE(char*) StyleDef;
} // namespace GUI

// Network.
namespace Network {
  KEY ID;

  KEY SendDNT;
  VALUE(bool) SendDNTDef;

  KEY EnableApiServer;
  VALUE(bool) EnableApiServerDef;

  KEY EnableHttp2;
  VALUE(bool) EnableHttp2Def;

  KEY CustomUserAgent;
  VALUE(QString) CustomUserAgentDef;

  KEY IgnoreAllCookies;
  VALUE(bool) IgnoreAllCookiesDef;
} // namespace Network

// General.
namespace General {
  KEY ID;

  KEY UpdateOnStartup;
  VALUE(bool) UpdateOnStartupDef;

  KEY FirstRun;
  VALUE(bool) FirstRunDef;

  KEY Language;
  VALUE(QString) LanguageDef;
} // namespace General

// Downloads.
namespace Downloads {
  KEY ID;
  KEY AlwaysPromptForFilename;

  VALUE(bool) AlwaysPromptForFilenameDef;

  KEY TargetDirectory;

  VALUE(QString) TargetDirectoryDef;

  KEY RemovePolicy;

  VALUE(int) RemovePolicyDef;

  KEY TargetExplicitDirectory;

  VALUE(QString) TargetExplicitDirectoryDef;

  KEY ShowDownloadsWhenNewDownloadStarts;

  VALUE(bool) ShowDownloadsWhenNewDownloadStartsDef;

  KEY ItemUrl;
  KEY ItemLocation;
  KEY ItemDone;
} // namespace Downloads

// Proxy.
namespace Proxy {
  KEY ID;
  KEY Type;

  VALUE(QNetworkProxy::ProxyType) TypeDef;

  KEY Host;

  VALUE(QString) HostDef;

  KEY Username;

  VALUE(QString) UsernameDef;

  KEY Password;

  VALUE(QString) PasswordDef;

  KEY Port;

  VALUE(int) PortDef;
} // namespace Proxy

// Database.
namespace Database {
  KEY ID;

  KEY UseInMemory;

  VALUE(bool) UseInMemoryDef;

  KEY MySQLHostname;

  VALUE(QString) MySQLHostnameDef;

  KEY MySQLUsername;

  VALUE(QString) MySQLUsernameDef;

  KEY MySQLPassword;

  VALUE(QString) MySQLPasswordDef;

  KEY MySQLPort;

  VALUE(int) MySQLPortDef;

  KEY MySQLDatabase;

  VALUE(char*) MySQLDatabaseDef;

  KEY ActiveDriver;

  VALUE(char*) ActiveDriverDef;
} // namespace Database

// Keyboard.
namespace Keyboard {
  KEY ID;
}

// Notifications.
namespace Notifications {
  KEY ID;
}

// Web browser.
namespace Browser {
  KEY ID;

  KEY DisableCache;
  VALUE(bool) DisableCacheDef;

  KEY WebEngineChromiumFlags;
  VALUE(QString) WebEngineChromiumFlagsDef;

  KEY OpenLinksInExternalBrowserRightAway;
  VALUE(bool) OpenLinksInExternalBrowserRightAwayDef;

  KEY CustomExternalBrowserEnabled;
  VALUE(bool) CustomExternalBrowserEnabledDef;

  KEY CustomExternalBrowserExecutable;
  VALUE(QString) CustomExternalBrowserExecutableDef;

  KEY CustomExternalBrowserArguments;
  VALUE(char*) CustomExternalBrowserArgumentsDef;

  KEY CustomExternalEmailEnabled;
  VALUE(bool) CustomExternalEmailEnabledDef;

  KEY CustomExternalEmailExecutable;
  VALUE(QString) CustomExternalEmailExecutableDef;

  KEY ExternalTools;
  VALUE(QStringList) ExternalToolsDef;

  KEY CustomExternalEmailArguments;
  VALUE(char*) CustomExternalEmailArgumentsDef;
} // namespace Browser

// Categories.
namespace CategoriesExpandStates {
  KEY ID;
}

class Settings : public QSettings {
    Q_OBJECT

  public:
    // Destructor.
    virtual ~Settings();

    // Type of used settings.
    SettingsProperties::SettingsType type() const;

    // Getters/setters for settings values.
    QVariant password(const QString& section, const QString& key, const QVariant& default_value = QVariant()) const;
    void setPassword(const QString& section, const QString& key, const QVariant& value);

    QStringList allKeys(const QString& section);

    QVariant value(const QString& section, const QString& key, const QVariant& default_value = QVariant()) const;
    void setValue(const QString& section, const QString& key, const QVariant& value);
    void setValue(const QString& key, const QVariant& value);

    bool contains(const QString& section, const QString& key) const;
    void remove(const QString& section, const QString& key = {});

    // Returns the path which contains the settings.
    QString pathName() const;

    // Synchronizes settings.
    QSettings::Status checkSettings();

    bool initiateRestoration(const QString& settings_backup_file_path);
    static void finishRestoration(const QString& desired_settings_file_path);

    // Creates settings file in correct location.
    static Settings* setupSettings(QObject* parent);

    // Returns properties of the actual application-wide settings.
    static SettingsProperties determineProperties();

  private:
    explicit Settings(const QString& file_name,
                      Format format,
                      SettingsProperties::SettingsType type,
                      QObject* parent = nullptr);

  private:
    mutable QReadWriteLock m_lock;
    SettingsProperties::SettingsType m_initializationStatus;
};

inline SettingsProperties::SettingsType Settings::type() const {
  return m_initializationStatus;
}

// Getters/setters for settings values.
inline QVariant Settings::password(const QString& section, const QString& key, const QVariant& default_value) const {
  return TextFactory::decrypt(value(section, key, default_value).toString());
}

inline void Settings::setPassword(const QString& section, const QString& key, const QVariant& value) {
  setValue(section, key, TextFactory::encrypt(value.toString()));
}

inline QVariant Settings::value(const QString& section, const QString& key, const QVariant& default_value) const {
  return QSettings::value(QString(QSL("%1/%2")).arg(section, key), default_value);
}

inline void Settings::setValue(const QString& section, const QString& key, const QVariant& value) {
  QWriteLocker lck(&m_lock);
  QSettings::setValue(QString(QSL("%1/%2")).arg(section, key), value);
}

inline void Settings::setValue(const QString& key, const QVariant& value) {
  QWriteLocker lck(&m_lock);
  QSettings::setValue(key, value);
}

inline bool Settings::contains(const QString& section, const QString& key) const {
  return QSettings::contains(QString(QSL("%1/%2")).arg(section, key));
}

inline void Settings::remove(const QString& section, const QString& key) {
  QWriteLocker lck(&m_lock);

  if (key.isEmpty()) {
    beginGroup(section);
    QSettings::remove({});
    endGroup();
  }
  else {
    QSettings::remove(QString(QSL("%1/%2")).arg(section, key));
  }
}

#endif // SETTINGS_H
