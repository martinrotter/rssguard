// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/filterutils.h"

#include "definitions/definitions.h"

#include <QDomDocument>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>

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

  for (int i = 0; i < elem.childNodes().size(); i++) {
    if (!elem.childNodes().at(i).isElement()) {
      continue;
    }

    elems << QSL("\"%1\": %2").arg(elem.childNodes().at(i).toElement().tagName(),
                                   jsonProcessXmlElement(elem.childNodes().at(i).toElement()));
  }

  if (attrs.isEmpty()) {
    if (elems.isEmpty()) {
      return QSL("\"%1\"").arg(jsonEscapeString(elem.text()));
    }
    else {
      return QSL("{%1}").arg(elems.join(QSL(",\n")));
    }
  }
  else if (elems.isEmpty()) {
    return QSL("{%1, \"__text\": \"%2\"}").arg(attrs.join(QSL(",\n")),
                                               jsonEscapeString(elem.text()));
  }
  else {
    return QSL("{%1, %2}").arg(attrs.join(QSL(",\n")),
                               elems.join(QSL(",\n")));
  }
}

QString FilterUtils::fromXmlToJson(const QString& xml) const {
  QDomDocument xml_doc;

  xml_doc.setContent(xml);

  QString json = QSL("%1").arg(jsonProcessXmlElement(xml_doc.documentElement()));

  return QSL("{\"%1\": %2}").arg(xml_doc.documentElement().tagName(),
                                 json);
}
