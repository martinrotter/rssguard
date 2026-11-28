// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/domdocument.h"

#include "definitions/definitions.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/xmlencodingdetector.h"

#include <QRegularExpression>

DomDocument::DomDocument() : QDomDocument() {}

bool DomDocument::setContent(const QByteArray& data,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
  QString xml_data_encoding = XmlEncodingDetector::detectXmlEncoding(data);
  QString decoded_xml_data = TextFactory::fromEncoding(data, xml_data_encoding);

  return setContent(decoded_xml_data, namespace_processing, error_msg, error_line, error_column);
}

bool DomDocument::setContent(const QString& text,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
  static const QRegularExpression bare_ampersand(QSL("(?<= )&(?= )"));
  static const QRegularExpression forbidden_chars(QSL("[\\x00-\\x08\\x0B-\\x0C\\x0E-\\x1F\\x7F-\\x84\\x86-\\x9F]+"));
  auto text_modified = text.trimmed();

  text_modified.replace(QSL("&shy;"), QString())
    .replace(QSL("&rsquo;"), QSL("\'"))
    .replace(bare_ampersand, QSL("&amp;"))
    .remove(forbidden_chars);

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
