// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/icalparser.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/definitions.h"

IcalParser::IcalParser(const QString& data) : FeedParser(data, DataType::Other) {}

IcalParser::~IcalParser() {}

QList<StandardFeed*> IcalParser::discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const {
  auto base_result = FeedParser::discoverFeeds(root, url, greedy);

  if (!base_result.isEmpty()) {
    return base_result;
  }

  QString my_url = url.toString();

  // Test direct URL for a feed.
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray data;
  auto res = NetworkFactory::performNetworkOperation(my_url,
                                                     timeout,
                                                     {},
                                                     data,
                                                     QNetworkAccessManager::Operation::GetOperation,
                                                     {},
                                                     {},
                                                     {},
                                                     {},
                                                     root->networkProxy());

  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    try {
      // 1.
      auto guessed_feed = guessFeed(data, res.m_contentType);

      guessed_feed.first->setSource(my_url);

      return {guessed_feed.first};
    }
    catch (...) {
      qDebugNN << LOGSEC_CORE << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }
  }

  return {};
}

QPair<StandardFeed*, QList<IconLocation>> IcalParser::guessFeed(const QByteArray& content,
                                                                const QString& content_type) const {
  if (content_type.contains(QSL("text/calendar")) || content.contains("\r\n")) {
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
    feed->setType(StandardFeed::Type::iCalendar);
    feed->setTitle(calendar.title());

    return QPair<StandardFeed*, QList<IconLocation>>(feed, icon_possible_locations);
  }
  else {
    throw ApplicationException(QObject::tr("not an iCalendar"));
  }
}

Icalendar::Icalendar(const QByteArray& data) : FeedParser(QString::fromUtf8(data), FeedParser::DataType::Ical) {
  processLines(m_data);
}

QString Icalendar::title() const {
  return m_title;
}

void Icalendar::setTitle(const QString& title) {
  m_title = title;
}

void Icalendar::processLines(const QString& data) {
  QRegularExpression regex("^BEGIN:(\\w+)\\r$(.+?)^(BEGIN|END):\\w+",
                           QRegularExpression::PatternOption::MultilineOption |
                             QRegularExpression::PatternOption::DotMatchesEverythingOption);

  auto all_matches = regex.globalMatch(data);

  while (all_matches.hasNext()) {
    auto match = all_matches.next();
    QString component = match.captured(1);
    QString body = match.captured(2);

    if (component == QSL("VCALENDAR")) {
      processComponentCalendar(body);
    }

    if (component == QSL("VEVENT")) {
      processComponentEvent(body);
    }
  }
}

void Icalendar::processComponentCalendar(const QString& body) {
  auto tokenized = tokenizeBody(body);

  setTitle(tokenized.value(QSL("X-WR-CALNAME")));
}

void Icalendar::processComponentEvent(const QString& body) {
  auto tokenized = tokenizeBody(body);

  EventComponent event;

  event.setUid(tokenized.value(QSL("UID")));
  event.setTitle(tokenized.value(QSL("SUMMARY")));
  event.setDescription(tokenized.value(QSL("DESCRIPTION")));
  event.setCreated(TextFactory::parseDateTime(tokenized.value(QSL("CREATED"))));

  m_components.append(event);
}

QMap<QString, QString> Icalendar::tokenizeBody(const QString& body) const {
  QRegularExpression regex("^(?=[A-Z-]+:)", QRegularExpression::PatternOption::MultilineOption);
  auto all_matches = body.split(regex);
  QMap<QString, QString> res;

  for (const QString& match : all_matches) {
    int sep = match.indexOf(':');
    QString property = match.left(sep).simplified();

    if (property.isEmpty()) {
      continue;
    }

    QString value = match.mid(sep + 1);

    value = value.replace(QRegularExpression("\\r\\n\\s?"), QString());
    value = value.replace(QRegularExpression("\\r?\\n"), QSL("<br/>"));

    res.insert(property, value);
  }

  return res;
}

QString IcalendarComponent::uid() const {
  return m_uid;
}

void IcalendarComponent::setUid(const QString& uid) {
  m_uid = uid;
}

QString EventComponent::title() const {
  return m_title;
}

void EventComponent::setTitle(const QString& title) {
  m_title = title;
}

QString EventComponent::description() const {
  return m_description;
}

void EventComponent::setDescription(const QString& description) {
  m_description = description;
}

QDateTime EventComponent::created() const {
  return m_created;
}

void EventComponent::setCreated(const QDateTime& created) {
  m_created = created;
}
