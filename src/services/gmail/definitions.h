// For license of this file, see <object-root-folder>/LICENSE.md.

#ifndef GMAIL_DEFINITIONS_H
#define GMAIL_DEFINITIONS_H

#define GMAIL_OAUTH_AUTH_URL      "https://accounts.google.com/o/oauth2/auth"
#define GMAIL_OAUTH_TOKEN_URL     "https://accounts.google.com/o/oauth2/token"
#define GMAIL_OAUTH_SCOPE         "https://mail.google.com/"

#define GMAIL_API_LABELS_LIST     "https://www.googleapis.com/gmail/v1/users/me/labels"
#define GMAIL_API_MSGS_LIST       "https://www.googleapis.com/gmail/v1/users/me/messages"
#define GMAIL_API_BATCH           "https://www.googleapis.com/batch"

#define GMAIL_DEFAULT_BATCH_SIZE  50
#define GMAIL_MAX_BATCH_SIZE      999
#define GMAIL_MIN_BATCH_SIZE      20

#define GMAIL_CONTENT_TYPE_HTTP   "application/http"

#endif // GMAIL_DEFINITIONS_H
