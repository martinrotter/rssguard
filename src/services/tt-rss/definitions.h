#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define MINIMAL_API_LEVEL 10
#define CONTENT_TYPE      "application/json; charset=utf-8"

///
/// Errors.
///
#define NOT_LOGGED_IN     "NOT_LOGGED_IN"   // Error when user needs to login before making an operation.
#define UNKNOWN_METHOD    "UNKNOWN_METHOD"  // Given "op" is not recognized.
#define INCORRECT_USAGE   "INCORRECT_USAGE" // Given "op" was used with bad parameters.

// General return status codes.
#define API_STATUS_OK     0
#define API_STATUS_ERR    1
#define STATUS_OK         "OK"

// Login.
#define API_DISABLED      "API_DISABLED"    // API is not enabled.
#define LOGIN_ERROR       "LOGIN_ERROR"     // Incorrect password/username.

// Logout.
#define LOGOUT_OK         "OK"

#endif // DEFINITIONS_H
