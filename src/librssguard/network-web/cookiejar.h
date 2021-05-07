// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QNetworkCookieJar>

class CookieJar : public QNetworkCookieJar {
  public:
    explicit CookieJar(QObject* parent = nullptr);

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const;
    virtual bool setCookiesFromUrl(const QList<QNetworkCookie>& cookie_list, const QUrl& url);
    virtual bool insertCookie(const QNetworkCookie& cookie);
    virtual bool updateCookie(const QNetworkCookie& cookie);
    virtual bool deleteCookie(const QNetworkCookie& cookie);
    static QList<QNetworkCookie> extractCookiesFromUrl(const QString& url);

  private:
    void loadCookies();
    void saveCookies();
};

#endif // COOKIEJAR_H
