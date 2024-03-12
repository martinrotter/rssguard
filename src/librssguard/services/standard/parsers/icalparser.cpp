// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/icalparser.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/definitions.h"

IcalParser::IcalParser(const QString& data)
  : FeedParser(data, DataType::Other), m_iCalendar(Icalendar(m_data.toUtf8())) {}

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

QVariantList IcalParser::objMessageElements() {
  QVariantList lst;

  for (const auto& comp : m_iCalendar.m_components) {
    lst.append(QVariant::fromValue(comp));
  }

  return lst;
}

QString IcalParser::objMessageTitle(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  return comp.title();
}

QString IcalParser::objMessageUrl(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);
  return comp.url();
}

QString IcalParser::objMessageDescription(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  return comp.description();
}

QString IcalParser::objMessageAuthor(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  return comp.organizer();
}

QDateTime IcalParser::objMessageDateCreated(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  return comp.startsOn();
}

QString IcalParser::objMessageId(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  return comp.uid();
}

QList<Enclosure> IcalParser::objMessageEnclosures(const QVariant& msg_element) const {
  return {};
}

QList<MessageCategory> IcalParser::objMessageCategories(const QVariant& msg_element) const {
  return {};
}

QString IcalParser::objMessageRawContents(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(comp.properties()))
                             .toJson(QJsonDocument::JsonFormat::Indented));
}

Icalendar::Icalendar(const QByteArray& data) : FeedParser(QString::fromUtf8(data), FeedParser::DataType::Other) {
  processLines(m_data);
}

QString Icalendar::title() const {
  return m_title;
}

void Icalendar::setTitle(const QString& title) {
  m_title = title;
}

void Icalendar::processLines(const QString& data) {
  QRegularExpression regex("^BEGIN:(\\w+)\\r$(.+?)(?=^BEGIN|^END)",
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

  setTitle(tokenized.value(QSL("X-WR-CALNAME")).toString());
}

void Icalendar::processComponentEvent(const QString& body) {
  auto tokenized = tokenizeBody(body);

  EventComponent event;

  event.setProperties(tokenized);

  m_components.append(event);
}

QVariantMap Icalendar::tokenizeBody(const QString& body) const {
  QRegularExpression regex("^(?=[A-Z-]+(?:;[A-Z]+=[A-Z]+)?:)", QRegularExpression::PatternOption::MultilineOption);
  auto all_matches = body.split(regex);
  QVariantMap res;

  for (const QString& match : all_matches) {
    int sep = match.indexOf(':');
    QString property = match.left(sep).simplified();

    if (property.isEmpty()) {
      continue;
    }

    QString value = match.mid(sep + 1);

    value = value.replace(QRegularExpression("\\r\\n\\s?"), QString());
    value = value.replace(QRegularExpression("\\\\n"), QSL("<br/>"));

    res.insert(property, value);
  }

  return res;
}

QString IcalendarComponent::uid() const {
  return m_properties.value(QSL("UID")).toString();
}

QVariantMap IcalendarComponent::properties() const {
  return m_properties;
}

void IcalendarComponent::setProperties(const QVariantMap& properties) {
  m_properties = properties;
}

QVariant IcalendarComponent::getPropertyValue(const QString& property_name) const {
  if (m_properties.contains(property_name)) {
    return m_properties.value(property_name);
  }

  QStringList keys = m_properties.keys();
  auto linq = boolinq::from(keys.begin(), keys.end());
  QString found_key = linq.firstOrDefault([&](const QString& ky) {
    int index_sep = ky.indexOf(';');

    return ky.startsWith(property_name) && index_sep == property_name.size();
  });

  return m_properties.value(found_key);
}

QDateTime EventComponent::startsOn() const {
  return TextFactory::parseDateTime(getPropertyValue(QSL("DTSTART")).toString());
}

QDateTime EventComponent::endsOn() const {
  return TextFactory::parseDateTime(m_properties.value(QSL("DTEND")).toString());
}

QString EventComponent::title() const {
  return m_properties.value(QSL("SUMMARY")).toString();
}

QString EventComponent::url() const {
  return m_properties.value(QSL("URL")).toString();
}

QString EventComponent::organizer() const {
  return m_properties.value(QSL("ORGANIZER")).toString();
}

QString EventComponent::location() const {
  return m_properties.value(QSL("LOCATION")).toString();
}

QString EventComponent::description() const {
  return m_properties.value(QSL("DESCRIPTION")).toString();
}

QDateTime EventComponent::created() const {
  return TextFactory::parseDateTime(m_properties.value(QSL("CREATED")).toString());
}

QDateTime EventComponent::lastModified() const {
  return TextFactory::parseDateTime(m_properties.value(QSL("LAST-MODIFIED")).toString());
}
