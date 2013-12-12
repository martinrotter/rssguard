#ifndef FEEDSMODELSTANDARDFEED_H
#define FEEDSMODELSTANDARDFEED_H

#include "core/feedsmodelfeed.h"


// Represents STANDARD RSS/RDF/ATOM feed with no
// online synchronization services (NO TT-RSS, NO FEEDLY).
class FeedsModelStandardFeed : public FeedsModelFeed {
  public:
    // Constructors and destructors.
    explicit FeedsModelStandardFeed(FeedsModelRootItem *parent_item);
    virtual ~FeedsModelStandardFeed();

    QVariant data(int column, int role) const;

};

#endif // FEEDSMODELSTANDARDFEED_H
