// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include "miscellaneous/autosaver.h"

#include <QNetworkCookieJar>
#include <QPointer>
#include <QReadWriteLock>

class WebFactory;
class SharedCookieJarProxy;

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
class QWebEngineCookieStore;
#endif

class CookieJar : public QNetworkCookieJar {
    Q_OBJECT

  public:
    explicit CookieJar(WebFactory* parent = nullptr);
    virtual ~CookieJar();

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
    void clearCookies();
    void loadCookies();
    void saveCookies();

  private:
    friend class SharedCookieJarProxy;

    QList<QNetworkCookie> cookiesForUrlUnrestricted(const QUrl& url) const;
    bool setCookiesFromUrlUnrestricted(const QList<QNetworkCookie>& cookie_list, const QUrl& url);
    bool insertCookieUnrestricted(const QNetworkCookie& cookie);
    bool updateCookieUnrestricted(const QNetworkCookie& cookie);
    bool deleteCookieUnrestricted(const QNetworkCookie& cookie);

    bool insertCookieInternal(const QNetworkCookie& cookie, bool notify_others, bool should_save);
    bool updateCookieInternal(const QNetworkCookie& cookie, bool notify_others);
    bool deleteCookieInternal(const QNetworkCookie& cookie, bool notify_others);

  private:
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
    QWebEngineCookieStore* m_webEngineCookies;
#endif

    mutable QReadWriteLock m_lock{QReadWriteLock::RecursionMode::Recursive};
    bool m_ignoreAllCookies;
    bool m_bypassGlobalPolicy;
    AutoSaver m_saver;
};

class SharedCookieJarProxy final : public QNetworkCookieJar {
  public:
    explicit SharedCookieJarProxy(CookieJar* shared_jar, QObject* parent = nullptr);

    QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const override;
    bool setCookiesFromUrl(const QList<QNetworkCookie>& cookie_list, const QUrl& url) override;
    bool insertCookie(const QNetworkCookie& cookie) override;
    bool updateCookie(const QNetworkCookie& cookie) override;
    bool deleteCookie(const QNetworkCookie& cookie) override;

  private:
    QPointer<CookieJar> m_sharedJar;
};

class DiscardingCookieJar : public QNetworkCookieJar {
  public:
    explicit DiscardingCookieJar(QObject* parent = nullptr);

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const;
    virtual bool setCookiesFromUrl(const QList<QNetworkCookie>& cookie_list, const QUrl& url);
    virtual bool insertCookie(const QNetworkCookie& cookie);
    virtual bool updateCookie(const QNetworkCookie& cookie);
};

#endif // COOKIEJAR_H
