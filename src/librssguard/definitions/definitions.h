// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <QDebug>
#include <QtGlobal>

#define SERVICE_CODE_STD_RSS   "std-rss"
#define SERVICE_CODE_TT_RSS    "tt-rss"
#define SERVICE_CODE_OWNCLOUD  "owncloud"
#define SERVICE_CODE_GREADER   "greader"
#define SERVICE_CODE_FEEDLY    "feedly"
#define SERVICE_CODE_INOREADER "inoreader"
#define SERVICE_CODE_GMAIL     "gmail"
#define SERVICE_CODE_REDDIT    "reddit"
#define SERVICE_CODE_NEWSBLUR  "newsblur"

#define ADBLOCK_SERVER_FILE   "adblock-server.js"
#define ADBLOCK_SERVER_PORT   48484
#define ADBLOCK_HOWTO         APP_URL_DOCUMENTATION "#adbl"
#define ADBLOCK_ICON_ACTIVE   "adblock"
#define ADBLOCK_ICON_DISABLED "adblock-disabled"

#define OAUTH_DECRYPTION_KEY 11451167756100761335ul
#define OAUTH_REDIRECT_URI   "http://localhost"

#define ENCLOSURES_OUTER_SEPARATOR '#'
#define ECNLOSURES_INNER_SEPARATOR '&'

#define URI_SCHEME_FEED_SHORT "feed:"
#define URI_SCHEME_FEED       "feed://"

#define URI_SCHEME_HTTP_SHORT "http:"
#define URI_SCHEME_HTTP       "http://"

#define URI_SCHEME_HTTPS_SHORT "https:"
#define URI_SCHEME_HTTPS       "https://"

#define DEFAULT_LOCALE "en_US"

#define NO_PARENT_CATEGORY -1
#define ID_RECYCLE_BIN     -2
#define ID_IMPORTANT       -3
#define ID_LABELS          -4
#define ID_UNREAD          -5
#define ID_PROBES          -6

#define MSG_SCORE_MAX 100.0
#define MSG_SCORE_MIN 0.0

#define LOWER_TITLE_ROLE                  64
#define HIGHLIGHTED_FOREGROUND_TITLE_ROLE 65
#define TEXT_DIRECTION_ROLE               66

#define SOUNDS_BUILTIN_DIRECTORY    ":/sounds"
#define ARGUMENTS_LIST_SEPARATOR    "\n"
#define IS_IN_ARRAY(offset, array)  ((offset >= 0) && (offset < array.count()))
#define DEFAULT_SQL_MESSAGES_FILTER "0 > 1"
#define MAX_MULTICOLUMN_SORT_STATES 3

#define RELEASES_LIST      "https://api.github.com/repos/martinrotter/rssguard/releases"
#define MSG_FILTERING_HELP APP_URL_DOCUMENTATION "#fltr"
#define URL_REGEXP                                                                                             \
  "^(http|https|feed|ftp):\\/\\/[\\w\\-_]+(\\.[\\w\\-_]+)+([\\w\\-\\.,@?^=%&amp;:/~\\+#]*[\\w\\-\\@?^=%&amp;/" \
  "~\\+#])?$"
