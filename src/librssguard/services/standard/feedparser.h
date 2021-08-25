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
    QList<Enclosure> mrssGetEnclosures(const QDomElement& msg_element) const;
    QString mrssTextFromPath(const QDomElement& msg_element, const QString& xml_path) const;
    QString rawXmlChild(const QDomElement& container) const;
    QStringList textsFromPath(const QDomElement& element, const QString& namespace_uri, const QString& xml_path, bool only_first) const;
    virtual QDomNodeList messageElements() = 0;
    virtual QString feedAuthor() const;
    virtual Message extractMessage(const QDomElement& msg_element, QDateTime current_time) const = 0;

  protected:
    QString m_xmlData;
    QDomDocument m_xml;
    QString m_mrssNamespace;
};

#endif // FEEDPARSER_H
