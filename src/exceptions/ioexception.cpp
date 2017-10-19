// For license of this file, see <object-root-folder>/LICENSE.md.

#include "exceptions/ioexception.h"

IOException::IOException(const QString& message) : ApplicationException(message) {}

IOException::~IOException() {}
