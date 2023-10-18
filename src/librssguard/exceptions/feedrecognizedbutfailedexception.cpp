// For license of this file, see <project-root-folder>/LICENSE.md.

#include "exceptions/feedrecognizedbutfailedexception.h"

FeedRecognizedButFailedException::FeedRecognizedButFailedException(const QString& message,
                                                                   const QVariant& arbitrary_data)
  : ApplicationException(message), m_arbitraryData(arbitrary_data) {}

QVariant FeedRecognizedButFailedException::arbitraryData() const {
  return m_arbitraryData;
}
