// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/domdocument.h"

#include "miscellaneous/iofactory.h"

#include <QRegularExpression>
#include <QXmlInputSource>

class XmlInputSource : public QXmlInputSource {
  public:
    virtual QString fromRawData(const QByteArray& data, bool beginning) {
      return QXmlInputSource::fromRawData(data, beginning);
    }
};

DomDocument::DomDocument() : QDomDocument() {}

bool DomDocument::setContent(const QByteArray& text,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
  QString decoded_xml_data = XmlInputSource().fromRawData(text, true);

  return setContent(decoded_xml_data, namespace_processing, error_msg, error_line, error_column);
}

bool DomDocument::setContent(const QString& text,
                             bool namespace_processing,
                             QString* error_msg,
                             int* error_line,
                             int* error_column) {
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

QString DomDocument::extractEncoding(const QByteArray& xml_data) {
  QString enc =
    QRegularExpression(QSL("encoding=\"([A-Z0-9\\-]+)\""), QRegularExpression::PatternOption::CaseInsensitiveOption)
      .match(xml_data)
      .captured(1);

  return enc;
}
