// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include "core/message.h"

#include <QMap>
#include <QObject>

class QMenu;
class CookieJar;
class ApiServer;

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
    //   â€ = &forall; (entity name), &#8704; (base-10 entity), &#x2200; (base-16 entity)
    static QString unescapeHtml(const QString& html);

    QString limitSizeOfHtmlImages(const QString& html, int desired_width, int images_max_height) const;
    QString processFeedUriScheme(const QString& url);

    CookieJar* cookieJar() const;

    void startApiServer();
    void stopApiServer();

    void updateProxy();
    bool sendMessageViaEmail(const Message& message);

    QString customUserAgent() const;
    void setCustomUserAgent(const QString& user_agent);

  public slots:
    bool openUrlInExternalBrowser(const QUrl& url) const;

  private:
    static QMap<QString, char16_t> generateUnescapes();

    ApiServer* m_apiServer;
    CookieJar* m_cookieJar;
    QString m_customUserAgent;
};

#endif // WEBFACTORY_H
