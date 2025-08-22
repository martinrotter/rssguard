// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/domdocument.h"

#include "miscellaneous/iofactory.h"

#include <QRegularExpression>

DomDocument::DomDocument() : QDomDocument() {}

bool DomDocument::setContent(const QByteArray& text,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
  auto text_modified = QByteArray(text)
                         .trimmed()
                         .replace("&shy;", "")
                         .replace(" & ", " &amp; ")
                         .replace(0x000B, QByteArray())
                         .replace(0x0014, QByteArray());

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
  // NOTE: Removing QChar::SpecialCharacter::Null is only possible
  // in QString overload of this method because removing zero bytes
  // from bytearray could remove Unicode characters and mess the data.
  //
  // Note that this "QString" method overload is used for feed XML parsing.
  //
  // Keep this kind of in sync with "QByteArray" overload of this method.
  auto text_modified = QString(text)
                         .trimmed()
                         .replace("&shy;", QString())
                         .replace(" & ", " &amp; ")
                         .replace(QChar::SpecialCharacter::Null, QString());

  // Remove all forbidden characters per XML official format, see https://www.w3.org/TR/xml11/#charsets.
  text_modified.remove(QRegularExpression(QSL("[\\x01-\\x08]")));
  text_modified.remove(QRegularExpression(QSL("[\\x0B-\\x0C]")));
  text_modified.remove(QRegularExpression(QSL("[\\x0E-\\x1F]")));
  text_modified.remove(QRegularExpression(QSL("[\\x7F-\\x84]")));
  text_modified.remove(QRegularExpression(QSL("[\\x86-\\x9F]")));

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
