// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/icalparser.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "services/standard/definitions.h"

IcalParser::IcalParser(const QString& data) : FeedParser(data, DataType::Other) {}

IcalParser::~IcalParser() {}

QPair<StandardFeed*, QList<IconLocation>> IcalParser::guessFeed(const QByteArray& content,
                                                                const QString& content_type) const {
  if (content_type == QSL("text/calendar") || content.contains('\n')) {
    Icalendar calendar;

    try {
      calendar = Icalendar(content);
    }
    catch (const ApplicationException& ex) {
      throw FeedRecognizedButFailedException(QObject::tr("iCalendar error '%1'").arg(ex.message()));
    }

    auto* feed = new StandardFeed();
    QList<IconLocation> icon_possible_locations;

    feed->setEncoding(QSL(DEFAULT_FEED_ENCODING));
    feed->setType(StandardFeed::Type::Atom10);
    feed->setTitle(calendar.title());

    return QPair<StandardFeed*, QList<IconLocation>>(feed, icon_possible_locations);
  }
  else {
    throw ApplicationException(QObject::tr("not an iCalendar"));
  }
}

Icalendar::Icalendar() {}

Icalendar::Icalendar(const QByteArray& data) {
  processLines(data);
}

QString Icalendar::title() const {
  return m_title;
}

void Icalendar::setTitle(const QString& title) {
  m_title = title;
}

void Icalendar::processLines(const QByteArray& data) {
  QString str_data = QString::fromUtf8(data);

  QStringList str_blocks =
    str_data.remove(QRegularExpression("^END:\\w+$")).split(QRegularExpression(QSL("^BEGIN:\\w+$")));
}
