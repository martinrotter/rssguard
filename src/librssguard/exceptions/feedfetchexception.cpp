// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/feedfetchexception.h"

FeedFetchException::FeedFetchException(Feed::Status feed_status, const QString& message)
  : ApplicationException(message), m_feedStatus(feed_status) {}

Feed::Status FeedFetchException::feedStatus() const {
  return m_feedStatus;
}
