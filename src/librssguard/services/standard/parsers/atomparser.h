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
    virtual ~AtomParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const;

    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const QString& content_type) const;

  protected:
    virtual QString xmlMessageTitle(const QDomElement& msg_element) const;
    virtual QString xmlMessageDescription(const QDomElement& msg_element) const;
    virtual QDateTime xmlMessageDateCreated(const QDomElement& msg_element) const;
    virtual QString xmlMessageId(const QDomElement& msg_element) const;
    virtual QString xmlMessageUrl(const QDomElement& msg_element) const;
    virtual QList<Enclosure> xmlMessageEnclosures(const QDomElement& msg_element) const;
    virtual QList<MessageCategory> xmlMessageCategories(const QDomElement& msg_element) const;
    virtual QDomNodeList xmlMessageElements();
    virtual QString xmlMessageAuthor(const QDomElement& msg_element) const;
    virtual QString feedAuthor() const;

  private:
    QString atomNamespace() const;

    QString m_atomNamespace;
};

#endif // ATOMPARSER_H
