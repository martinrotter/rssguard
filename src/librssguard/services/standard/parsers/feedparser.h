// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDPARSER_H
#define FEEDPARSER_H

#include <QDomDocument>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "core/message.h"
#include "definitions/typedefs.h"
#include "services/standard/standardfeed.h"

// Base class for all XML-based feed parsers.
class FeedParser {
  public:
    explicit FeedParser(QString data, bool is_xml = true);
    virtual ~FeedParser();

    // Returns list of absolute URLs of discovered feeds from provided base URL.
    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url) const;

    // Guesses feed.
    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const QString& content_type) const;

    // Returns list of all messages from the feed.
    virtual QList<Message> messages();

  protected:
    virtual QString feedAuthor() const;

    // XML.
    virtual QDomNodeList xmlMessageElements();
    virtual QString xmlMessageTitle(const QDomElement& msg_element) const;
    virtual QString xmlMessageUrl(const QDomElement& msg_element) const;
    virtual QString xmlMessageDescription(const QDomElement& msg_element) const;
    virtual QString xmlMessageAuthor(const QDomElement& msg_element) const;
    virtual QDateTime xmlMessageDateCreated(const QDomElement& msg_element) const;
    virtual QString xmlMessageId(const QDomElement& msg_element) const;
    virtual QList<Enclosure> xmlMessageEnclosures(const QDomElement& msg_element) const;
    virtual QList<MessageCategory> xmlMessageCategories(const QDomElement& msg_element) const;
    virtual QString xmlMessageRawContents(const QDomElement& msg_element) const;

    // JSON.
    virtual QJsonArray jsonMessageElements();
    virtual QString jsonMessageTitle(const QJsonObject& msg_element) const;
    virtual QString jsonMessageUrl(const QJsonObject& msg_element) const;
    virtual QString jsonMessageDescription(const QJsonObject& msg_element) const;
    virtual QString jsonMessageAuthor(const QJsonObject& msg_element) const;
    virtual QDateTime jsonMessageDateCreated(const QJsonObject& msg_element) const;
    virtual QString jsonMessageId(const QJsonObject& msg_element) const;
    virtual QList<Enclosure> jsonMessageEnclosures(const QJsonObject& msg_element) const;
    virtual QList<MessageCategory> jsonMessageCategories(const QJsonObject& msg_element) const;
    virtual QString jsonMessageRawContents(const QJsonObject& msg_element) const;

  protected:
    QList<Enclosure> xmlMrssGetEnclosures(const QDomElement& msg_element) const;
    QString xmlMrssTextFromPath(const QDomElement& msg_element, const QString& xml_path) const;
    QString xmlRawChild(const QDomElement& container) const;
    QStringList xmlTextsFromPath(const QDomElement& element,
                                 const QString& namespace_uri,
                                 const QString& xml_path,
                                 bool only_first) const;

  protected:
    bool m_isXml;
    QString m_data;
    QDomDocument m_xml;
    QJsonDocument m_json;
    QString m_mrssNamespace;
};

#endif // FEEDPARSER_H
