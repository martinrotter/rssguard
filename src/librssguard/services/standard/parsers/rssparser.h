// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RSSPARSER_H
#define RSSPARSER_H

#include "services/standard/parsers/feedparser.h"

#include "core/message.h"

#include <QList>

class RssParser : public FeedParser {
  public:
    explicit RssParser(const QString& data);

  protected:
    virtual QDomNodeList messageElements();
    virtual QString messageTitle(const QDomElement& msg_element) const;
    virtual QString messageDescription(const QDomElement& msg_element) const;
    virtual QString messageAuthor(const QDomElement& msg_element) const;
    virtual QDateTime messageDateCreated(const QDomElement& msg_element) const;
    virtual QString messageId(const QDomElement& msg_element) const;
    virtual QString messageUrl(const QDomElement& msg_element) const;
    virtual QList<Enclosure> messageEnclosures(const QDomElement& msg_element) const;
};

#endif // RSSPARSER_H
