// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDPARSER_H
#define FEEDPARSER_H

#include "src/standardfeed.h"

#include <librssguard/core/message.h>
#include <librssguard/definitions/typedefs.h>
#include <librssguard/miscellaneous/domdocument.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

struct FeedComment {
    QString m_title;
    QString m_contents;
};

// Base class for all XML-based feed parsers.
class FeedParser {
  public:
    enum class DataType {
      Xml,
      Json,
      Other
    };

    FeedParser();
    explicit FeedParser(QString data, DataType is_xml = DataType::Xml);
    virtual ~FeedParser();

    // Returns list of absolute URLs of discovered feeds from provided base URL.
    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const;

    // Guesses feed.
    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const NetworkResult& network_res =
                                                                  NetworkResult()) const;

    // Returns list of all messages from the feed.
    virtual QList<Message> messages();

    QString dateTimeFormat() const;
    void setDateTimeFormat(const QString& dt_format);

    bool dontUseRawXmlSaving() const;
    void setDontUseRawXmlSaving(bool no_raw_xml_saving);

    std::function<QByteArray(QUrl)> resourceHandler() const;
    void setResourceHandler(const std::function<QByteArray(QUrl)>& res_handler);

    bool fetchComments() const;
    void setFetchComments(bool cmnts);

  protected:
    virtual QString feedAuthor() const;

    // XML.
    virtual QDomNodeList xmlMessageElements();
    virtual QString xmlMessageTitle(const QDomElement& msg_element) const;
    virtual QString xmlMessageUrl(const QDomElement& msg_element) const;
    virtual QString xmlMessageDescription(const QDomElement& msg_element) const;
    virtual QString xmlMessageAuthor(const QDomElement& msg_element) const;
    virtual QDateTime xmlMessageDateCreated(const QDomElement& msg_element);
    virtual QString xmlMessageId(const QDomElement& msg_element) const;
    virtual QList<Enclosure> xmlMessageEnclosures(const QDomElement& msg_element) const;
    virtual QList<MessageCategory*> xmlMessageCategories(const QDomElement& msg_element) const;
    virtual QString xmlMessageRawContents(const QDomElement& msg_element) const;

    // JSON.
    virtual QJsonArray jsonMessageElements();
    virtual QString jsonMessageTitle(const QJsonObject& msg_element) const;
    virtual QString jsonMessageUrl(const QJsonObject& msg_element) const;
    virtual QString jsonMessageDescription(const QJsonObject& msg_element) const;
    virtual QString jsonMessageAuthor(const QJsonObject& msg_element) const;
    virtual QDateTime jsonMessageDateCreated(const QJsonObject& msg_element);
    virtual QString jsonMessageId(const QJsonObject& msg_element) const;
    virtual QList<Enclosure> jsonMessageEnclosures(const QJsonObject& msg_element) const;
    virtual QList<MessageCategory*> jsonMessageCategories(const QJsonObject& msg_element) const;
    virtual QString jsonMessageRawContents(const QJsonObject& msg_element) const;

    // Objects.
    virtual QVariantList objMessageElements();
    virtual QString objMessageTitle(const QVariant& msg_element) const;
    virtual QString objMessageUrl(const QVariant& msg_element) const;
    virtual QString objMessageDescription(const QVariant& msg_element);
    virtual QString objMessageAuthor(const QVariant& msg_element) const;
    virtual QDateTime objMessageDateCreated(const QVariant& msg_element);
    virtual QString objMessageId(const QVariant& msg_element) const;
    virtual QList<Enclosure> objMessageEnclosures(const QVariant& msg_element) const;
    virtual QList<MessageCategory*> objMessageCategories(const QVariant& msg_element) const;
    virtual QString objMessageRawContents(const QVariant& msg_element) const;

  protected:
    void logUnsuccessfulRequest(const NetworkResult& reply) const;
    QList<Enclosure> xmlMrssGetEnclosures(const QDomElement& msg_element) const;
    QString xmlMrssTextFromPath(const QDomElement& msg_element, const QString& xml_path) const;
    QString xmlRawChild(const QDomElement& container) const;
    QStringList xmlTextsFromPath(const QDomElement& element,
                                 const QString& namespace_uri,
                                 const QString& xml_path,
                                 bool only_first) const;

    QString formatComments(const QList<FeedComment>& comments) const;

  protected:
    std::function<QByteArray(QUrl)> m_resourceHandler;
    DataType m_dataType;
    QString m_data;
    QString m_dateTimeFormat;
    DomDocument m_xml;
    QJsonDocument m_json;
    QString m_mrssNamespace;
    bool m_dontUseRawXmlSaving;
    bool m_fetchComments;
};

#endif // FEEDPARSER_H
