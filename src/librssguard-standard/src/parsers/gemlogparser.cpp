// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/gemlogparser.h"

#include "src/definitions.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/feedrecognizedbutfailedexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <qtlinq/qtlinq.h>

GemlogParser::GemlogParser(const QString& data) : FeedParser(data, DataType::Other) {
  m_entries = extractFeedEntries(data);
}

GemlogParser::~GemlogParser() {}

QList<StandardFeed*> GemlogParser::discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const {
  auto base_result = FeedParser::discoverFeeds(root, url, greedy);

  if (!base_result.isEmpty()) {
    return base_result;
  }

  QString my_url = url.toString();

  if (my_url.startsWith(QSL(URI_SCHEME_HTTP))) {
    my_url = my_url.mid(QSL(URI_SCHEME_HTTP).size());
  }

  if (my_url.startsWith(QSL(URI_SCHEME_HTTPS))) {
    my_url = my_url.mid(QSL(URI_SCHEME_HTTPS).size());
  }

  if (!my_url.startsWith(QSL("gemini://"))) {
    my_url = QSL("gemini://") + my_url;
  }

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

QPair<StandardFeed*, QList<IconLocation>> GemlogParser::guessFeed(const QByteArray& content,
                                                                  const NetworkResult& network_res) const {
  if (network_res.m_contentType.contains(QSL("text/gemini"))) {
    QString content_str = QString::fromUtf8(content);

    auto* feed = new StandardFeed();

    feed->setEncoding(QSL(DEFAULT_FEED_ENCODING));
    feed->setType(StandardFeed::Type::Gemlog);
    feed->setTitle(extractFeedTitle(content_str));
    feed->setSource(network_res.m_url.toString());

    return QPair<StandardFeed*, QList<IconLocation>>(feed, {});
  }
  else {
    throw ApplicationException(QObject::tr("not a Gemlog"));
  }
}

QString GemlogParser::extractFeedTitle(const QString& gemlog) const {
  static QRegularExpression exp(QSL("#\\s+(.+)$"), QRegularExpression::PatternOption::MultilineOption);

  auto match = exp.match(gemlog);
  return match.captured(1);
}

QVariantList GemlogParser::extractFeedEntries(const QString& gemlog) {
  QVariantList entries;

  if (gemlog.isEmpty()) {
    return entries;
  }

  QString gemini_hypertext = QString(gemlog).replace(QSL("\r\n"), QSL("\n")).replace(QSL("\r"), QSL("\n"));
  QStringList lines = gemini_hypertext.split(QL1C('\n'));

  static QRegularExpression exp_link(R"(^=>\s+([^\s]+)\s+(\d{4}-\d{2}-\d{2})\s+(.+)$)");

  QRegularExpressionMatch mtch;

  for (const QString& line : lines) {
    if ((mtch = exp_link.match(line)).hasMatch()) {
      GemlogEntry entry;

      entry.m_link = mtch.captured(1);
      entry.m_date = QDateTime::fromString(mtch.captured(2), QSL("yyyy-MM-dd"));
      entry.m_title = mtch.captured(3);
      entry.m_rawData = line;

      entries.append(QVariant::fromValue(entry));
    }
  }

  return entries;
}

QVariantList GemlogParser::objMessageElements() {
  return m_entries;
}

QString GemlogParser::objMessageTitle(const QVariant& msg_element) const {
  const auto& entry = msg_element.value<GemlogEntry>();

  return entry.m_title;
}

QString GemlogParser::objMessageUrl(const QVariant& msg_element) const {
  const auto& entry = msg_element.value<GemlogEntry>();

  return entry.m_link;
}

QString GemlogParser::objMessageDescription(const QVariant& msg_element) {
  Q_UNUSED(msg_element)
  return {};
}

QString GemlogParser::objMessageAuthor(const QVariant& msg_element) const {
  Q_UNUSED(msg_element)
  return {};
}

QDateTime GemlogParser::objMessageDateCreated(const QVariant& msg_element) {
  const auto& entry = msg_element.value<GemlogEntry>();

  return entry.m_date;
}

QString GemlogParser::objMessageId(const QVariant& msg_element) const {
  return objMessageUrl(msg_element);
}

QString GemlogParser::objMessageRawContents(const QVariant& msg_element) const {
  const auto& entry = msg_element.value<GemlogEntry>();

  return entry.m_rawData;
}
