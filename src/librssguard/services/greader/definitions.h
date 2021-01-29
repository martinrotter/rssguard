#ifndef GREADER_DEFINITIONS_H
#define GREADER_DEFINITIONS_H

#define GREADER_UNLIMITED_BATCH_SIZE       -1

// FreshRSS.
#define FRESHRSS_BASE_URL_PATH    "api/greader.php/"

// API.
#define GREADER_API_CLIENT_LOGIN        "accounts/ClientLogin?Email=%1&Passwd=%2"
#define GREADER_API_TAG_LIST            "reader/api/0/tag/list?output=json"
#define GREADER_API_SUBSCRIPTION_LIST   "reader/api/0/subscription/list?output=json"
#define GREADER_API_STREAM_CONTENTS     "reader/api/0/stream/contents/%1?output=json&n=%2"

#endif // GREADER_DEFINITIONS_H
