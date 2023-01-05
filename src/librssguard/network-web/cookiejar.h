// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QNetworkCookieJar>

#include <QReadWriteLock>

#if defined(USE_WEBENGINE)
class QWebEngineCookieStore;
#endif

class CookieJar : public QNetworkCookieJar {
  public:
    explicit CookieJar(QObject* parent = nullptr);

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const;
    virtual bool setCookiesFromUrl(const QList<QNetworkCookie>& cookie_list, const QUrl& url);
    virtual bool insertCookie(const QNetworkCookie& cookie);
    virtual bool updateCookie(const QNetworkCookie& cookie);
    virtual bool deleteCookie(const QNetworkCookie& cookie);
    // virtual bool validateCookie(const QNetworkCookie& cookie, const QUrl& url) const;

    void updateSettings();

  public:
    static QList<QNetworkCookie> extractCookiesFromUrl(const QString& url);

  private:
    bool insertCookieInternal(const QNetworkCookie& cookie, bool notify_others, bool should_save);
    bool updateCookieInternal(const QNetworkCookie& cookie, bool notify_others);
    bool deleteCookieInternal(const QNetworkCookie& cookie, bool notify_others);

    void loadCookies();
    void saveCookies();

  private:
#if defined(USE_WEBENGINE)
    QWebEngineCookieStore* m_webEngineCookies;
#endif

    mutable QReadWriteLock m_lock{QReadWriteLock::RecursionMode::Recursive};
    bool m_ignoreAllCookies;
};

#endif // COOKIEJAR_H