#define SCRIPT_SOURCE_TYPE_REGEXP    "^.+#.*$"
#define TEXT_TITLE_LIMIT             30
#define TEXT_TOOLTIP_LIMIT           50
#define RESELECT_MESSAGE_THRESSHOLD  500
#define ICON_SIZE_SETTINGS           16
#define TRAY_ICON_BUBBLE_TIMEOUT     20000
#define MSG_DATETIME_DIFF_THRESSHOLD 1000 * 120 // In seconds.
#define CLOSE_LOCK_TIMEOUT           500
#define DOWNLOAD_TIMEOUT             30000
#define MESSAGES_VIEW_DEFAULT_COL    100
#define MESSAGES_VIEW_MINIMUM_COL    16
#define FEEDS_VIEW_COLUMN_COUNT      2
#define DEFAULT_DAYS_TO_DELETE_MSG   14
#define ELLIPSIS_LENGTH              3
#define DEFAULT_AUTO_UPDATE_INTERVAL 900  // In seconds.
#define AUTO_UPDATE_INTERVAL         10   // In seconds.
#define STARTUP_UPDATE_DELAY         15.0 // In seconds.
#define TIMEZONE_OFFSET_LIMIT        6
#define CHANGE_EVENT_DELAY           250
#define FLAG_ICON_SUBFOLDER          "flags"
#define SEARCH_BOX_ACTION_NAME       "search"
#define HIGHLIGHTER_ACTION_NAME      "highlighter"
#define FILTER_ACTION_NAME           "filter"
#define SPACER_ACTION_NAME           "spacer"
#define SEPARATOR_ACTION_NAME        "separator"
#define FILTER_WIDTH                 125
#define FILTER_RIGHT_MARGIN          5
#define FEEDS_VIEW_INDENTATION       10
#define MIME_TYPE_ITEM_POINTER       "rssguard/itempointer"
#define DOWNLOADER_ICON_SIZE         48
#define ENCRYPTION_FILE_NAME         "key.private"
#define RELOAD_MODEL_BORDER_NUM      10
#define COOKIE_URL_IDENTIFIER        ":COOKIE:"
#define DEFAULT_NOTIFICATION_VOLUME  50
#define MAX_THREADPOOL_THREADS       32
#define WEB_BROWSER_SCROLL_STEP      50.0
#define MAX_NUMBER_OF_REDIRECTIONS   4

#define NOTIFICATIONS_MARGIN    16
#define NOTIFICATIONS_WIDTH     300
#define NOTIFICATIONS_TIMEOUT   15s
#define NOTIFICATIONS_PAGE_SIZE 10

#define GOOGLE_SEARCH_URL  "https://www.google.com/search?q=%1&ie=utf-8&oe=utf-8"
#define GOOGLE_SUGGEST_URL "http://suggestqueries.google.com/complete/search?output=toolbar&hl=en&q=%1"

#define EXTERNAL_TOOL_SEPARATOR "|||"

#define USER_DATA_PLACEHOLDER  "%data%"
#define SKIN_STYLE_PLACEHOLDER "%style%"

#define CLI_VER_SHORT "v"
#define CLI_VER_LONG  "version"

#define CLI_HELP_SHORT "h"
#define CLI_HELP_LONG  "help"

#define CLI_LOG_SHORT "l"
#define CLI_LOG_LONG  "log"

#define CLI_DAT_SHORT "d"
#define CLI_DAT_LONG  "data"

#define CLI_SIN_SHORT "s"
#define CLI_SIN_LONG  "no-single-instance"

#define CLI_USERAGENT_SHORT "u"
#define CLI_USERAGENT_LONG  "user-agent"

#define CLI_ADBLOCKPORT_SHORT "p"
#define CLI_ADBLOCKPORT_LONG  "adblock-port"

#define CLI_NSTDOUTERR_SHORT "n"
#define CLI_NSTDOUTERR_LONG  "no-standard-output"

#define CLI_STYLE_SHORT "t"
#define CLI_STYLE_LONG  "style"

#define CLI_NDEBUG_SHORT "g"
#define CLI_NDEBUG_LONG  "no-debug-output"

#define CLI_FORCE_LITE_SHORT "w"
#define CLI_FORCE_LITE_LONG  "lite"

#define CLI_QUIT_INSTANCE "q"
#define CLI_IS_RUNNING    "a"

#define CLI_THREADS "threads"

#define HTTP_HEADERS_ACCEPT         "Accept"
#define HTTP_HEADERS_CONTENT_TYPE   "Content-Type"
#define HTTP_HEADERS_CONTENT_LENGTH "Content-Length"
#define HTTP_HEADERS_AUTHORIZATION  "Authorization"
#define HTTP_HEADERS_USER_AGENT     "User-Agent"
#define HTTP_HEADERS_COOKIE         "Cookie"

