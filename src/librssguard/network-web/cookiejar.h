// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QNetworkCookieJar>

#include "miscellaneous/autosaver.h"

#include <QReadWriteLock>

#if defined(USE_WEBENGINE)
class QWebEngineCookieStore;
#endif

class CookieJar : public QNetworkCookieJar {
    Q_OBJECT

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

  public slots:
    void loadCookies();
    void saveCookies();

  private:
    bool insertCookieInternal(const QNetworkCookie& cookie, bool notify_others, bool should_save);
    bool updateCookieInternal(const QNetworkCookie& cookie, bool notify_others);
    bool deleteCookieInternal(const QNetworkCookie& cookie, bool notify_others);

  private:
#if defined(USE_WEBENGINE)
    QWebEngineCookieStore* m_webEngineCookies;
#endif

    mutable QReadWriteLock m_lock{QReadWriteLock::RecursionMode::Recursive};
    bool m_ignoreAllCookies;
    AutoSaver m_saver;
};

#endif // COOKIEJAR_H
