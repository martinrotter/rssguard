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

#ifndef INOREADER_DEFINITIONS_H
#define INOREADER_DEFINITIONS_H

#define INOREADER_OAUTH_SCOPE           "read write"
#define INOREADER_OAUTH_TOKEN_URL       "https://www.inoreader.com/oauth2/token"
#define INOREADER_OAUTH_AUTH_URL        "https://www.inoreader.com/oauth2/auth"
#define INOREADER_OAUTH_CLI_ID          "1000000604"
#define INOREADER_OAUTH_CLI_KEY         "gsStoZ3aAoQJCgQxoFSuXkWI7Sly87yK"
#define INOREADER_REFRESH_TOKEN_KEY     "refresh_token"
#define INOREADER_ACCESS_TOKEN_KEY      "access_token"
#define INOREADER_DEFAULT_BATCH_SIZE    100
#define INOREADER_UNLIMITED_BATCH_SIZE  -1

#define INOREADER_API_FEED_CONTENTS     "https://www.inoreader.com/reader/api/0/stream/contents"
#define INOREADER_API_LIST_LABELS       "https://www.inoreader.com/reader/api/0/tag/list"
#define INOREADER_API_LIST_FEEDS        "https://www.inoreader.com/reader/api/0/subscription/list"

#endif // INOREADER_DEFINITIONS_H
