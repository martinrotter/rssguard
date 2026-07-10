// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include "core/message.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMap>
#include <QNetworkProxy>
#include <QObject>
#include <QStringList>
#include <QUrl>

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
#include "network-web/httpserver.h"

#include <QWebEngineSettings>

#define PAC_SERVER_PORT 42751
#define PAC_SERVER_FILE "proxies.pac"

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
class ServiceRoot;

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
class PacServer : public HttpServer {
  protected:
    virtual void answerClient(QTcpSocket* socket, const HttpRequest& request);
};
#endif

class RSSGUARD_DLLSPEC WebFactory : public QObject {
    Q_OBJECT

  public:
    explicit WebFactory(QObject* parent = nullptr);
    virtual ~WebFactory();

    CookieJar* cookieJar() const;

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    QString urlToTld(const QUrl& url);

    static QString webCacheFolder();

    QStringList extractAllHyperlinks(const QUrl& base_url, const QByteArray& html_data);

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
    bool openUrlInExternalBrowser(const QList<QUrl>& urls, bool use_external_tools, bool can_bring_forward_after) const;
    bool downloadUrlToFile(const QUrl& url) const;

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  public:
    QWebEngineProfile* webEngineProfile() const;
    QList<QAction*> webEngineAttributeActions() const;

    void updateWebEngineProfileSettings();
    void generatePacAndStartServer(const QList<ServiceRoot*>& accounts);

    static QString proxiesPacFilePath();
    static QString injectPacIntoChromiumFlags(const QString& cli_flags, const QString& user_flags);

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
#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
    QByteArray generatePacFile(const QHash<QString, QNetworkProxy>& proxies_per_host);
#endif

    static QMap<QString, char16_t> generateUnescapes();

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
    QWebEngineProfile* m_webEngineProfile;
    GeminiSchemeHandler* m_geminiHandler;
    QList<QAction*> m_webEngineAttributeActions;
    PacServer m_pacServer;
#endif

    QString m_customUserAgent;
    CookieJar* m_cookieJar;
};

#endif // WEBFACTORY_H
