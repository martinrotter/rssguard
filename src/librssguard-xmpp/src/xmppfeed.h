// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPFEED_H
#define XMPPFEED_H

#include <librssguard/services/abstract/feed.h>

#include <QXmppMessage.h>
#include <QXmppMucManager.h>

class XmppServiceRoot;
class QXmppMucManager;

class XmppFeed : public Feed {
    Q_OBJECT

    friend class FormXmppFeedDetails;

  public:
    enum class Type {
      PubSubServiceNode = 1,
      PubSubPep = 2,
      MultiUserChatRoom = 3,
      SingleUserChat = 4
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
    void unjoin();

    static QString typeToString(Type type);
    static QString extractXmppMessageTitle(const QString& text);
    static Message articleFromXmppMessage(const QXmppMessage& msg);

  private slots:
    void onJoinedChanged();
    void onError(const QXmppStanza::Error& error);
    void onMessageReceived(const QXmppMessage& msg);

  private:
    QString serviceName() const;
    XmppServiceRoot* serviceRoot() const;
    void removeItself();

  private:
    QList<Message> m_articles;
    Type m_type;
    QPointer<QXmppMucRoom> m_mucRoom;
};

#endif // XMPPFEED_H
