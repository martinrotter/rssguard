// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/message.h"

#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/label.h"

#include <QDebug>
#include <QFlags>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QVariant>
#include <QVector>

Enclosure::Enclosure(QString url, QString mime) : m_url(std::move(url)), m_mimeType(std::move(mime)) {}

QList<Enclosure> Enclosures::decodeEnclosuresFromString(const QString& enclosures_data) {
  QJsonParseError enc_err;
  QJsonDocument enc_doc = QJsonDocument::fromJson(enclosures_data.toUtf8(), &enc_err);
  QList<Enclosure> enclosures;

  if (enc_err.error != QJsonParseError::ParseError::NoError) {
    // Provide backwards compatibility.
    auto enc = enclosures_data.split(ENCLOSURES_OUTER_SEPARATOR, SPLIT_BEHAVIOR::SkipEmptyParts);

    enclosures.reserve(enc.size());

    for (const QString& single_enclosure : std::as_const(enc)) {
      Enclosure enclosure;

      if (single_enclosure.contains(ECNLOSURES_INNER_SEPARATOR)) {
        QStringList mime_url = single_enclosure.split(ECNLOSURES_INNER_SEPARATOR);

        enclosure.m_mimeType = QString::fromUtf8(QByteArray::fromBase64(mime_url.at(0).toLocal8Bit()));
        enclosure.m_url = QString::fromUtf8(QByteArray::fromBase64(mime_url.at(1).toLocal8Bit()));
      }
      else {
        enclosure.m_url = QString::fromUtf8(QByteArray::fromBase64(single_enclosure.toLocal8Bit()));
      }

      enclosures.append(enclosure);
    }
  }
  else {
    QJsonArray enc_arr = enc_doc.array();

    for (const QJsonValue& enc_val : enc_arr) {
      const QJsonObject& enc_obj = enc_val.toObject();

      Enclosure enclosure;

      enclosure.m_mimeType = enc_obj.value(QSL("mime")).toString();
      enclosure.m_url = enc_obj.value(QSL("url")).toString();

      enclosures.append(enclosure);
    }
  }

  return enclosures;
}

QJsonArray Enclosures::encodeEnclosuresToJson(const QList<Enclosure>& enclosures) {
  QJsonArray enc_arr;

  for (const Enclosure& enc : enclosures) {
    QJsonObject enc_obj;

    enc_obj.insert(QSL("mime"), enc.m_mimeType);
    enc_obj.insert(QSL("url"), enc.m_url);

    enc_arr.append(enc_obj);
  }

  return enc_arr;
}

QString Enclosures::encodeEnclosuresToString(const QList<Enclosure>& enclosures) {
  return QJsonDocument(encodeEnclosuresToJson(enclosures)).toJson(QJsonDocument::JsonFormat::Compact);

  /*
  QStringList enclosures_str;

  for (const Enclosure& enclosure : enclosures) {
    if (enclosure.m_mimeType.isEmpty()) {
      enclosures_str.append(enclosure.m_url.toUtf8().toBase64());
    }
    else {
      enclosures_str.append(QString(enclosure.m_mimeType.toUtf8().toBase64()) + ECNLOSURES_INNER_SEPARATOR +
                            enclosure.m_url.toUtf8().toBase64());
    }
  }

  return enclosures_str.join(QString(ENCLOSURES_OUTER_SEPARATOR));
  */
}

Message::Message() {
  m_title = m_url = m_author = m_contents = m_rawContents = m_feedId = m_feedTitle = m_customId = m_customHash =
    QL1S("");
  m_enclosures = QList<Enclosure>();
  m_categories = QList<MessageCategory*>();
  m_accountId = m_id = 0;
  m_score = 0.0;
  m_isRead = m_isImportant = m_isDeleted = false;
  m_rtlBehavior = RtlBehavior::NoRtl;
  m_assignedLabels = QList<Label*>();
  m_assignedLabelsByFilter = QList<Label*>();
  m_deassignedLabelsByFilter = QList<Label*>();
}