#define LOGSEC_NETWORK        "network: "
#define LOGSEC_ADBLOCK        "adblock: "
#define LOGSEC_FEEDMODEL      "feed-model: "
#define LOGSEC_FEEDDOWNLOADER "feed-downloader: "
#define LOGSEC_MESSAGEMODEL   "message-model: "
#define LOGSEC_JS             "javascript: "
#define LOGSEC_GUI            "gui: "
#define LOGSEC_MPV            "libmpv: "
#define LOGSEC_QTMULTIMEDIA   "qtmultimedia: "
#define LOGSEC_NOTIFICATIONS  "notifications: "
#define LOGSEC_CORE           "core: "
#define LOGSEC_NODEJS         "nodejs: "
#define LOGSEC_DB             "database: "
#define LOGSEC_NEXTCLOUD      "nextcloud: "
#define LOGSEC_GREADER        "greader: "
#define LOGSEC_FEEDLY         "feedly: "
#define LOGSEC_TTRSS          "tt-rss: "
#define LOGSEC_GMAIL          "gmail: "
#define LOGSEC_OAUTH          "oauth: "
#define LOGSEC_REDDIT         "reddit: "
#define LOGSEC_NEWSBLUR       "newsblur: "

#define MAX_ZOOM_FACTOR     5.0f
#define MIN_ZOOM_FACTOR     0.25f
#define DEFAULT_ZOOM_FACTOR 1.0f
#define ZOOM_FACTOR_STEP    0.05f

#if defined(NO_LITE)
#define HTTP_COMPLETE_USERAGENT                                                           \
  (qApp->web()->engineProfile()->httpUserAgent().toLocal8Bit() + QByteArrayLiteral(" ") + \
   QByteArrayLiteral(APP_USERAGENT))
#else
#define HTTP_COMPLETE_USERAGENT                                                                         \
  (QByteArrayLiteral("Mozilla/5.0 (Windows NT 6.2; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) " \
                     "QtWebEngine/5.15.2 Chrome/83.0.4103.122 Safari/537.36 ") +                        \
   QByteArrayLiteral(APP_USERAGENT))
#endif

#define INTERNAL_URL_MESSAGE      "http://rssguard.message"
#define INTERNAL_URL_BLANK        "http://rssguard.blank"
#define INTERNAL_URL_INFO         "http://rssguard.info"
#define INTERNAL_URL_ADBLOCKED    "http://rssguard.adblocked"
#define INTERNAL_URL_MESSAGE_HOST "rssguard.message"

#define FEED_REGEX_MATCHER      "<link[^>]+type=\"application\\/(?:atom\\+xml|rss\\+xml|feed\\+json|json)\"[^>]*>"
#define FEED_HREF_REGEX_MATCHER "href=\"([^\"]+)\""

#define PLACEHOLDER_UNREAD_COUNTS "%unread"
#define PLACEHOLDER_ALL_COUNTS    "%all"

#define BACKUP_NAME_SETTINGS   "config"
#define BACKUP_SUFFIX_SETTINGS ".ini.backup"
#define BACKUP_NAME_DATABASE   "database"
#define BACKUP_SUFFIX_DATABASE ".db.backup"

#define APP_DB_MYSQL_DRIVER "QMYSQL"
#define APP_DB_MYSQL_INIT   "db_init_mysql.sql"
#define APP_DB_MYSQL_TEST   "MySQLTest"
#define APP_DB_MYSQL_PORT   3306

#define APP_DB_SQLITE_DRIVER "QSQLITE"
#define APP_DB_SQLITE_INIT   "db_init_sqlite.sql"
#define APP_DB_SQLITE_PATH   "database"
#define APP_DB_SQLITE_FILE   "database.db"

// Keep this in sync with schema versions declared in SQL initialization code.
#define APP_DB_SCHEMA_VERSION                "7"
#define APP_DB_UPDATE_FILE_PATTERN           "db_update_%1_%2_%3.sql"
#define APP_DB_COMMENT_SPLIT                 "-- !\n"
#define APP_DB_INCLUDE_PLACEHOLDER           "!!"
#define APP_DB_NAME_PLACEHOLDER              "##"
#define APP_DB_AUTO_INC_PRIM_KEY_PLACEHOLDER "$$"
#define APP_DB_BLOB_PLACEHOLDER              "^^"

#define APP_CFG_PATH "config"
#define APP_CFG_FILE "config.ini"

