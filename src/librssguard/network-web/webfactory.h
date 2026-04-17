// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include "core/message.h"

#include <QMap>
#include <QObject>
#include <QWebEngineSettings>

class CookieJar;
class GeminiSchemeHandler;
class QMenu;
class QAction;
class QWebEngineProfile;

#if QT_VERSION_MAJOR < 6
class QWebEngineDownloadItem;
#else
class QWebEngineDownloadRequest;
#endif

class RSSGUARD_DLLSPEC WebFactory : public QObject {
    Q_OBJECT

  public:
    explicit WebFactory(QObject* parent = nullptr);
    virtual ~WebFactory();

    QWebEngineProfile* webEngineProfile() const;
    CookieJar* cookieJar() const;

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    QString urlToTld(const QUrl& url);

    QString webCacheFolder() const;

    // HTML entity unescaping. This method
    // converts both HTML entity names and numbers to UTF-8 string.
    static QString unescapeHtml(const QString& html);

    QString processFeedUriScheme(const QString& url);

    void updateWebEngineProfileSettings();
    void updateProxy();
    bool sendMessageViaEmail(const Message& message);

    QString customUserAgent() const;
    void setCustomUserAgent(const QString& user_agent);

    QList<QAction*> webEngineAttributeActions() const;

  public slots:
    bool openUrlInExternalBrowser(const QUrl& url) const;
    bool openUrlInExternalBrowser(const QUrl& url, bool use_external_tools) const;

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

    static QMap<QString, char16_t> generateUnescapes();

    QString m_customUserAgent;
    QWebEngineProfile* m_webEngineProfile;
    CookieJar* m_cookieJar;
    GeminiSchemeHandler* m_geminiHandler;
    QList<QAction*> m_webEngineAttributeActions;
};

#endif // WEBFACTORY_H
