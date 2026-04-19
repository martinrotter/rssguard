// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include "core/message.h"

#include <QMap>
#include <QObject>

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include <QWebEngineSettings>

class QWebEngineProfile;
class GeminiSchemeHandler;

#if QT_VERSION_MAJOR < 6
class QWebEngineDownloadItem;
#else
class QWebEngineDownloadRequest;
#endif
#endif

class CookieJar;
class QMenu;
class QAction;

class RSSGUARD_DLLSPEC WebFactory : public QObject {
    Q_OBJECT

  public:
    explicit WebFactory(QObject* parent = nullptr);
    virtual ~WebFactory();

    CookieJar* cookieJar() const;

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    QString urlToTld(const QUrl& url);

    QString webCacheFolder() const;

    // HTML entity unescaping. This method
    // converts both HTML entity names and numbers to UTF-8 string.
    static QString unescapeHtml(const QString& html);

    QString processFeedUriScheme(const QString& url);

    void updateProxy();
    bool sendMessageViaEmail(const Message& message);

    QString customUserAgent() const;
    void setCustomUserAgent(const QString& user_agent);

  public slots:
    bool openUrlInExternalBrowser(const QUrl& url) const;
    bool openUrlInExternalBrowser(const QUrl& url, bool use_external_tools) const;

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  public:
    QWebEngineProfile* webEngineProfile() const;
    void updateWebEngineProfileSettings();
    QList<QAction*> webEngineAttributeActions() const;

  private slots:
    void onWebEngineAttributeChanged(bool enabled);
    void onClearHttpCacheCompleted();

#if QT_VERSION_MAJOR < 6
    void onDownloadRequested(QWebEngineDownloadItem* download);
#else
    void onDownloadRequested(QWebEngineDownloadRequest* download);
#endif

  private:
    void initializeWebEngineProfile();
    void initializeWebEngineAttributeActions();
    QAction* createEngineSettingsAction(QObject* parent,
                                        const QString& title,
                                        QWebEngineSettings::WebAttribute web_attribute);

    static bool isByDefaultDisabledWebEngineAttribute(QWebEngineSettings::WebAttribute web_attribute);
#endif

  private:
    static QMap<QString, char16_t> generateUnescapes();

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
    QWebEngineProfile* m_webEngineProfile;
    GeminiSchemeHandler* m_geminiHandler;
    QList<QAction*> m_webEngineAttributeActions;
#endif

    QString m_customUserAgent;
    CookieJar* m_cookieJar;
};

#endif // WEBFACTORY_H
