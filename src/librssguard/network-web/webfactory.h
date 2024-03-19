// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include "core/message.h"

#include <QMap>
#include <QObject>

#if defined(NO_LITE)
class QWebEngineProfile;
class QWebEngineSettings;
class QAction;
class NetworkUrlInterceptor;
#endif

class QMenu;
class AdBlockManager;
class CookieJar;
class ApiServer;
class Readability;

class RSSGUARD_DLLSPEC WebFactory : public QObject {
    Q_OBJECT

  public:
    explicit WebFactory(QObject* parent = nullptr);
    virtual ~WebFactory();

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

    // HTML entity unescaping. This method
    // converts both HTML entity names and numbers to UTF-8 string.
    // Example of entities are:
    //   ∀ = &forall; (entity name), &#8704; (base-10 entity), &#x2200; (base-16 entity)
    static QString unescapeHtml(const QString& html);

    QString limitSizeOfHtmlImages(const QString& html, int desired_width, int images_max_height) const;
    QString processFeedUriScheme(const QString& url);

    AdBlockManager* adBlock() const;

#if defined(NO_LITE)
    QAction* engineSettingsAction();
    NetworkUrlInterceptor* urlIinterceptor() const;
    QWebEngineProfile* engineProfile() const;
#endif

    CookieJar* cookieJar() const;
    Readability* readability() const;

    void startApiServer();
    void stopApiServer();

    void updateProxy();
    bool sendMessageViaEmail(const Message& message);

#if defined(NO_LITE)
    void loadCustomCss(const QString user_styles_path);
#endif

    QString customUserAgent() const;
    void setCustomUserAgent(const QString& user_agent);

  public slots:
#if defined(NO_LITE)
    void cleanupCache();
#endif

    bool openUrlInExternalBrowser(const QUrl& url) const;

#if defined(NO_LITE)
  private slots:
    void createMenu(QMenu* menu = nullptr);
    void webEngineSettingChanged(bool enabled);

  private:
    QAction* createEngineSettingsAction(const QString& title, int web_attribute);
#endif

  private:
    static QMap<QString, char16_t> generateUnescapes();

  private:
    AdBlockManager* m_adBlock;

#if defined(NO_LITE)
    QWebEngineProfile* m_engineProfile;
    NetworkUrlInterceptor* m_urlInterceptor;
    QAction* m_engineSettings;
#endif

    ApiServer* m_apiServer;
    CookieJar* m_cookieJar;
    Readability* m_readability;
    QString m_customUserAgent;
};

#endif // WEBFACTORY_H
