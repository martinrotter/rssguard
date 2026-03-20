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

  private:
    XmppServiceRoot* serviceRoot() const;
    void removeItself();
};

#endif // XMPPFEED_H
