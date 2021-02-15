#ifndef FEEDLY_DEFINITIONS_H
#define FEEDLY_DEFINITIONS_H

#define FEEDLY_UNLIMITED_BATCH_SIZE       -1
#define FEEDLY_MAX_BATCH_SIZE             999

#define FEEDLY_GENERATE_DAT               "https://feedly.com/v3/auth/dev"

#define FEEDLY_API_REDIRECT_URI_PORT      8080
#define FEEDLY_API_SCOPE                  "https://cloud.feedly.com/subscriptions"

#if defined(NDEBUG)
#define FEEDLY_API_URL_BASE       "https://sandbox7.feedly.com/v3/"
#else
#define FEEDLY_API_URL_BASE       "https://cloud.feedly.com/v3/"
#endif

#define FEEDLY_API_URL_AUTH       "auth/auth"
#define FEEDLY_API_URL_TOKEN      "auth/token"
#define FEEDLY_API_URL_PROFILE    "profile"
#define FEEDLY_API_URL_COLLETIONS "collections"

#endif // FEEDLY_DEFINITIONS_H
