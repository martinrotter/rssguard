// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/atomparser.h"

#include "src/definitions.h"
#include "src/standardfeed.h"

#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>

#include <QTextCodec>

AtomParser::AtomParser(const QString& data) : FeedParser(data) {
  QString version = m_xml.documentElement().attribute(QSL("version"));

  if (version == QSL("0.3")) {
    m_atomNamespace = QSL("http://purl.org/atom/ns#");
  }
  else {
    m_atomNamespace = QSL("http://www.w3.org/2005/Atom");
  }
}

AtomParser::~AtomParser() {}

QList<StandardFeed*> AtomParser::discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const {
  auto base_result = FeedParser::discoverFeeds(root, url, greedy);

  if (!base_result.isEmpty()) {
    return base_result;
  }

  QString my_url = url.toString();
  QList<StandardFeed*> feeds;

  // 1. Test direct URL for a feed.
  // 2. Test embedded ATOM feed links from HTML data.
  // 3. Test "URL/feed" endpoint.
  // 4. Test "URL/atom" endpoint.
  // 5. If URL is Github repository, test for:
  //    https://github.com/:owner/:repo/releases.atom
  //    https://github.com/:owner/:repo/commits.atom
  //    https://github.com/:user/:repo/tags.atom
  // 6. If URL is Youtube, find channel ID.
  // 7. If URL is reddit, append ".rss".

  // Download URL.
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
  QString direct_html_data = QString::fromUtf8(data);

  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    try {
      // 1.
      auto guessed_feed = guessFeed(data, res);

      return {guessed_feed.first};
    }
    catch (...) {
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }

    // 2.
    static QRegularExpression rx(QSL(ATOM_REGEX_MATCHER), QRegularExpression::PatternOption::CaseInsensitiveOption);
    static QRegularExpression rx_href(QSL(ATOM_HREF_REGEX_MATCHER),
                                      QRegularExpression::PatternOption::CaseInsensitiveOption);

    rx.optimize();
    rx_href.optimize();

    QRegularExpressionMatchIterator it_rx = rx.globalMatch(direct_html_data);

    while (it_rx.hasNext()) {
      QRegularExpressionMatch mat_tx = it_rx.next();
      QString link_tag = mat_tx.captured();
      QString feed_link = rx_href.match(link_tag).captured(1);

      if (feed_link.startsWith(QL1S("//"))) {
        feed_link = QSL(URI_SCHEME_HTTP) + feed_link.mid(2);
      }
      else if (feed_link.startsWith(QL1C('/'))) {
        feed_link = url.toString(QUrl::UrlFormattingOption::RemovePath | QUrl::UrlFormattingOption::RemoveQuery |
                                 QUrl::UrlFormattingOption::StripTrailingSlash) +
                    feed_link;
      }

      QByteArray data;
      auto res = NetworkFactory::performNetworkOperation(feed_link,
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
          auto guessed_feed = guessFeed(data, res);

          feeds.append(guessed_feed.first);
        }
        catch (const ApplicationException& ex) {
          qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(feed_link)
                   << " should be direct link to feed file but was not recognized:" << QUOTE_W_SPACE_DOT(ex.message());
        }
      }
    }
  }
  else {
    logUnsuccessfulRequest(res);
  }

  // 3.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash) + QSL("/feed");
  res = NetworkFactory::performNetworkOperation(my_url,
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
      auto guessed_feed = guessFeed(data, res);

      feeds.append(guessed_feed.first);
    }
    catch (...) {
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }
  }
  else {
    logUnsuccessfulRequest(res);
  }

  // 4.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash) + QSL("/atom");
  res = NetworkFactory::performNetworkOperation(my_url,
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
      auto guessed_feed = guessFeed(data, res);

      feeds.append(guessed_feed.first);
    }
    catch (...) {
      qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
    }
  }
  else {
    logUnsuccessfulRequest(res);
  }

  // 5.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash);

  auto mtch_github = QRegularExpression(QSL(GITHUB_URL_REGEX)).match(my_url);

  if (mtch_github.isValid() && mtch_github.hasMatch()) {
    QStringList github_feeds = {QSL("releases.atom"), QSL("commits.atom"), QSL("tags.atom")};
    QString gh_username = mtch_github.captured(1);
    QString gh_repo = mtch_github.captured(2);

    for (const QString& github_feed : github_feeds) {
      my_url = QSL("https://github.com/%1/%2/%3").arg(gh_username, gh_repo, github_feed);
      res = NetworkFactory::performNetworkOperation(my_url,
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
          auto guessed_feed = guessFeed(data, res);

          feeds.append(guessed_feed.first);
        }
        catch (...) {
          qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
        }
      }
      else {
        logUnsuccessfulRequest(res);
      }
    }
  }

  // 6.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash);

  if (!direct_html_data.isEmpty() && my_url.contains(QSL("youtube"))) {
    QString youtube_channel_id =
      QRegularExpression(QSL("\"externalChannelId\": ?\"([^\"]+)\"")).match(direct_html_data).captured(1);

    if (youtube_channel_id.isEmpty()) {
      // Try to get channel ID differently.
      youtube_channel_id =
        QRegularExpression(QSL("{\"key\":\"browse_id\",\"value\":\"([^\"]+)\"}")).match(direct_html_data).captured(1);
    }

    if (!youtube_channel_id.isEmpty()) {
      my_url = QSL("https://www.youtube.com/feeds/videos.xml?channel_id=%1").arg(youtube_channel_id);
      res = NetworkFactory::performNetworkOperation(my_url,
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
          auto guessed_feed = guessFeed(data, res);

          feeds.append(guessed_feed.first);
        }
        catch (...) {
          qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
        }
      }
      else {
        logUnsuccessfulRequest(res);
      }
    }
  }

  // 7.
  my_url = url.toString(QUrl::UrlFormattingOption::StripTrailingSlash);

  if (my_url.contains(QSL("reddit.com")) && !my_url.endsWith(QSL(".rss"))) {
    my_url += QSL(".rss");
    res = NetworkFactory::performNetworkOperation(my_url,
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
        auto guessed_feed = guessFeed(data, res);

        feeds.append(guessed_feed.first);
      }
      catch (...) {
        qDebugNN << LOGSEC_STANDARD << QUOTE_W_SPACE(my_url) << "is not a direct feed file.";
      }
    }
    else {
      logUnsuccessfulRequest(res);
    }
  }

  return feeds;
}

