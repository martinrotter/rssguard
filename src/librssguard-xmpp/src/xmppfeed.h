// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPFEED_H
#define XMPPFEED_H

#include "src/xmppcategory.h"

#include <librssguard/services/abstract/feed.h>

#include <QXmppMessage.h>
#include <QXmppMucManager.h>

class XmppServiceRoot;
class QXmppMucManager;

class XmppFeed : public Feed {
    Q_OBJECT

    friend class FormXmppFeedDetails;

  public:
    explicit XmppFeed(RootItem* parent = nullptr);

    virtual bool canBeDeleted() const;
    virtual void deleteItem();
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QString additionalTooltip() const;

    void storeRealTimeArticle(const Message& message);

    QList<Message> articles() const;
    void setArticles(const QList<Message>& articles);

    XmppCategory::Type type() const;

    void join(QXmppMucManager* muc_manager);
    void unjoin();

    QString service() const;
    void setService(const QString& service);

    static QString extractXmppMessageTitle(const QString& text);
    static Message articleFromXmppMessage(XmppCategory::Type source_type, const QXmppMessage& msg);

  public slots:
    void obtainArticles(bool notify_on_finish = false);

  private slots:
    void onJoinedChanged();
    void onError(const QXmppStanza::Error& error);
    void onMucMessageReceived(const QXmppMessage& msg);

  private:
    void obtainPubSubArticles(bool notify_on_finish);
    void obtainArchivedArticles(bool notify_on_finish);

    XmppServiceRoot* serviceRoot() const;
    void removeItself();

  private:
    QList<Message> m_articles;
    QPointer<QXmppMucRoom> m_mucRoom;
    QString m_service;
};

#endif // XMPPFEED_H
