// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/filterobjects.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "exceptions/applicationexception.h"
#include "filtering/filteringsystem.h"
#include "miscellaneous/domdocument.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/labelsnode.h"

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
    attrs << QSL("\"%1\": \"%2\"")
               .arg(jsonEscapeString(elem.attributes().item(i).toAttr().name()),
                    jsonEscapeString(elem.attributes().item(i).toAttr().value()));
  }

  QStringList elems;
  QString elem_text;

  for (int i = 0; i < elem.childNodes().size(); i++) {
    QDomNode el = elem.childNodes().at(i);

    if (el.isText()) {
      elem_text = jsonEscapeString(el.nodeValue());
    }

    if (!el.isElement()) {
      continue;
    }

    elems << QSL("\"%1\": %2").arg(el.toElement().tagName(), jsonProcessXmlElement(el.toElement()));
  }

  QString str;

  if (!elems.isEmpty() && !attrs.isEmpty()) {
    str =
      QSL("{%1, %2, %3}").arg(attrs.join(QSL(",\n")), elems.join(QSL(",\n")), QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else if (!elems.isEmpty()) {
    str = QSL("{%1, %2}").arg(elems.join(QSL(",\n")), QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else if (!attrs.isEmpty()) {
    str = QSL("{%1, %2}").arg(attrs.join(QSL(",\n")), QSL("\"__text\": \"%1\"").arg(elem_text));
  }
  else {
    str = QSL("{%1}").arg(QSL("\"__text\": \"%1\"").arg(elem_text));
  }

  return str;
}

QString FilterUtils::fromXmlToJson(const QString& xml) const {
  DomDocument xml_doc;

  xml_doc.setContent(xml, true);

  QString json = QSL("%1").arg(jsonProcessXmlElement(xml_doc.documentElement()));

  return QSL("{\"%1\": %2}").arg(xml_doc.documentElement().tagName(), json);
}

QDateTime FilterUtils::parseDateTime(const QString& dat) const {
  return TextFactory::parseDateTime(dat);
}

QString FilterUtils::runExecutableGetOutput(const QString& executable,
                                            const QStringList& arguments,
                                            const QString& working_directory) const {
  try {
    return IOFactory::startProcessGetOutput(executable, arguments, working_directory);
  }
  catch (const ApplicationException& ex) {
    return ex.message();
  }
}

void FilterUtils::runExecutable(const QString& executable,
                                const QStringList& arguments,
                                const QString& working_directory) const {
  try {
    IOFactory::startProcessDetached(executable, arguments, working_directory);
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_JS << "Error when running executable:" << QUOTE_W_SPACE_DOT(ex.message());
  }
}

void FilterUtils::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

QString FilterApp::findLabel(const QString& label_title) const {
  Label* found_lbl = boolinq::from(m_system->availableLabels()).firstOrDefault([label_title](Label* lbl) {
    return lbl->title().toLower() == label_title.toLower();
  });

  if (found_lbl == nullptr) {
    qWarningNN << LOGSEC_CORE << "Label with title" << QUOTE_W_SPACE(label_title) << "not found.";
  }

  return found_lbl != nullptr ? found_lbl->customId() : QString();
}

QString FilterApp::createLabel(const QString& label_title, const QString& hex_color) {
  QString lbl_id = findLabel(label_title);

  if (!lbl_id.isEmpty()) {
    // Label exists.
    return lbl_id;
  }

  if (!Globals::hasFlag(m_system->account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Adding)) {
    qWarningNN << LOGSEC_CORE << "This account does not support creating labels.";
    return nullptr;
  }

  Label* new_lbl = nullptr;

  try {
    auto rnd_color = TextFactory::generateRandomColor();

    new_lbl = new Label(label_title, hex_color.isEmpty() ? rnd_color : hex_color);
    QSqlDatabase db = m_system->database();

    DatabaseQueries::createLabel(db, new_lbl, m_system->account()->accountId());
    m_system->account()->requestItemReassignment(new_lbl, m_system->account()->labelsNode());

    m_system->availableLabels().append(new_lbl);

    return new_lbl->customId();
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_CORE << "Cannot create label:" << QUOTE_W_SPACE_DOT(ex.message());

    if (new_lbl != nullptr) {
      new_lbl->deleteLater();
    }
  }

  return {};
}

QList<Label*> FilterApp::availableLabels() const {
  return m_system->availableLabels();
}

void FilterApp::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

QString FilterFeed::title() const {
  return m_system->feed()->title();
}

void FilterFeed::setSystem(FilteringSystem* sys) {
  m_system = sys;
}
