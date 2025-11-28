// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/domdocument.h"

#include "definitions/definitions.h"
#include "miscellaneous/xmlencodingdetector.h"

#include <QRegularExpression>
#include <QTextCodec>

DomDocument::DomDocument() : QDomDocument() {}

bool DomDocument::setContent(const QByteArray& data,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
  QString xml_data_encoding = XmlEncodingDetector::detectXmlEncoding(data);
  QTextCodec* codec = QTextCodec::codecForName(xml_data_encoding.toLocal8Bit());
  QString decoded_xml_data;

  if (codec == nullptr) {
    decoded_xml_data = QString::fromUtf8(data);
  }
  else {
    decoded_xml_data = codec->toUnicode(data);
  }

  return setContent(decoded_xml_data, namespace_processing, error_msg, error_line, error_column);
}

bool DomDocument::setContent(const QString& text,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
  auto text_modified = QString(text)
                         .trimmed()
                         .replace(QSL("&shy;"), QString())
                         .replace(QSL("&rsquo;"), QSL("\'"))
                         .replace(QSL(" & "), QSL(" &amp; "))
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
