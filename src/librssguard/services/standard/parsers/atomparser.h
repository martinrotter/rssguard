// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ATOMPARSER_H
#define ATOMPARSER_H

#include "services/standard/parsers/feedparser.h"

#include "core/message.h"

#include <QDomDocument>
#include <QList>

class AtomParser : public FeedParser {
  public:
    explicit AtomParser(const QString& data);

    QString atomNamespace() const;

  protected:
    virtual QString messageTitle(const QDomElement& msg_element) const;
    virtual QString messageDescription(const QDomElement& msg_element) const;
    virtual QDateTime messageDateCreated(const QDomElement& msg_element) const;
    virtual QString messageId(const QDomElement& msg_element) const;
    virtual QString messageUrl(const QDomElement& msg_element) const;
    virtual QList<Enclosure> messageEnclosures(const QDomElement& msg_element) const;
    virtual QDomNodeList messageElements();
    virtual QString messageAuthor(const QDomElement& msg_element) const;
    virtual QString feedAuthor() const;

  private:
    QString m_atomNamespace;
};

#endif // ATOMPARSER_H