#define APP_SKIN_USER_FOLDER   "skins"
#define APP_SKIN_DEFAULT       "nudus-light"
#define APP_SKIN_METADATA_FILE "metadata.xml"
#define APP_STYLE_DEFAULT      "Fusion"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(FORCE_BUNDLE_ICONS)
#define APP_THEME_DEFAULT ""
#else
#define APP_THEME_DEFAULT "Breeze"
#endif

#if defined(FORCE_BUNDLE_ICONS)
// Forcibly bundle icons.
#define APP_THEME_DEFAULT "Breeze"
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#define APP_THEME_DEFAULT ""
#else
// Bundle icons otherwise.
#define APP_THEME_DEFAULT "Breeze"
#endif

#define APP_LOCAL_ICON_THEME_FOLDER "icons"
#define APP_NO_THEME                ""

// Indexes of columns as they are DEFINED IN THE TABLE for MESSAGES.
#define MSG_DB_ID_INDEX             0
#define MSG_DB_READ_INDEX           1
#define MSG_DB_IMPORTANT_INDEX      2
#define MSG_DB_DELETED_INDEX        3
#define MSG_DB_PDELETED_INDEX       4
#define MSG_DB_FEED_CUSTOM_ID_INDEX 5
#define MSG_DB_TITLE_INDEX          6
#define MSG_DB_URL_INDEX            7
#define MSG_DB_AUTHOR_INDEX         8
#define MSG_DB_DCREATED_INDEX       9
#define MSG_DB_CONTENTS_INDEX       10
#define MSG_DB_ENCLOSURES_INDEX     11
#define MSG_DB_SCORE_INDEX          12
#define MSG_DB_ACCOUNT_ID_INDEX     13
#define MSG_DB_CUSTOM_ID_INDEX      14
#define MSG_DB_CUSTOM_HASH_INDEX    15
#define MSG_DB_FEED_TITLE_INDEX     16
#define MSG_DB_FEED_IS_RTL_INDEX    17
#define MSG_DB_HAS_ENCLOSURES       18
#define MSG_DB_LABELS               19
#define MSG_DB_LABELS_IDS           20

// Indexes of columns as they are DEFINED IN THE TABLE for CATEGORIES.
#define CAT_DB_ID_INDEX          0
#define CAT_DB_ORDER_INDEX       1
#define CAT_DB_PARENT_ID_INDEX   2
#define CAT_DB_TITLE_INDEX       3
#define CAT_DB_DESCRIPTION_INDEX 4
#define CAT_DB_DCREATED_INDEX    5
#define CAT_DB_ICON_INDEX        6
#define CAT_DB_ACCOUNT_ID_INDEX  7
#define CAT_DB_CUSTOM_ID_INDEX   8

// Indexes of columns as they are DEFINED IN THE TABLE for FEEDS.
#define FDS_DB_ID_INDEX                        0
#define FDS_DB_ORDER_INDEX                     1
#define FDS_DB_TITLE_INDEX                     2
#define FDS_DB_DESCRIPTION_INDEX               3
#define FDS_DB_DCREATED_INDEX                  4
#define FDS_DB_ICON_INDEX                      5
#define FDS_DB_CATEGORY_INDEX                  6
#define FDS_DB_SOURCE_INDEX                    7
#define FDS_DB_UPDATE_TYPE_INDEX               8
#define FDS_DB_UPDATE_INTERVAL_INDEX           9
#define FDS_DB_IS_OFF_INDEX                    10
#define FDS_DB_IS_QUIET_INDEX                  11
#define FDS_DB_IS_RTL_INDEX                    12
#define FDS_DB_ADD_ANY_DATETIME_ARTICLES_INDEX 13
#define FDS_DB_DATETIME_TO_AVOID_INDEX         14
#define FDS_DB_OPEN_ARTICLES_INDEX             15
#define FDS_DB_ACCOUNT_ID_INDEX                16
#define FDS_DB_CUSTOM_ID_INDEX                 17
#define FDS_DB_CUSTOM_DATA_INDEX               18

// Indexes of columns for feed models.
#define FDS_MODEL_TITLE_INDEX  0
#define FDS_MODEL_COUNTS_INDEX 1

