// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDPARSER_H
#define FEEDPARSER_H

#include <QDomDocument>
#include <QString>

#include "core/message.h"

class FeedParser {
  public:
    explicit FeedParser(QString  data);
    virtual ~FeedParser();

    virtual QList<Message> messages();

  protected:
    QStringList textsFromPath(const QDomElement& element, const QString& namespace_uri, const QString& xml_path, bool only_first) const;
    virtual QDomNodeList messageElements() = 0;
    virtual QString feedAuthor() const;
    virtual Message extractMessage(const QDomElement& msg_element, QDateTime current_time) const = 0;

  protected:
    QString m_xmlData;
    QDomDocument m_xml;
};

#endif // FEEDPARSER_H
