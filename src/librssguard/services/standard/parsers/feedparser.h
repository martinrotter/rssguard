// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDPARSER_H
#define FEEDPARSER_H

#include <QDomDocument>
#include <QString>

#include "core/message.h"

// Base class for all XML-based feed parsers.
class FeedParser {
  public:
    explicit FeedParser(QString data);

    virtual QList<Message> messages();

  protected:
    virtual QString feedAuthor() const;
    virtual QDomNodeList messageElements() = 0;
    virtual QString messageTitle(const QDomElement& msg_element) const = 0;
    virtual QString messageUrl(const QDomElement& msg_element) const = 0;
    virtual QString messageDescription(const QDomElement& msg_element) const = 0;
    virtual QString messageAuthor(const QDomElement& msg_element) const = 0;
    virtual QDateTime messageDateCreated(const QDomElement& msg_element) const = 0;
    virtual QString messageId(const QDomElement& msg_element) const = 0;
    virtual QList<Enclosure> messageEnclosures(const QDomElement& msg_element) const = 0;
    virtual QString messageRawContents(const QDomElement& msg_element) const;

  protected:
    QList<Enclosure> mrssGetEnclosures(const QDomElement& msg_element) const;
    QString mrssTextFromPath(const QDomElement& msg_element, const QString& xml_path) const;
    QString rawXmlChild(const QDomElement& container) const;
    QStringList textsFromPath(const QDomElement& element, const QString& namespace_uri, const QString& xml_path, bool only_first) const;

  protected:
    QString m_xmlData;
    QDomDocument m_xml;
    QString m_mrssNamespace;
};

#endif // FEEDPARSER_H
