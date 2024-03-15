// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef IOEXCEPTION_H
#define IOEXCEPTION_H

#include "exceptions/applicationexception.h"

class RSSGUARD_DLLSPEC IOException : public ApplicationException {
  public:
    explicit IOException(const QString& message = QString());
};

#endif // IOEXCEPTION_H