void Message::sanitize(const Feed* feed, bool fix_future_datetimes) {
  static QRegularExpression reg_spaces(QString::fromUtf8(QByteArray("[\xE2\x80\xAF]")));
  static QRegularExpression reg_whites(QSL("[\\s]{2,}"));
  static QRegularExpression reg_nul(QSL("\\x00"));
  static QRegularExpression reg_news(QSL("([\\n\\r])|(^\\s)"));

  // Sanitize title.
  m_title = qApp->web()->stripTags(WebFactory::unescapeHtml(m_title));

  m_title = m_title
              .trimmed()

              // Remove non-breaking spaces.
              .replace(reg_spaces, QSL(" "))

              // Shrink consecutive whitespaces.
              .replace(reg_whites, QSL(" "))

              // Remove all newlines and leading white space.
              .remove(reg_news)

              // Remove non-breaking zero-width spaces.
              .remove(QChar(65279));

  // Sanitize author.
  m_author = qApp->web()->stripTags(WebFactory::unescapeHtml(m_author));

  // Remove NUL (0) bytes.
  m_contents = m_contents.remove(reg_nul);

  // Sanitize URL.
  m_url = m_url.trimmed();

  // Check if messages contain relative URLs and if they do, then replace them.
  if (m_url.startsWith(QL1S("//"))) {
    m_url = QSL(URI_SCHEME_HTTPS) + m_url.mid(2);
  }
  else if (feed != nullptr && QUrl(m_url).isRelative()) {
    QUrl feed_url(feed->source());

    if (feed_url.isValid()) {
      QUrl feed_homepage_url = QUrl(feed_url.scheme() + QSL("://") + feed_url.host());

      feed_homepage_url.setPort(feed_url.port());
      m_url = feed_homepage_url.resolved(m_url).toString();
    }
  }

  // Fix datetimes in future.
  if ((fix_future_datetimes && m_createdFromFeed && m_created.toUTC() > QDateTime::currentDateTimeUtc()) ||
      (m_createdFromFeed && (!m_created.isValid() || m_created.toSecsSinceEpoch() < 0))) {
    qWarningNN << LOGSEC_CORE << "Fixing date of article" << QUOTE_W_SPACE(m_title) << "from invalid date/time"
               << QUOTE_W_SPACE_DOT(m_created);
    m_createdFromFeed = false;
    m_created = QDateTime::currentDateTimeUtc();
  }
}

void Message::deallocateCategories() {
  qDeleteAll(m_categories);
  m_categories.clear();
}

QJsonObject Message::toJson() const {
  QJsonObject obj;

  obj.insert(QSL("contents"), m_contents);
  obj.insert(QSL("is_read"), m_isRead);
  obj.insert(QSL("is_important"), m_isImportant);
  obj.insert(QSL("title"), m_title);
  obj.insert(QSL("date_created"), m_created.toMSecsSinceEpoch());
  obj.insert(QSL("author"), m_author);
  obj.insert(QSL("url"), m_url);
  obj.insert(QSL("id"), m_id);
  obj.insert(QSL("custom_id"), m_customId);
  obj.insert(QSL("account_id"), m_accountId);
  obj.insert(QSL("custom_hash"), m_customHash);
  obj.insert(QSL("feed_custom_id"), m_feedId);
  obj.insert(QSL("feed_title"), m_feedTitle);
  obj.insert(QSL("is_rtl"), int(m_rtlBehavior));
  obj.insert(QSL("enclosures"), Enclosures::encodeEnclosuresToJson(m_enclosures));

  return obj;
}

