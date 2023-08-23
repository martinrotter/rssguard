// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEWSBLURSERVICEROOT_H
#define NEWSBLURSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class NewsBlurNetwork;

class NewsBlurServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    explicit NewsBlurServiceRoot(RootItem* parent = nullptr);

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual LabelOperation supportedLabelOperations() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);

    NewsBlurNetwork* network() const;

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitleIcon();

  private:
    NewsBlurNetwork* m_network;
};

inline NewsBlurNetwork* NewsBlurServiceRoot::network() const {
  return m_network;
}

#endif // NEWSBLURSERVICEROOT_H
