// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef UNRECOGNIZEDFEEDFORMATEXCEPTION_H
#define UNRECOGNIZEDFEEDFORMATEXCEPTION_H

#include "exceptions/applicationexception.h"

#include <QVariant>

class RSSGUARD_DLLSPEC FeedRecognizedButFailedException : public ApplicationException {
  public:
    explicit FeedRecognizedButFailedException(const QString& message = {}, const QVariant& arbitrary_data = {});

    void raise() const override { throw *this; }
    FeedRecognizedButFailedException* clone() const override { return new FeedRecognizedButFailedException(*this); }

    QVariant arbitraryData() const;

  private:
    QVariant m_arbitraryData;
};

#endif // UNRECOGNIZEDFEEDFORMATEXCEPTION_H
