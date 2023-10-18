// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef UNRECOGNIZEDFEEDFORMATEXCEPTION_H
#define UNRECOGNIZEDFEEDFORMATEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QVariant>

class FeedRecognizedButFailedException : public ApplicationException {
  public:
    explicit FeedRecognizedButFailedException(const QString& message = {}, const QVariant& arbitrary_data = {});

    QVariant arbitraryData() const;

  private:
    QVariant m_arbitraryData;
};

#endif // UNRECOGNIZEDFEEDFORMATEXCEPTION_H