QPair<StandardFeed*, QList<IconLocation>> AtomParser::guessFeed(const QByteArray& content,
                                                                const NetworkResult& network_res) const {
  QString xml_schema_encoding = QSL(DEFAULT_FEED_ENCODING);
  QString xml_contents_encoded;
  QString enc = DomDocument::extractEncoding(content);

  if (!enc.isEmpty()) {
    // Some "encoding" attribute was found get the encoding
    // out of it.
    xml_schema_encoding = enc;
  }

  QTextCodec* custom_codec = QTextCodec::codecForName(xml_schema_encoding.toLocal8Bit());

  if (custom_codec != nullptr) {
    xml_contents_encoded = custom_codec->toUnicode(content);
  }
  else {
    xml_contents_encoded = QString::fromUtf8(content);
  }

  // Feed XML was obtained, guess it now.
  DomDocument xml_document;
  QString error_msg;
  int error_line, error_column;

  if (!xml_document.setContent(xml_contents_encoded, true, &error_msg, &error_line, &error_column)) {
    throw ApplicationException(QObject::tr("XML is not well-formed, %1, line %2, column %3")
                                 .arg(error_msg)
                                 .arg(QString::number(error_line))
                                 .arg(QString::number(error_column)));
  }

  QDomElement root_element = xml_document.documentElement();

  if (root_element.namespaceURI() != atomNamespace()) {
    throw ApplicationException(QObject::tr("not an ATOM feed"));
  }

  auto* feed = new StandardFeed();
  QList<IconLocation> icon_possible_locations;

  feed->setEncoding(xml_schema_encoding);
  feed->setType(StandardFeed::Type::Atom10);
  feed->setTitle(root_element.namedItem(QSL("title")).toElement().text());
  feed->setDescription(root_element.namedItem(QSL("subtitle")).toElement().text());
  feed->setSource(network_res.m_url.toString());

  QString icon_link = root_element.namedItem(QSL("icon")).toElement().text();

  if (!icon_link.isEmpty()) {
    icon_possible_locations.append({icon_link, true});
  }

  QString home_page = root_element.namedItem(QSL("link")).toElement().attribute(QSL("href"));

  if (!home_page.isEmpty()) {
    icon_possible_locations.prepend({home_page, false});
  }

  return {feed, icon_possible_locations};
}

QString AtomParser::feedAuthor() const {
  auto authors = m_xml.documentElement().elementsByTagNameNS(m_atomNamespace, QSL("author"));

  for (int i = 0; i < authors.size(); i++) {
    QDomNode auth = authors.at(i);

    if (auth.parentNode() == m_xml.documentElement()) {
      return auth.toElement().elementsByTagNameNS(m_atomNamespace, QSL("name")).at(0).toElement().text();
    }
  }

  return {};
}

QString AtomParser::xmlMessageAuthor(const QDomElement& msg_element) const {
  QDomNodeList authors = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("author"));
  QStringList author_str;

  for (int i = 0; i < authors.size(); i++) {
    QDomNodeList names = authors.at(i).toElement().elementsByTagNameNS(m_atomNamespace, QSL("name"));

    if (!names.isEmpty()) {
      author_str.append(names.at(0).toElement().text());
    }
  }

  return author_str.join(QSL(", "));
}

QString AtomParser::atomNamespace() const {
  return m_atomNamespace;
}

