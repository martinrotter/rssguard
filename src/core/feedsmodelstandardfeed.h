#ifndef FEEDSMODELSTANDARDFEED_H
#define FEEDSMODELSTANDARDFEED_H

#include <QDateTime>

#include "core/feedsmodelfeed.h"


// Represents STANDARD RSS/RDF/ATOM feed with no
// online synchronization services (NO TT-RSS, NO FEEDLY).
// So, parent item is either root item or category.
class FeedsModelStandardFeed : public FeedsModelFeed {
  public:
    // Constructors and destructors.
    explicit FeedsModelStandardFeed(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelStandardFeed();

    QVariant data(int column, int role) const;


  private:
    QString m_title;
    QDateTime m_creationDate;
    QString m_encoding;
    QString m_url;
    QString m_description;
    QString m_language;

    int m_totalCount;
    int m_unreadCount;
};

#endif // FEEDSMODELSTANDARDFEED_H
