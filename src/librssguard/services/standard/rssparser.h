// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RSSPARSER_H
#define RSSPARSER_H

#include "services/standard/feedparser.h"

#include "core/message.h"

#include <QList>

class RssParser : public FeedParser {
  public:
    explicit RssParser(const QString& data);

  private:
    QDomNodeList messageElements();
    Message extractMessage(const QDomElement& msg_element, QDateTime current_time) const;
};

#endif // RSSPARSER_H
