// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/cookiejar.h"

#include "definitions/definitions.h"

#include <QDateTime>
#include <QNetworkCookie>

CookieJar::CookieJar(QObject* parent) : QNetworkCookieJar(parent) {}

QList<QNetworkCookie> CookieJar::extractCookiesFromUrl(const QString& url) const {
  if (!url.contains(QSL(COOKIE_URL_IDENTIFIER))) {
    return {};
  }

  int index = url.lastIndexOf(QSL(COOKIE_URL_IDENTIFIER), -1, Qt::CaseSensitivity::CaseInsensitive);
  QString cookies_string = url.right(url.length() - index - QSL(COOKIE_URL_IDENTIFIER).size());
  QStringList cookies_list = cookies_string.split(';');
  QList<QNetworkCookie> cookies;

  for (const QString& single_cookie : cookies_list) {
    const QList<QNetworkCookie>& extracted_cookies = QNetworkCookie::parseCookies(single_cookie.toUtf8());

    if (extracted_cookies.isEmpty()) {
      continue;
    }

    QNetworkCookie cookie = extracted_cookies.at(0);
    QDateTime date = QDateTime::currentDateTime();

    date = date.addYears(30);
    cookie.setExpirationDate(date);
    cookies.append(cookie);
  }

  return cookies;
}

bool CookieJar::insertCookies(const QList<QNetworkCookie>& cookies) {
  bool result = true;

  for (const QNetworkCookie& cookie : cookies) {
    result &= insertCookie(cookie);
  }

  return result;
}