// Indexes of columns for message filter manager models.
#define MFM_MODEL_ISREAD      0
#define MFM_MODEL_ISIMPORTANT 1
#define MFM_MODEL_ISDELETED   2
#define MFM_MODEL_TITLE       3
#define MFM_MODEL_URL         4
#define MFM_MODEL_AUTHOR      5
#define MFM_MODEL_CREATED     6
#define MFM_MODEL_SCORE       7

#if defined(Q_OS_LINUX)
#define OS_ID "Linux"
#elif defined(Q_OS_FREEBSD)
#define OS_ID "FreeBSD"
#elif defined(Q_OS_NETBSD)
#define OS_ID "NetBSD"
#elif defined(Q_OS_OPENBSD)
#define OS_ID "OpenBSD"
#elif defined(Q_OS_OS2)
#define OS_ID "OS2"
#elif defined(Q_OS_MACOS)
#define OS_ID "macOS"
#elif defined(Q_OS_WIN)
#define OS_ID "Windows"
#elif defined(Q_OS_ANDROID)
#define OS_ID "Android"
#elif defined(Q_OS_UNIX)
#define OS_ID "Unix"
#else
#define OS_ID ""
#endif

// Paths.
#define APP_THEME_PATH QSL(":/graphics")
#define APP_SQL_PATH   QSL(":/sql")
#define APP_INFO_PATH  QSL(":/text")

#define WEB_UI_FOLDER QSL(":/scripts/web_ui")
#define WEB_UI_FILE   QSL("rssguard.html")

#define APP_ICON_PATH       QSL(":/graphics/rssguard.png")
#define APP_ICON_PLAIN_PATH QSL(":/graphics/rssguard_plain.png")

#define APP_ICON_MONO_PATH       QSL(":/graphics/rssguard_mono.png")
#define APP_ICON_MONO_PLAIN_PATH QSL(":/graphics/rssguard_plain_mono.png")

#define APP_SKIN_PATH          QSL(":/skins")
#define APP_INITIAL_FEEDS_PATH QSL(":/initial_feeds")
#define APP_LANG_PATH          QSL(":/localization")

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#define APP_DESKTOP_ENTRY_FILE "rssguard.desktop.in"

#define APP_DESKTOP_ENTRY_PATH QSL(":/desktop")
#endif

//
// Source code specific enhancements.
//
#if QT_VERSION >= 0x050E00 // Qt >= 5.14.0
#define FROM_STD_LIST(x, y)    (x(y.begin(), y.end()))
#define FROM_LIST_TO_SET(x, y) (x(y.begin(), y.end()))
#else
#define FROM_STD_LIST(x, y)    (x::fromStdList(y))
#define FROM_LIST_TO_SET(x, y) (x::fromList(y))
#endif

#ifndef qDebugNN
#define qDebugNN qDebug().noquote().nospace()
#endif

#ifndef qWarningNN
#define qWarningNN qWarning().noquote().nospace()
#endif

#ifndef qCriticalNN
#define qCriticalNN qCritical().noquote().nospace()
#endif

#ifndef qInfoNN
#define qInfoNN qInfo().noquote().nospace()
#endif

#define NONQUOTE_W_SPACE_DOT(x) " " << (x) << "."
#define NONQUOTE_W_SPACE(x)     " " << (x) << " "
#define QUOTE_W_SPACE_DOT(x)    " '" << (x) << "'."
#define QUOTE_W_SPACE_COMMA(x)  " '" << (x) << "',"
#define QUOTE_W_SPACE(x)        " '" << (x) << "' "
#define QUOTE_NO_SPACE(x)       "'" << (x) << "'"

#ifndef QSL

// Thin macro wrapper for literal strings.
// They are much more memory efficient and faster.
// Use it for all literals except for two cases:
//  a) Methods which take QLatin1String (use QLatin1String for literal argument too),
//  b) Construction of empty literals "", use QString() instead of QStringLiteral("").
#define QSL(x) QStringLiteral(x)
#endif

#ifndef QL1S

// Macro for latin strings. Latin strings are
// faster than QStrings created from literals.
#define QL1S(x) QLatin1String(x)
#endif

#ifndef QL1C

// Macro for latin chars.
#define QL1C(x) QLatin1Char(x)
#endif

#endif // DEFINITIONS_H
