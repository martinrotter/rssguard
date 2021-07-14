// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERSERVICEROOT_H
#define INOREADERSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class InoreaderNetworkFactory;

class InoreaderServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    explicit InoreaderServiceRoot(RootItem* parent = nullptr);
    virtual ~InoreaderServiceRoot();

    InoreaderNetworkFactory* network() const;

    virtual LabelOperation supportedLabelOperations() const;
    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual QString additionalTooltip() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(const QList<Feed*>& feeds,
                                             const QHash<QString, QPair<ServiceRoot::BagOfMessages, QStringList>>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitle();

  private:
    InoreaderNetworkFactory* m_network;
};

inline InoreaderNetworkFactory* InoreaderServiceRoot::network() const {
  return m_network;
}

#endif // INOREADERSERVICEROOT_H
