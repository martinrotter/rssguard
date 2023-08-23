// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSS_DEFINITIONS_H
#define TTRSS_DEFINITIONS_H

#define TTRSS_MINIMAL_API_LEVEL 9
#define TTRSS_CONTENT_TYPE_JSON      "application/json; charset=utf-8"

///
/// Errors.
///
#define TTRSS_NOT_LOGGED_IN     "NOT_LOGGED_IN"   // Error when user needs to login before making an operation.
#define TTRSS_UNKNOWN_METHOD    "UNKNOWN_METHOD"  // Given "op" is not recognized.
#define TTRSS_INCORRECT_USAGE   "INCORRECT_USAGE" // Given "op" was used with bad parameters.

// Limitations
#define TTRSS_DEFAULT_MESSAGES  100
#define TTRSS_MAX_MESSAGES      200

// General return status codes.
#define TTRSS_API_STATUS_OK       0
#define TTRSS_API_STATUS_ERR      1
#define TTRSS_CONTENT_NOT_LOADED  -1

// Login.
#define TTRSS_API_DISABLED      "API_DISABLED"    // API is not enabled.
#define TTRSS_LOGIN_ERROR       "LOGIN_ERROR"     // Incorrect password/username.

// Get feed tree.
#define TTRSS_GFT_TYPE_CATEGORY "category"

// "Published" feed/label.
#define TTRSS_PUBLISHED_LABEL_ID    -2
#define TTRSS_PUBLISHED_FEED_ID     0

// Subscribe to feed.
#define STF_UNKNOWN          -1
#define STF_EXISTS            0
#define STF_INVALID_URL       2
#define STF_UNREACHABLE_URL   5
#define STF_URL_NO_FEED       3
#define STF_URL_MANY_FEEDS    4
#define STF_INSERTED          1

// Unsubscribe from feed.
#define UFF_FEED_NOT_FOUND    "FEED_NOT_FOUND"
#define UFF_OK                "OK"

#endif // TTRSS_DEFINITIONS_H
