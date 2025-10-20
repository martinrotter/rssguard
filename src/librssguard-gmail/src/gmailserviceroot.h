// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GMAILSERVICEROOT_H
#define GMAILSERVICEROOT_H

#include "src/gui/emailpreviewer.h"

#include <librssguard/services/abstract/cacheforserviceroot.h>
#include <librssguard/services/abstract/serviceroot.h>

class GmailNetworkFactory;

class GmailServiceRoot : public ServiceRoot, public CacheForServiceRoot {
    Q_OBJECT

  public:
    explicit GmailServiceRoot(RootItem* parent = nullptr);
    virtual ~GmailServiceRoot();

    GmailNetworkFactory* network() const;

    virtual QList<QAction*> contextMenuMessagesList(const QList<Message>& messages);
    virtual QList<QAction*> serviceMenu();
    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual QString additionalTooltip() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual bool displaysEnclosures() const;
    virtual LabelOperation supportedLabelOperations() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);
    virtual bool wantsBaggedIdsOfExistingMessages() const;
    virtual CustomMessagePreviewer* customMessagePreviewer();

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private slots:
    void replyToEmail();
    void writeNewEmail();

  private:
    void updateTitle();

  private:
    QPointer<EmailPreviewer> m_emailPreview;
    GmailNetworkFactory* m_network;
    QAction* m_actionReply;
    Message m_replyToMessage;
};



#endif // GMAILSERVICEROOT_H
