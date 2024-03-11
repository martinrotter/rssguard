// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/parsers/feedparser.h"

#include "exceptions/applicationexception.h"
#include "exceptions/feedfetchexception.h"
#include "miscellaneous/application.h"
#include "network-web/webfactory.h"
#include "services/standard/definitions.h"

#include <QDebug>
#include <QRegularExpression>

#include <utility>

FeedParser::FeedParser(QString data, DataType is_xml)
  : m_dataType(is_xml), m_data(std::move(data)), m_mrssNamespace(QSL("http://search.yahoo.com/mrss/")) {
  if (m_data.isEmpty()) {
    return;
  }

  if (m_dataType == DataType::Xml) {
    // XML.
    QString error;

    if (!m_xml.setContent(m_data, true, &error)) {
      throw FeedFetchException(Feed::Status::ParsingError, QObject::tr("XML problem: %1").arg(error));
    }
  }
  else if (m_dataType == DataType::Json) {
    // JSON.
    QJsonParseError err;

    m_json = QJsonDocument::fromJson(m_data.toUtf8(), &err);

    if (m_json.isNull() && err.error != QJsonParseError::ParseError::NoError) {
      throw FeedFetchException(Feed::Status::ParsingError, QObject::tr("JSON problem: %1").arg(err.errorString()));
    }
  }
}

FeedParser::~FeedParser() {}

QList<StandardFeed*> FeedParser::discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const {
  Q_UNUSED(root)
  Q_UNUSED(greedy)

  if (url.isLocalFile()) {
    QString file_path = url.toLocalFile();

    if (QFile::exists(file_path)) {
      try {
        auto guessed_feed = guessFeed(IOFactory::readFile(file_path), {});

        guessed_feed.first->setSourceType(StandardFeed::SourceType::LocalFile);
        guessed_feed.first->setSource(file_path);

        return {guessed_feed.first};
      }
      catch (...) {
        qDebugNN << LOGSEC_CORE << QUOTE_W_SPACE(file_path) << "is not a local feed file.";
      }
    }
  }

  return {};
}

QPair<StandardFeed*, QList<IconLocation>> FeedParser::guessFeed(const QByteArray& content,
                                                                const QString& content_type) const {
  return {};
}

QString FeedParser::xmlMessageRawContents(const QDomElement& msg_element) const {
  QString raw_contents;
  QTextStream str(&raw_contents);

  msg_element.save(str, 0, QDomNode::EncodingPolicy::EncodingFromTextStream);
  return raw_contents;
}

QJsonArray FeedParser::jsonMessageElements() {
  return {};
}

QString FeedParser::jsonMessageTitle(const QJsonObject& msg_element) const {
  return {};
}

QString FeedParser::jsonMessageUrl(const QJsonObject& msg_element) const {
  return {};
}

QString FeedParser::jsonMessageDescription(const QJsonObject& msg_element) const {
  return {};
}

QString FeedParser::jsonMessageAuthor(const QJsonObject& msg_element) const {
  return {};
}

QDateTime FeedParser::jsonMessageDateCreated(const QJsonObject& msg_element) const {
  return {};
}

QString FeedParser::jsonMessageId(const QJsonObject& msg_element) const {
  return {};
}

QList<Enclosure> FeedParser::jsonMessageEnclosures(const QJsonObject& msg_element) const {
  return {};
}

QList<MessageCategory> FeedParser::jsonMessageCategories(const QJsonObject& msg_element) const {
  return {};
}

QString FeedParser::jsonMessageRawContents(const QJsonObject& msg_element) const {
  return {};
}

QVariantList FeedParser::objMessageElements() {
  return {};
}

QString FeedParser::objMessageTitle(const QVariant& msg_element) const {
  return {};
}

QString FeedParser::objMessageUrl(const QVariant& msg_element) const {
  return {};
}

QString FeedParser::objMessageDescription(const QVariant& msg_element) const {
  return {};
}

QString FeedParser::objMessageAuthor(const QVariant& msg_element) const {
  return {};
}

QDateTime FeedParser::objMessageDateCreated(const QVariant& msg_element) const {
  return {};
}

QString FeedParser::objMessageId(const QVariant& msg_element) const {
  return {};
}

QList<Enclosure> FeedParser::objMessageEnclosures(const QVariant& msg_element) const {
  return {};
}

QList<MessageCategory> FeedParser::objMessageCategories(const QVariant& msg_element) const {
  return {};
}

