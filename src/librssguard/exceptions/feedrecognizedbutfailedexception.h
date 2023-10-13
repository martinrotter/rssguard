// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef UNRECOGNIZEDFEEDFORMATEXCEPTION_H
#define UNRECOGNIZEDFEEDFORMATEXCEPTION_H

#include "exceptions/applicationexception.h"

class FeedRecognizedButFailedException : public ApplicationException {
  public:
    explicit FeedRecognizedButFailedException(const QString& message = {});
};

#endif // UNRECOGNIZEDFEEDFORMATEXCEPTION_H
