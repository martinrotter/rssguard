// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDIT_DEFINITIONS_H
#define REDDIT_DEFINITIONS_H

#define REDDIT_OAUTH_REDIRECT_URI_PORT 14499
#define REDDIT_OAUTH_AUTH_URL          "https://www.reddit.com/api/v1/authorize"
#define REDDIT_OAUTH_TOKEN_URL         "https://www.reddit.com/api/v1/access_token"
#define REDDIT_OAUTH_SCOPE             "identity mysubreddits read"

#define REDDIT_REG_API_URL "https://www.reddit.com/prefs/apps"

#define REDDIT_API_GET_PROFILE "https://oauth.reddit.com/api/v1/me"
#define REDDIT_API_SUBREDDITS  "https://oauth.reddit.com/subreddits/mine/subscriber?limit=%1"
#define REDDIT_API_HOT         "https://oauth.reddit.com%1hot?limit=%2&count=%3&g=%4"

#define REDDIT_DEFAULT_BATCH_SIZE 100
#define REDDIT_MAX_BATCH_SIZE     999

#define REDDIT_CONTENT_TYPE_HTTP "application/http"
#define REDDIT_CONTENT_TYPE_JSON "application/json"

#endif // REDDIT_DEFINITIONS_H
