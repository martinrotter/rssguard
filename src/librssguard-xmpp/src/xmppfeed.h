// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPFEED_H
#define XMPPFEED_H

#include <librssguard/services/abstract/feed.h>

class XmppServiceRoot;

class XmppFeed : public Feed {
    Q_OBJECT

    friend class FormXmppFeedDetails;

  public:
    explicit XmppFeed(RootItem* parent = nullptr);

    virtual bool canBeDeleted() const;
    virtual void deleteItem();

    void obtainArticles();

    void storeRealTimeArticle(const Message& message);

    QList<Message> articles() const;
    void setArticles(const QList<Message>& articles);

  private:
    QString serviceName() const;
    XmppServiceRoot* serviceRoot() const;
    void removeItself();

  private:
    QList<Message> m_articles;
};

#endif // XMPPFEED_H
