// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBFACTORY_H
#define WEBFACTORY_H

#include "core/message.h"

#include <QMap>
#include <QObject>

class QMenu;

class RSSGUARD_DLLSPEC WebFactory : public QObject {
    Q_OBJECT

  public:
    explicit WebFactory(QObject* parent = nullptr);
    virtual ~WebFactory();

    // Strips "<....>" (HTML, XML) tags from given text.
    QString stripTags(QString text);

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

  private:
    static QMap<QString, char16_t> generateUnescapes();

    QString m_customUserAgent;
};

#endif // WEBFACTORY_H
