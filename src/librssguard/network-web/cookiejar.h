// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QNetworkCookieJar>

class CookieJar : public QNetworkCookieJar {
  public:
    explicit CookieJar(QObject* parent = nullptr);

    QList<QNetworkCookie> extractCookiesFromUrl(const QString& url) const;
    bool insertCookies(const QList<QNetworkCookie>& cookies);
};

#endif // COOKIEJAR_H