QList<FeedComment> AtomParser::comments(const QDomElement& msg_element) const {
  QDomNodeList links = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("link"));

  for (int i = 0; i < links.size(); i++) {
    QDomElement link_elem = links.at(i).toElement();

    if (link_elem.attribute(QSL("rel")) != QSL("replies") ||
        link_elem.attribute(QSL("type")) != QSL("application/atom+xml")) {
      continue;
    }

    QString comments_atom = link_elem.attribute(QSL("href"));

    if (comments_atom.isEmpty()) {
      continue;
    }

    QByteArray comments_atom_data = m_resourceHandler(comments_atom);
    AtomParser atom_parser(QString::fromUtf8(comments_atom_data));
    QList<Message> extracted_comments = atom_parser.messages();

    if (extracted_comments.isEmpty()) {
      return {};
    }

    QList<FeedComment> cmnts;
    cmnts.reserve(extracted_comments.size());

    for (Message& extracted_comment : extracted_comments) {
      extracted_comment.sanitize(nullptr, false);

      FeedComment cmnt;
      cmnt.m_title = extracted_comment.m_title;
      cmnt.m_contents = extracted_comment.m_contents;
      cmnts.append(cmnt);
    }

    return cmnts;
  }

  return {};
}

QDomNodeList AtomParser::xmlMessageElements() {
  return m_xml.elementsByTagNameNS(m_atomNamespace, QSL("entry"));
}

QString AtomParser::xmlMessageTitle(const QDomElement& msg_element) const {
  return xmlTextsFromPath(msg_element, m_atomNamespace, QSL("title"), true).join(QSL(", "));
}

QString AtomParser::xmlMessageDescription(const QDomElement& msg_element) const {
  QString summary = xmlRawChild(msg_element.elementsByTagNameNS(m_atomNamespace, QSL("content")).at(0).toElement());

  if (summary.isEmpty()) {
    summary = xmlRawChild(msg_element.elementsByTagNameNS(m_atomNamespace, QSL("summary")).at(0).toElement());

    if (summary.isEmpty()) {
      summary = xmlRawChild(msg_element.elementsByTagNameNS(m_mrssNamespace, QSL("description")).at(0).toElement());
    }
  }

  if (fetchComments()) {
    summary += formatComments(comments(msg_element));
  }

  return summary;
}

QDateTime AtomParser::xmlMessageDateCreated(const QDomElement& msg_element) {
  QString updated = xmlTextsFromPath(msg_element, m_atomNamespace, QSL("updated"), true).join(QSL(", "));

  if (updated.simplified().isEmpty()) {
    updated = xmlTextsFromPath(msg_element, m_atomNamespace, QSL("modified"), true).join(QSL(", "));
  }

  return TextFactory::parseDateTime(updated, &m_dateTimeFormat);
}

QString AtomParser::xmlMessageId(const QDomElement& msg_element) const {
  return msg_element.elementsByTagNameNS(m_atomNamespace, QSL("id")).at(0).toElement().text();
}

QString AtomParser::xmlMessageUrl(const QDomElement& msg_element) const {
  QDomNodeList elem_links = msg_element.toElement().elementsByTagNameNS(m_atomNamespace, QSL("link"));
  QString last_link_other;

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString attribute = link.attribute(QSL("rel"));

    if (attribute.isEmpty() || attribute == QSL("alternate")) {
      return link.attribute(QSL("href"));
    }
    else if (attribute != QSL("enclosure")) {
      last_link_other = link.attribute(QSL("href"));
    }
  }

  if (!last_link_other.isEmpty()) {
    return last_link_other;
  }
  else {
    return {};
  }
}

QList<QSharedPointer<Enclosure>> AtomParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  QList<QSharedPointer<Enclosure>> enclosures;
  QDomNodeList elem_links = msg_element.elementsByTagNameNS(m_atomNamespace, QSL("link"));

  for (int i = 0; i < elem_links.size(); i++) {
    QDomElement link = elem_links.at(i).toElement();
    QString attribute = link.attribute(QSL("rel"));

    if (attribute == QSL("enclosure")) {
      enclosures.append(QSharedPointer<Enclosure>(new Enclosure(link.attribute(QSL("href")),
                                                                link.attribute(QSL("type")))));
    }
  }

  return enclosures;
}

QList<MessageCategory*> AtomParser::xmlMessageCategories(const QDomElement& msg_element) const {
  QList<MessageCategory*> cats;
  QDomNodeList elem_cats = msg_element.toElement().elementsByTagNameNS(m_atomNamespace, QSL("category"));

  for (int i = 0; i < elem_cats.size(); i++) {
    QDomElement cat = elem_cats.at(i).toElement();
    QString lbl = cat.attribute(QSL("label"));
    QString term = cat.attribute(QSL("term"));

    cats.append(new MessageCategory(lbl.isEmpty() ? term : lbl));
  }

  return cats;
}
