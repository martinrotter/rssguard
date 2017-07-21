// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef TTRSS_DEFINITIONS_H
#define TTRSS_DEFINITIONS_H

#define MINIMAL_API_LEVEL 9
#define CONTENT_TYPE      "application/json; charset=utf-8"

///
/// Errors.
///
#define NOT_LOGGED_IN     "NOT_LOGGED_IN"   // Error when user needs to login before making an operation.
#define UNKNOWN_METHOD    "UNKNOWN_METHOD"  // Given "op" is not recognized.
#define INCORRECT_USAGE   "INCORRECT_USAGE" // Given "op" was used with bad parameters.

// Limitations
#define MAX_MESSAGES      200

// General return status codes.
#define API_STATUS_OK     0
#define API_STATUS_ERR    1
#define STATUS_OK         "OK"

#define CONTENT_NOT_LOADED  -1

// Login.
#define API_DISABLED      "API_DISABLED"    // API is not enabled.
#define LOGIN_ERROR       "LOGIN_ERROR"     // Incorrect password/username.

// Logout.
#define LOGOUT_OK         "OK"

// Get feed tree.
#define GFT_TYPE_CATEGORY "category"

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
