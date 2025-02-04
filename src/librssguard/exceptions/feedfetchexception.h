// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDFETCHEXCEPTION_H
#define FEEDFETCHEXCEPTION_H

#include "exceptions/applicationexception.h"
#include "services/abstract/feed.h"

class RSSGUARD_DLLSPEC FeedFetchException : public ApplicationException {
  public:
    // "data" parameter should contain this, depending on "feed_status":
    //   - Feed::Status::NetworkError -> NetworkResult instance
    //   - other feed status values -> arbitrary data
    explicit FeedFetchException(Feed::Status feed_status, const QString& message = {}, const QVariant& data = {});

    Feed::Status feedStatus() const;
    QVariant data() const;

  private:
    QVariant m_data;
    Feed::Status m_feedStatus;
};

#endif // FEEDFETCHEXCEPTION_H
