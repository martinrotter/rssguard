// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/feedfetchexception.h"

FeedFetchException::FeedFetchException(Feed::Status feed_status, const QString& message, const QVariant& data)
  : ApplicationException(message), m_data(data), m_feedStatus(feed_status) {}

Feed::Status FeedFetchException::feedStatus() const {
  return m_feedStatus;
}

QVariant FeedFetchException::data() const {
  return m_data;
}