QString FeedParser::objMessageRawContents(const QVariant& msg_element) const {
  return {};
}

QList<Message> FeedParser::messages() {
  QString feed_author = feedAuthor();
  QList<Message> messages;
  QDateTime current_time = QDateTime::currentDateTimeUtc();

  // Pull out all messages.
  if (m_dataType == DataType::Xml) {
    QDomNodeList messages_in_xml = xmlMessageElements();

    for (int i = 0; i < messages_in_xml.size(); i++) {
      QDomElement message_item = messages_in_xml.item(i).toElement();

      try {
        Message new_message;

        // Fill available data.
        new_message.m_title = xmlMessageTitle(message_item);
        new_message.m_contents = xmlMessageDescription(message_item);
        new_message.m_author = xmlMessageAuthor(message_item);
        new_message.m_url = xmlMessageUrl(message_item);
        new_message.m_created = xmlMessageDateCreated(message_item);
        new_message.m_customId = xmlMessageId(message_item);
        new_message.m_rawContents = xmlMessageRawContents(message_item);
        new_message.m_enclosures = xmlMessageEnclosures(message_item);
        new_message.m_enclosures.append(xmlMrssGetEnclosures(message_item));
        new_message.m_categories.append(xmlMessageCategories(message_item));

        messages.append(new_message);
      }
      catch (const ApplicationException& ex) {
        qDebugNN << LOGSEC_CORE << "Problem when extracting XML message: " << ex.message();
      }
    }
  }
  else if (m_dataType == DataType::Json) {
    QJsonArray messages_in_json = jsonMessageElements();

    for (int i = 0; i < messages_in_json.size(); i++) {
      QJsonObject message_item = messages_in_json.at(i).toObject();

      try {
        Message new_message;

        // Fill available data.
        new_message.m_title = jsonMessageTitle(message_item);
        new_message.m_contents = jsonMessageDescription(message_item);
        new_message.m_author = jsonMessageAuthor(message_item);
        new_message.m_url = jsonMessageUrl(message_item);
        new_message.m_created = jsonMessageDateCreated(message_item);
        new_message.m_customId = jsonMessageId(message_item);
        new_message.m_rawContents = jsonMessageRawContents(message_item);
        new_message.m_enclosures = jsonMessageEnclosures(message_item);

        messages.append(new_message);
      }
      catch (const ApplicationException& ex) {
        qDebugNN << LOGSEC_CORE << "Problem when extracting JSON message: " << ex.message();
      }
    }
  }
  else if (m_dataType == DataType::Other) {
    auto messages_in_obj = objMessageElements();

    for (const QVariant& message_item : messages_in_obj) {
      try {
        Message new_message;

        // Fill available data.
        new_message.m_title = objMessageTitle(message_item);
        new_message.m_contents = objMessageDescription(message_item);
        new_message.m_author = objMessageAuthor(message_item);
        new_message.m_url = objMessageUrl(message_item);
        new_message.m_created = objMessageDateCreated(message_item);
        new_message.m_customId = objMessageId(message_item);
        new_message.m_rawContents = objMessageRawContents(message_item);
        new_message.m_enclosures = objMessageEnclosures(message_item);

        messages.append(new_message);
      }
      catch (const ApplicationException& ex) {
        qDebugNN << LOGSEC_CORE << "Problem when extracting OBJ message: " << ex.message();
      }
    }
  }

  // Fixup missing data.
  //
  // NOTE: Message must have "title" field, otherwise it is skipped.
  for (int i = 0; i < messages.size(); i++) {
    Message& new_message = messages[i];

    // Title.
    if (new_message.m_title.simplified().isEmpty()) {
      if (new_message.m_url.simplified().isEmpty()) {
        messages.removeAt(i--);
        continue;
      }
      else {
        new_message.m_title = new_message.m_url;
      }
    }

    // Author.
    if (new_message.m_author.isEmpty() && !feed_author.isEmpty()) {
      new_message.m_author = feed_author;
    }

    // Created date.
    new_message.m_createdFromFeed = !new_message.m_created.isNull();

    if (!new_message.m_createdFromFeed) {
      // Date was NOT obtained from the feed, set current date as creation date for the message.
      // NOTE: Date is lessened by 1 second for each message to allow for more
      // stable sorting.
      new_message.m_created = current_time.addSecs(-1);
      current_time = new_message.m_created;
    }

    // Enclosures.
    for (Enclosure& enc : new_message.m_enclosures) {
      if (enc.m_mimeType.simplified().isEmpty()) {
        enc.m_mimeType = QSL(DEFAULT_ENCLOSURE_MIME_TYPE);
      }
    }

    // Url.
    static QRegularExpression reg_non_url(QSL("[\\t\\n]"));

    new_message.m_url = new_message.m_url.replace(reg_non_url, {});
  }

  return messages;
}

