// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADER_DEFINITIONS_H
#define INOREADER_DEFINITIONS_H

#define INOREADER_OAUTH_SCOPE           "read write"
#define INOREADER_OAUTH_TOKEN_URL       "https://www.inoreader.com/oauth2/token"
#define INOREADER_OAUTH_AUTH_URL        "https://www.inoreader.com/oauth2/auth"
#define INOREADER_REG_API_URL           "https://www.inoreader.com/developers/register-app"

#define INOREADER_OAUTH_CLI_ID          "999999019"
#define INOREADER_OAUTH_CLI_KEY         "k4bkOJ5Jj1erc52tmniluKtU6lZdNoZc"

#define INOREADER_REFRESH_TOKEN_KEY     "refresh_token"
#define INOREADER_ACCESS_TOKEN_KEY      "access_token"

#define INOREADER_DEFAULT_BATCH_SIZE    100
#define INOREADER_MAX_BATCH_SIZE        999
#define INOREADER_MIN_BATCH_SIZE        20
#define INOREADER_API_EDIT_TAG_BATCH    50

#define INOREADER_STATE_READING_LIST    "state/com.google/reading-list"
#define INOREADER_STATE_READ            "state/com.google/read"
#define INOREADER_STATE_IMPORTANT       "state/com.google/starred"

#define INOREADER_FULL_STATE_READING_LIST    "user/-/state/com.google/reading-list"
#define INOREADER_FULL_STATE_READ            "user/-/state/com.google/read"
#define INOREADER_FULL_STATE_IMPORTANT       "user/-/state/com.google/starred"

#define INOREADER_API_FEED_CONTENTS     "https://www.inoreader.com/reader/api/0/stream/contents"
#define INOREADER_API_LIST_LABELS       "https://www.inoreader.com/reader/api/0/tag/list?types=1"
#define INOREADER_API_LIST_FEEDS        "https://www.inoreader.com/reader/api/0/subscription/list"
#define INOREADER_API_EDIT_TAG          "https://www.inoreader.com/reader/api/0/edit-tag"

#endif // INOREADER_DEFINITIONS_H
