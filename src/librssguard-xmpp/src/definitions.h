#ifndef XMPP_DEFINITIONS_H
#define XMPP_DEFINITIONS_H

#define XMPP_DEFAULT_BATCH_SIZE 100

// URLs.
#define XMPP_URL_REEDAH    "https://www.reedah.com"
#define XMPP_URL_TOR       "https://theoldreader.com"
#define XMPP_URL_BAZQUX    "https://bazqux.com"
#define XMPP_URL_INOREADER "https://www.inoreader.com"

// States.
#define XMPP_API_STATE_READING_LIST "state/com.google/reading-list"

// Means "read" message. If both "reading-list" and "read" are specified, message is READ. If this state
// is not present, message is UNREAD.
#define XMPP_API_STATE_READ      "state/com.google/read"
#define XMPP_API_STATE_IMPORTANT "state/com.google/starred"

#define XMPP_API_FULL_STATE_READING_LIST "user/-/state/com.google/reading-list"
#define XMPP_API_FULL_STATE_READ         "user/-/state/com.google/read"
#define XMPP_API_FULL_STATE_IMPORTANT    "user/-/state/com.google/starred"

// API.
#define XMPP_API_CLIENT_LOGIN        "accounts/ClientLogin"
#define XMPP_API_TAG_LIST            "reader/api/0/tag/list?output=json"
#define XMPP_API_SUBSCRIPTION_LIST   "reader/api/0/subscription/list?output=json"
#define XMPP_API_STREAM_CONTENTS     "reader/api/0/stream/contents/%1?output=json&n=%2"
#define XMPP_API_EDIT_TAG            "reader/api/0/edit-tag"
#define XMPP_API_SUBSCRIPTION_EXPORT "reader/api/0/subscription/export"
#define XMPP_API_SUBSCRIPTION_IMPORT "reader/api/0/subscription/import"
#define XMPP_API_SUBSCRIPTION_EDIT   "reader/api/0/subscription/edit?ac=%1&s=%2"
#define XMPP_API_ITEM_IDS            "reader/api/0/stream/items/ids?output=json&n=%2&s=%1"
#define XMPP_API_ITEM_CONTENTS       "reader/api/0/stream/items/contents?output=json&n=200000"
#define XMPP_API_TOKEN               "reader/api/0/token"
#define XMPP_API_USER_INFO           "reader/api/0/user-info?output=json"

// Edit subscription ops.
#define XMPP_API_EDIT_SUBSCRIPTION_ADD    "subscribe"
#define XMPP_API_EDIT_SUBSCRIPTION_MODIFY "edit"
#define XMPP_API_EDIT_SUBSCRIPTION_DELETE "unsubscribe"

// Misc.
#define XMPP_API_ITEM_IDS_MAX        200000
#define XMPP_API_EDIT_TAG_BATCH      200
#define XMPP_API_ITEM_CONTENTS_BATCH 999
#define XMPP_GLOBAL_UPDATE_THRES     0.3

// The Old Reader.
#define TOR_SPONSORED_STREAM_ID "tor/sponsored"
#define TOR_ITEM_CONTENTS_BATCH 9999

// Inoreader.
#define INO_ITEM_CONTENTS_BATCH 250

#define INO_HEADER_APPID  "AppId"
#define INO_HEADER_APPKEY "AppKey"

#define INO_OAUTH_REDIRECT_URI_PORT 14488
#define INO_OAUTH_SCOPE             "read write"
#define INO_OAUTH_TOKEN_URL         "https://www.inoreader.com/oauth2/token"
#define INO_OAUTH_AUTH_URL          "https://www.inoreader.com/oauth2/auth"
#define INO_REG_API_URL             "https://www.inoreader.com/developers/register-app"

// FreshRSS.
#define FRESHRSS_BASE_URL_PATH "api/xmpp.php/"

#endif // XMPP_DEFINITIONS_H
