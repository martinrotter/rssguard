#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define MINIMAL_API_LEVEL 10
#define CONTENT_TYPE      "application/json; charset=utf-8"

// Error when user needs to login before making an operation.
#define NOT_LOGGED_IN     "NOT_LOGGED_IN"

// General return status codes.
#define API_STATUS_OK     0
#define API_STATUS_ERR    1
#define STATUS_OK         "OK"

// Login.
#define API_DISABLED      "API_DISABLED"    // API is not enabled.
#define LOGIN_ERROR       "LOGIN_ERROR"     // Incorrect password/username.

// Logout.
#define LOGOUT_OK         "OK"


/* //login
 *   QtJson::JsonObject obj;
  obj["op"] = "login";
  obj["user"] = "admin";
  obj["password"] = "Zy69tKWF";

  QByteArray arr;
  NetworkResult res = NetworkFactory::uploadData("http://rss.rotterovi.eu/api/",
                                                 15000,
                                                 QtJson::serialize(obj),
                                                 "application/json; charset=utf-8",
                                                 arr);

  obj = QtJson::parse(QString::fromUtf8(arr)).toMap();*/

#endif // DEFINITIONS_H

