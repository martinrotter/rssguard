// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPSERVICEROOT_H
#define XMPPSERVICEROOT_H

#include <librssguard/services/abstract/serviceroot.h>

class XmppNetwork;
class XmppFeed;

class XmppServiceRoot : public ServiceRoot {
    Q_OBJECT

  public:
    explicit XmppServiceRoot(RootItem* parent = nullptr);

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual void start(bool freshly_activated);
    virtual void stop();
    virtual QString code() const;
    virtual QList<QAction*> serviceMenu();
    virtual QString additionalTooltip() const;
    virtual bool supportsFeedAdding() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual void aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                          const QHash<QString, QHash<ServiceRoot::BagOfMessages, QStringList>>&
                                            stated_messages,
                                          const QHash<QString, QStringList>& tagged_messages);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);
    virtual bool wantsBaggedIdsOfExistingMessages() const;

    XmppNetwork* network() const;

  public slots:
    virtual void requestSyncIn();
    void pushArticleObtained(const QString& service, const QString& node, const Message& message);

  private:
    XmppFeed* findFeed(const QString& service, const QString& node) const;
    void updateTitle();

  private:
    XmppNetwork* m_network;
};

inline XmppNetwork* XmppServiceRoot::network() const {
  return m_network;
}

#endif // XMPPSERVICEROOT_H
