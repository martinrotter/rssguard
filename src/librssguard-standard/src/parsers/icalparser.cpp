// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/icalparser.h"

#include "src/definitions.h"

#include <librssguard/3rd-party/boolinq/boolinq.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/feedrecognizedbutfailedexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>

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
      auto guessed_feed = guessFeed(data, res);

      return {guessed_feed.first};
    }
    catch (...) {
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }
  }
  else {
    logUnsuccessfulRequest(res);
  }

  return {};
}

QPair<StandardFeed*, QList<IconLocation>> IcalParser::guessFeed(const QByteArray& content,
                                                                const NetworkResult& network_res) const {
  if (network_res.m_contentType.contains(QSL("text/calendar")) || content.startsWith(QSL("BEGIN").toLocal8Bit())) {
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
    feed->setSource(network_res.m_url.toString());

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

QString IcalParser::objMessageDescription(const QVariant& msg_element) {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  bool has_dt;
  auto son = comp.startsOn(m_iCalendar.m_tzs, &has_dt, &m_dateTimeFormat).toLocalTime();

  QString formaton = has_dt ? QLocale().dateTimeFormat(QLocale::FormatType::LongFormat)
                            : QLocale().dateFormat(QLocale::FormatType::LongFormat);

  auto soff = comp.endsOn(m_iCalendar.m_tzs, &has_dt, &m_dateTimeFormat).toLocalTime();
  QString formatoff = has_dt ? QLocale().dateTimeFormat(QLocale::FormatType::LongFormat)
                             : QLocale().dateFormat(QLocale::FormatType::LongFormat);

  QString body = QSL("Start date/time: %2<br/>"
                     "End date/time: %3<br/>"
                     "Location: %4<br/>"
                     "UID: %5<br/>"
                     "<br/>"
                     "%1")
                   .arg(comp.description(),
                        QLocale().toString(son, formaton),
                        QLocale().toString(soff, formatoff),
                        comp.location(),
                        comp.uid());

  return body;
}

QString IcalParser::objMessageAuthor(const QVariant& msg_element) const {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  return comp.organizer();
}

QDateTime IcalParser::objMessageDateCreated(const QVariant& msg_element) {
  const IcalendarComponent& comp_base = msg_element.value<IcalendarComponent>();
  const EventComponent& comp = static_cast<const EventComponent&>(comp_base);

  QDateTime dat = comp.startsOn(m_iCalendar.m_tzs, nullptr, &m_dateTimeFormat);

  return dat;
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
  if (!data.isEmpty()) {
    processLines(m_data);
  }
}

QString Icalendar::title() const {
  return m_title;
}

void Icalendar::setTitle(const QString& title) {
  m_title = title;
}

void Icalendar::processLines(const QString& data) {
  static QRegularExpression regex("^BEGIN:(\\w+)\\r$(.+?)(?=^BEGIN|^END)",
                                  QRegularExpression::PatternOption::MultilineOption |
                                    QRegularExpression::PatternOption::DotMatchesEverythingOption);

  auto all_matches = regex.globalMatch(data);

  if (!all_matches.hasNext()) {
    throw ApplicationException(QObject::tr("required iCal data are missing"));
  }

  while (all_matches.hasNext()) {
    auto match = all_matches.next();
    QString component = match.captured(1);
    QString body = match.captured(2);

    // Root calendar component.
    if (component == QSL("VCALENDAR")) {
      processComponentCalendar(body);
    }

    if (component == QSL("VTIMEZONE")) {
      processComponentTimezone(body);
    }

    // Event component.
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

void Icalendar::processComponentTimezone(const QString& body) {
  auto tokenized = tokenizeBody(body);

  QString tz_id = tokenized.value(QSL("TZID")).toString();

  if (!tz_id.isEmpty()) {
    m_tzs.insert(tz_id, QTimeZone(tz_id.toLocal8Bit()));
  }
}

QVariantMap Icalendar::tokenizeBody(const QString& body) const {
  static QRegularExpression regex("^(?=[A-Z-]+(?:;[A-Z-]+=[A-Z-\\/]+)?:)",
                                  QRegularExpression::PatternOption::MultilineOption |
                                    QRegularExpression::PatternOption::CaseInsensitiveOption);
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
    value = value.replace(QSL("\\n"), QSL("<br/>"));
    value = value.replace(QSL("\\,"), QSL(","));
    value = value.replace(QSL("\\;"), QSL(";"));

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
  QString modifier;

  return getPropertyValue(property_name, modifier);
}

QVariant IcalendarComponent::getPropertyValue(const QString& property_name, QString& property_modifier) const {
  if (m_properties.contains(property_name)) {
    return m_properties.value(property_name);
  }

  QStringList keys = m_properties.keys();
  auto linq = boolinq::from(keys.begin(), keys.end());
  QString found_key = linq.firstOrDefault([&](const QString& ky) {
    int index_sep = ky.indexOf(';');
    bool res = ky.startsWith(property_name) && index_sep == property_name.size();

    if (res) {
      property_modifier = ky.mid(index_sep + 1);
    }

    return res;
  });

  return m_properties.value(found_key);
}

QDateTime IcalendarComponent::fixupDate(QDateTime dat,
                                        const QString& dt_format,
                                        const QMap<QString, QTimeZone>& time_zones,
                                        const QString& modifiers,
                                        bool* has_dt) const {
  // dat.setTimeSpec(Qt::TimeSpec::LocalTime);

  // auto xx = dat.toUTC().toString();

  bool time_initialized = dt_format.contains('T');
  QStringList spl = modifiers.split('=');

  if (has_dt != nullptr) {
    *has_dt = time_initialized;
  }

  if (time_initialized && spl.size() == 2 && time_zones.contains(spl.at(1))) {
    QTimeZone tz = time_zones.value(spl.at(1));

#if QT_VERSION < 0x060900 // Qt < 6.9.0
    dat.setTimeSpec(Qt::TimeSpec::TimeZone);
#endif

    dat.setTimeZone(tz);

    return dat.toUTC();
  }
  else {
    return dat;
  }
}

QDateTime EventComponent::startsOn(const QMap<QString, QTimeZone>& time_zones, bool* had_dt, QString* dt_format) const {
  QString modifiers;
  bool has_dt;
  QDateTime dat = TextFactory::parseDateTime(getPropertyValue(QSL("DTSTART"), modifiers).toString(), dt_format);

  dat = fixupDate(dat, *dt_format, time_zones, modifiers, &has_dt);

  if (had_dt != nullptr) {
    *had_dt = has_dt;
  }

  return dat;
}

QDateTime EventComponent::endsOn(const QMap<QString, QTimeZone>& time_zones, bool* had_dt, QString* dt_format) const {
  QString modifiers;
  bool has_dt;
  QDateTime dat = TextFactory::parseDateTime(getPropertyValue(QSL("DTEND"), modifiers).toString(), dt_format);

  dat = fixupDate(dat, *dt_format, time_zones, modifiers, &has_dt);

  if (had_dt != nullptr) {
    *had_dt = has_dt;
  }

  return dat;
}

QString EventComponent::title() const {
  return getPropertyValue(QSL("SUMMARY")).toString();
}

QString EventComponent::url() const {
  return getPropertyValue(QSL("URL")).toString();
}

QString EventComponent::organizer() const {
  return getPropertyValue(QSL("ORGANIZER")).toString();
}

QString EventComponent::location() const {
  return getPropertyValue(QSL("LOCATION")).toString();
}

QString EventComponent::description() const {
  return getPropertyValue(QSL("DESCRIPTION")).toString();
}
