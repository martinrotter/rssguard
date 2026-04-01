// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPFEED_H
#define XMPPFEED_H

#include <librssguard/services/abstract/feed.h>

class XmppServiceRoot;
class QXmppMucManager;
class QXmppMucRoom;

class XmppFeed : public Feed {
    Q_OBJECT

    friend class FormXmppFeedDetails;

  public:
    enum class Type {
      PubSubNode = 1,
      Chatroom = 2
    };

    explicit XmppFeed(RootItem* parent = nullptr);

    virtual bool canBeDeleted() const;
    virtual void deleteItem();
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QString additionalTooltip() const;

    void obtainArticles();
    void storeRealTimeArticle(const Message& message);

    QList<Message> articles() const;
    void setArticles(const QList<Message>& articles);

    Type type() const;
    void setType(Type type);

    void join(QXmppMucManager* muc_manager);

    static QString typeToString(Type type);
    static QString extractXmppMessageTitle(const QString& text);

  private:
    QString serviceName() const;
    XmppServiceRoot* serviceRoot() const;
    void removeItself();

  private:
    QList<Message> m_articles;
    Type m_type;
    QXmppMucRoom* m_mucRoom;
};

#endif // XMPPFEED_H