#define MSG_FROM_REC                                                                                               \
  message.m_id = record.value(MSG_DB_ID_INDEX).toInt();                                                            \
  message.m_isRead = record.value(MSG_DB_READ_INDEX).toBool();                                                     \
  message.m_isImportant = record.value(MSG_DB_IMPORTANT_INDEX).toBool();                                           \
  message.m_isDeleted = record.value(MSG_DB_DELETED_INDEX).toBool();                                               \
  message.m_feedId = record.value(MSG_DB_FEED_CUSTOM_ID_INDEX).toString();                                         \
  message.m_feedTitle = record.value(MSG_DB_FEED_TITLE_INDEX).toString();                                          \
  message.m_title = record.value(MSG_DB_TITLE_INDEX).toString();                                                   \
  message.m_url = record.value(MSG_DB_URL_INDEX).toString();                                                       \
  message.m_author = record.value(MSG_DB_AUTHOR_INDEX).toString();                                                 \
  message.m_created = TextFactory::parseDateTime(record.value(MSG_DB_DCREATED_INDEX).value<qint64>());             \
  message.m_contents = record.value(MSG_DB_CONTENTS_INDEX).toString();                                             \
  message.m_enclosures = Enclosures::decodeEnclosuresFromString(record.value(MSG_DB_ENCLOSURES_INDEX).toString()); \
  message.m_score = record.value(MSG_DB_SCORE_INDEX).toDouble();                                                   \
  message.m_rtlBehavior = record.value(MSG_DB_FEED_IS_RTL_INDEX).value<RtlBehavior>();                             \
  message.m_accountId = record.value(MSG_DB_ACCOUNT_ID_INDEX).toInt();                                             \
  message.m_customId = record.value(MSG_DB_CUSTOM_ID_INDEX).toString();                                            \
  message.m_customHash = record.value(MSG_DB_CUSTOM_HASH_INDEX).toString();                                        \
  message.m_assignedLabelsIds = record.value(MSG_DB_LABELS_IDS).toString().split('.', SPLIT_BEHAVIOR::SkipEmptyParts);

Message Message::fromSqlQuery(const QSqlQuery& record) {
  Message message;

  MSG_FROM_REC

  return message;
}

Message Message::fromSqlRecord(const QSqlRecord& record, bool* result) {
  Message message;

  MSG_FROM_REC

  if (result != nullptr) {
    *result = true;
  }

  return message;
}

QString Message::generateRawAtomContents(const Message& msg) {
  return QSL("<entry>"
             "<title>%1</title>"
             "<link href=\"%2\" rel=\"alternate\" type=\"text/html\" title=\"%1\"/>"
             "<published>%3</published>"
             "<author><name>%6</name></author>"
             "<updated>%3</updated>"
             "<id>%4</id>"
             "<summary type=\"html\">%5</summary>"
             "</entry>")
    .arg(msg.m_title,
         msg.m_url,
         msg.m_created.toUTC().toString(QSL("yyyy-MM-ddThh:mm:ss")),
         msg.m_url,
         msg.m_contents.toHtmlEscaped(),
         msg.m_author);
}

QDataStream& operator<<(QDataStream& out, const Message& my_obj) {
  out << my_obj.m_accountId << my_obj.m_customHash << my_obj.m_customId << my_obj.m_feedId << my_obj.m_id
      << my_obj.m_isImportant << my_obj.m_isRead << my_obj.m_isDeleted << my_obj.m_score << my_obj.m_rtlBehavior;

  return out;
}

QDataStream& operator>>(QDataStream& in, Message& my_obj) {
  int account_id;
  QString custom_hash;
  QString custom_id;
  QString feed_id;
  int id;
  bool is_important;
  bool is_read;
  bool is_deleted;
  RtlBehavior is_rtl;
  double score;

  in >> account_id >> custom_hash >> custom_id >> feed_id >> id >> is_important >> is_read >> is_deleted >> score >>
    is_rtl;

  my_obj.m_accountId = account_id;
  my_obj.m_customHash = custom_hash;
  my_obj.m_customId = custom_id;
  my_obj.m_feedId = feed_id;
  my_obj.m_id = id;
  my_obj.m_isImportant = is_important;
  my_obj.m_isRead = is_read;
  my_obj.m_isDeleted = is_deleted;
  my_obj.m_score = score;
  my_obj.m_rtlBehavior = is_rtl;

  return in;
}

uint qHash(const Message& key, uint seed) {
  Q_UNUSED(seed)
  return (uint(key.m_accountId) * 10000) + uint(key.m_id);
}

uint qHash(const Message& key) {
  return (uint(key.m_accountId) * 10000) + uint(key.m_id);
}

MessageCategory::MessageCategory(const QString& title, QObject* parent) : QObject(parent), m_title(title) {}

MessageCategory::MessageCategory(const MessageCategory& other) {
  m_title = other.m_title;
}

QString MessageCategory::title() const {
  return m_title;
}

MessageCategory& MessageCategory::operator=(const MessageCategory& other) {
  m_title = other.m_title;

  return *this;
}
