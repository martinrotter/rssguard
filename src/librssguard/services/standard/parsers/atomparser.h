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
    virtual QString xmlMessageTitle(const QDomElement& msg_element) const;
    virtual QString xmlMessageDescription(const QDomElement& msg_element) const;
    virtual QDateTime xmlMessageDateCreated(const QDomElement& msg_element) const;
    virtual QString xmlMessageId(const QDomElement& msg_element) const;
    virtual QString xmlMessageUrl(const QDomElement& msg_element) const;
    virtual QList<Enclosure> xmlMessageEnclosures(const QDomElement& msg_element) const;
    virtual QDomNodeList xmlMessageElements();
    virtual QString xmlMessageAuthor(const QDomElement& msg_element) const;
    virtual QString feedAuthor() const;

  private:
    QString m_atomNamespace;
};

#endif // ATOMPARSER_H
