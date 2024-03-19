#ifndef FEEDLY_DEFINITIONS_H
#define FEEDLY_DEFINITIONS_H

#define FEEDLY_DEFAULT_BATCH_SIZE 100
#define FEEDLY_MAX_BATCH_SIZE     500
#define FEEDLY_MAX_TOTAL_SIZE     5000
#define FEEDLY_UNTAG_BATCH_SIZE   100

#define FEEDLY_GENERATE_DAT "https://feedly.com/v3/auth/dev"

#define FEEDLY_API_REDIRECT_URI_PORT 14466
#define FEEDLY_API_SCOPE             "https://cloud.feedly.com/subscriptions"

// #define FEEDLY_API_URL_BASE       "https://sandbox7.feedly.com/v3/"
#define FEEDLY_API_URL_BASE "https://cloud.feedly.com/v3/"

#define FEEDLY_API_SYSTEM_TAG_READ  "global.read"
#define FEEDLY_API_SYSTEM_TAG_SAVED "global.saved"

#define FEEDLY_MARKERS_READ        "markAsRead"
#define FEEDLY_MARKERS_UNREAD      "keepUnread"
#define FEEDLY_MARKERS_IMPORTANT   "markAsSaved"
#define FEEDLY_MARKERS_UNIMPORTANT "markAsUnsaved"

#define FEEDLY_API_URL_AUTH            "auth/auth"
#define FEEDLY_API_URL_TOKEN           "auth/token"
#define FEEDLY_API_URL_PROFILE         "profile"
#define FEEDLY_API_URL_COLLETIONS      "collections"
#define FEEDLY_API_URL_TAGS            "tags"
#define FEEDLY_API_URL_STREAM_CONTENTS "streams/contents?streamId=%1"
#define FEEDLY_API_URL_STREAM_IDS      "streams/%1/ids"
#define FEEDLY_API_URL_MARKERS         "markers"
#define FEEDLY_API_URL_ENTRIES         "entries/.mget"

#endif // FEEDLY_DEFINITIONS_H
