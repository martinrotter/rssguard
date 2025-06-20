// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/domdocument.h"

DomDocument::DomDocument() : QDomDocument() {}

bool DomDocument::setContent(const QByteArray& text,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
#if QT_VERSION_MAJOR == 5
  auto text_modified = text;
#else
  auto text_modified = QByteArray(text).replace("&shy;", "");
#endif

#if QT_VERSION >= 0x060500 // Qt >= 6.5.0
  QDomDocument::ParseResult res =
    QDomDocument::setContent(text_modified,
                             namespace_processing ? QDomDocument::ParseOption::UseNamespaceProcessing
                                                  : QDomDocument::ParseOption::Default);

  if (error_msg != nullptr) {
    *error_msg = res.errorMessage;
  }

  if (error_line != nullptr) {
    *error_line = res.errorLine;
  }

  if (error_column != nullptr) {
    *error_column = res.errorColumn;
  }

  return (bool)res;
#else
  return QDomDocument::setContent(text_modified, namespace_processing, error_msg, error_line, error_column);
#endif
}

bool DomDocument::setContent(const QString& text,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
#if QT_VERSION_MAJOR == 5
  auto text_modified = text;
#else
  auto text_modified = QString(text).replace("&shy;", "");
#endif

#if QT_VERSION >= 0x060500 // Qt >= 6.5.0
  QDomDocument::ParseResult res =
    QDomDocument::setContent(text_modified,
                             namespace_processing ? QDomDocument::ParseOption::UseNamespaceProcessing
                                                  : QDomDocument::ParseOption::Default);

  if (error_msg != nullptr) {
    *error_msg = res.errorMessage;
  }

  if (error_line != nullptr) {
    *error_line = res.errorLine;
  }

  if (error_column != nullptr) {
    *error_column = res.errorColumn;
  }

  return (bool)res;
#else
  return QDomDocument::setContent(text_modified, namespace_processing, error_msg, error_line, error_column);
#endif
}
