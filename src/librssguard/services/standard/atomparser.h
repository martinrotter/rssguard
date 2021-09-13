// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ATOMPARSER_H
#define ATOMPARSER_H

#include "services/standard/feedparser.h"

#include "core/message.h"

#include <QDomDocument>
#include <QList>

class AtomParser : public FeedParser {
  public:
    explicit AtomParser(const QString& data);

    QString atomNamespace() const;

  private:
    QDomNodeList messageElements();
    QString feedAuthor() const;
    Message extractMessage(const QDomElement& msg_element, const QDateTime& current_time) const;
    QString messageAuthor(const QDomElement& msg_element) const;

  private:
    QString m_atomNamespace;
};

#endif // ATOMPARSER_H
