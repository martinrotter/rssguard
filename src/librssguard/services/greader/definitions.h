#ifndef GREADER_DEFINITIONS_H
#define GREADER_DEFINITIONS_H

#define GREADER_UNLIMITED_BATCH_SIZE       -1

// States.

// Means "unread" message.
#define GREADER_API_STATE_READING_LIST    "state/com.google/reading-list"

// Means "read" message. If both "reading-list" and "read" are specified, message is READ.
#define GREADER_API_STATE_READ            "state/com.google/read"

#define GREADER_API_STATE_IMPORTANT       "state/com.google/starred"

#define GREADER_API_FULL_STATE_READING_LIST    "user/-/state/com.google/reading-list"
#define GREADER_API_FULL_STATE_READ            "user/-/state/com.google/read"
#define GREADER_API_FULL_STATE_IMPORTANT       "user/-/state/com.google/starred"

// API.
#define GREADER_API_CLIENT_LOGIN        "accounts/ClientLogin"
#define GREADER_API_TAG_LIST            "reader/api/0/tag/list?output=json"
#define GREADER_API_SUBSCRIPTION_LIST   "reader/api/0/subscription/list?output=json"
#define GREADER_API_STREAM_CONTENTS     "reader/api/0/stream/contents/%1?output=json&n=%2"
#define GREADER_API_EDIT_TAG            "reader/api/0/edit-tag"
#define GREADER_API_TOKEN               "reader/api/0/token"

// Misc.
#define GREADER_API_EDIT_TAG_BATCH      200

// The Old Reader.
#define TOR_SPONSORED_STREAM_ID   "tor/sponsored"

// FreshRSS.
#define FRESHRSS_BASE_URL_PATH    "api/greader.php/"

#endif // GREADER_DEFINITIONS_H
