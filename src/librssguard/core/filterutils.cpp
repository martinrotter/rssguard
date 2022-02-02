// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/filterutils.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"

#include <QDomDocument>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>

FilterUtils::FilterUtils(QObject* parent) : QObject(parent) {}

FilterUtils::~FilterUtils() {
  qDebugNN << "Destroying FilterUtils instance.";
}

QString FilterUtils::hostname() const {
  return QHostInfo::localHostName();
}

QString jsonEscapeString(const QString& s) {
  return QString(QJsonDocument(QJsonArray() << s).toJson(QJsonDocument::JsonFormat::Compact)).mid(2).chopped(2);
}

QString jsonProcessXmlElement(const QDomElement& elem) {
  QStringList attrs;

  for (int i = 0; i < elem.attributes().size(); i++) {
    attrs << QSL("\"%1\": \"%2\"").arg(jsonEscapeString(elem.attributes().item(i).toAttr().name()),
                                       jsonEscapeString(elem.attributes().item(i).toAttr().value()));
  }

  QStringList elems;
  QString elem_text;

  for (int i = 0; i < elem.childNodes().size(); i++) {
    if (elem.childNodes().at(i).isText()) {
      elem_text = jsonEscapeString(elem.childNodes().at(i).nodeValue());
    }

    if (!elem.childNodes().at(i).isElement()) {
      continue;
    }

    elems << QSL("\"%1\": %2").arg(elem.childNodes().at(i).toElement().tagName(),
                                   jsonProcessXmlElement(elem.childNodes().at(i).toElement()));
  }

  QString str;

  if (!elems.isEmpty() && !attrs.isEmpty()) {
    str = QSL("{%1, %2, %3}").arg(attrs.join(QSL(",\n")),
                                  elems.join(QSL(",\n")),
                                  QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else if (!elems.isEmpty()) {
    str = QSL("{%1, %2}").arg(elems.join(QSL(",\n")),
                              QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else if (!attrs.isEmpty()) {
    str = QSL("{%1, %2}").arg(attrs.join(QSL(",\n")),
                              QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else {
    str = QSL("{%1}").arg(QSL("\"__text\": \"%1\"").arg(elem_text));
  }

  return str;
}

QString FilterUtils::fromXmlToJson(const QString& xml) const {
  QDomDocument xml_doc;

  xml_doc.setContent(xml);

  QString json = QSL("%1").arg(jsonProcessXmlElement(xml_doc.documentElement()));

  return QSL("{\"%1\": %2}").arg(xml_doc.documentElement().tagName(),
                                 json);
}

QDateTime FilterUtils::parseDateTime(const QString& dat) const {
  return TextFactory::parseDateTime(dat);
}

QString FilterUtils::runExecutableGetOutput(const QString& executable, const QStringList& arguments) const {
  try {
    return IOFactory::startProcessGetOutput(executable, arguments);
  }
  catch (const ApplicationException& ex) {
    return ex.message();
  }
}