QList<Enclosure> FeedParser::xmlMrssGetEnclosures(const QDomElement& msg_element) const {
  QList<Enclosure> enclosures;
  auto content_list = msg_element.elementsByTagNameNS(m_mrssNamespace, QSL("content"));

  for (int i = 0; i < content_list.size(); i++) {
    QDomElement elem_content = content_list.at(i).toElement();
    QString url = elem_content.attribute(QSL("url"));
    QString type = elem_content.attribute(QSL("type"));

    if (type.isEmpty()) {
      type = QSL(DEFAULT_ENCLOSURE_MIME_TYPE);
    }

    if (!url.isEmpty() && !type.isEmpty()) {
      enclosures.append(Enclosure(url, type));
    }
  }

  auto thumbnail_list = msg_element.elementsByTagNameNS(m_mrssNamespace, QSL("thumbnail"));

  for (int i = 0; i < thumbnail_list.size(); i++) {
    QDomElement elem_content = thumbnail_list.at(i).toElement();
    QString url = elem_content.attribute(QSL("url"));

    if (!url.isEmpty()) {
      enclosures.append(Enclosure(url, QSL(DEFAULT_ENCLOSURE_MIME_TYPE)));
    }
  }

  return enclosures;
}

QString FeedParser::xmlMrssTextFromPath(const QDomElement& msg_element, const QString& xml_path) const {
  QString text = msg_element.elementsByTagNameNS(m_mrssNamespace, xml_path).at(0).toElement().text();

  return text;
}

QString FeedParser::xmlRawChild(const QDomElement& container) const {
  QString raw;
  auto children = container.childNodes();

  for (int i = 0; i < children.size(); i++) {
    if (children.at(i).isCDATASection()) {
      raw += children.at(i).toCDATASection().data();
    }
    else {
      QString raw_ch;
      QTextStream str(&raw_ch);

      children.at(i).save(str, 0);
      raw += qApp->web()->unescapeHtml(raw_ch);
    }
  }

  return raw;
}

QStringList FeedParser::xmlTextsFromPath(const QDomElement& element,
                                         const QString& namespace_uri,
                                         const QString& xml_path,
                                         bool only_first) const {
  QStringList paths = xml_path.split('/');
  QStringList result;
  QList<QDomElement> current_elements;

  current_elements.append(element);

  while (!paths.isEmpty()) {
    QList<QDomElement> next_elements;
    QString next_local_name = paths.takeFirst();

    for (const QDomElement& elem : current_elements) {
      QDomNodeList elements = elem.elementsByTagNameNS(namespace_uri, next_local_name);

      for (int i = 0; i < elements.size(); i++) {
        next_elements.append(elements.at(i).toElement());

        if (only_first) {
          break;
        }
      }

      if (next_elements.size() == 1 && only_first) {
        break;
      }
    }

    current_elements = next_elements;
  }

  if (!current_elements.isEmpty()) {
    for (const QDomElement& elem : std::as_const(current_elements)) {
      result.append(elem.text());
    }
  }

  return result;
}

QString FeedParser::feedAuthor() const {
  return QL1S("");
}

QDomNodeList FeedParser::xmlMessageElements() {
  return {};
}

QString FeedParser::xmlMessageTitle(const QDomElement& msg_element) const {
  return {};
}

QString FeedParser::xmlMessageUrl(const QDomElement& msg_element) const {
  return {};
}

QString FeedParser::xmlMessageDescription(const QDomElement& msg_element) const {
  return {};
}

QString FeedParser::xmlMessageAuthor(const QDomElement& msg_element) const {
  return {};
}

QDateTime FeedParser::xmlMessageDateCreated(const QDomElement& msg_element) const {
  return {};
}

QString FeedParser::xmlMessageId(const QDomElement& msg_element) const {
  return {};
}

QList<Enclosure> FeedParser::xmlMessageEnclosures(const QDomElement& msg_element) const {
  return {};
}

QList<MessageCategory> FeedParser::xmlMessageCategories(const QDomElement& msg_element) const {
  return {};
}
