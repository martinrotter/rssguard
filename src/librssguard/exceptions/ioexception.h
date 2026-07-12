// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef IOEXCEPTION_H
#define IOEXCEPTION_H

#include "exceptions/applicationexception.h"

class RSSGUARD_DLLSPEC IOException : public ApplicationException {
  public:
    explicit IOException(const QString& message = QString());

    void raise() const override { throw *this; }
    IOException* clone() const override { return new IOException(*this); }
};

#endif // IOEXCEPTION_H
