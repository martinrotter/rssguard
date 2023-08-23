// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAIL_DEFINITIONS_H
#define GMAIL_DEFINITIONS_H

#define GMAIL_OAUTH_REDIRECT_URI_PORT 14499
#define GMAIL_OAUTH_AUTH_URL "https://accounts.google.com/o/oauth2/auth"
#define GMAIL_OAUTH_TOKEN_URL "https://accounts.google.com/o/oauth2/token"
#define GMAIL_OAUTH_SCOPE "https://mail.google.com/"

#define GMAIL_REG_API_URL "https://console.developers.google.com/apis/credentials"

#define GMAIL_API_SEND_MESSAGE "https://www.googleapis.com/upload/gmail/v1/users/me/messages/send?uploadType=media"
#define GMAIL_API_BATCH_UPD_LABELS "https://www.googleapis.com/gmail/v1/users/me/messages/batchModify"
#define GMAIL_API_GET_PROFILE "https://gmail.googleapis.com/gmail/v1/users/me/profile"
#define GMAIL_API_GET_ATTACHMENT "https://www.googleapis.com/gmail/v1/users/me/messages/%1/attachments/%2"
#define GMAIL_API_LABELS_LIST "https://www.googleapis.com/gmail/v1/users/me/labels"
#define GMAIL_API_MSGS_LIST "https://www.googleapis.com/gmail/v1/users/me/messages"
#define GMAIL_API_BATCH "https://www.googleapis.com/batch/gmail/v1"

#define GMAIL_ATTACHMENT_SEP "####"

#define GMAIL_DEFAULT_BATCH_SIZE 100
#define GMAIL_MAX_BATCH_SIZE 999

#define GMAIL_LABEL_TYPE_USER "user"

#define GMAIL_SYSTEM_LABEL_UNREAD "UNREAD"
#define GMAIL_SYSTEM_LABEL_INBOX "INBOX"
#define GMAIL_SYSTEM_LABEL_SENT "SENT"
#define GMAIL_SYSTEM_LABEL_DRAFT "DRAFT"
#define GMAIL_SYSTEM_LABEL_SPAM "SPAM"
#define GMAIL_SYSTEM_LABEL_STARRED "STARRED"
#define GMAIL_SYSTEM_LABEL_TRASH "TRASH"

#define GMAIL_CONTENT_TYPE_HTTP "application/http"
#define GMAIL_CONTENT_TYPE_JSON "application/json"

enum class RecipientType { To, Cc, Bcc, ReplyTo };

#endif // GMAIL_DEFINITIONS_H
