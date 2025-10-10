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

#define MSG_FROM_REC                                                                                               \
  message.m_id = record.value(MSG_DB_ID_INDEX).toInt();                                                            \
  message.m_isRead = record.value(MSG_DB_READ_INDEX).toBool();                                                     \
  message.m_isImportant = record.value(MSG_DB_IMPORTANT_INDEX).toBool();                                           \
  message.m_isDeleted = record.value(MSG_DB_DELETED_INDEX).toBool();                                               \
  message.m_isPdeleted = record.value(MSG_DB_PDELETED_INDEX).toBool();                                             \
  message.m_feedId = record.value(MSG_DB_FEED_CUSTOM_ID_INDEX).toInt();                                            \
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

MessageEnclosure::MessageEnclosure(QString url, QString mime, QObject* parent)
  : QObject(parent), m_url(std::move(url)), m_mimeType(std::move(mime)) {}

MessageEnclosure::MessageEnclosure(const MessageEnclosure& other) {
  setMimeType(other.mimeType());
  setUrl(other.url());
}

MessageEnclosure::~MessageEnclosure() {
  qDebugNN << LOGSEC_CORE << "Destroying article enclosure.";
}

QString MessageEnclosure::url() const {
  return m_url;
}

void MessageEnclosure::setUrl(const QString& url) {
  m_url = url;
}

QString MessageEnclosure::mimeType() const {
  return m_mimeType;
}

void MessageEnclosure::setMimeType(const QString& mime) {
  m_mimeType = mime;
}

QList<QSharedPointer<MessageEnclosure>> Enclosures::decodeEnclosuresFromString(const QString& enclosures_data) {
  QJsonParseError enc_err;
  QJsonDocument enc_doc = QJsonDocument::fromJson(enclosures_data.toUtf8(), &enc_err);
  QList<QSharedPointer<MessageEnclosure>> enclosures;
  QJsonArray enc_arr = enc_doc.array();

  for (const QJsonValue& enc_val : enc_arr) {
    const QJsonObject& enc_obj = enc_val.toObject();

    MessageEnclosure* enclosure = new MessageEnclosure();

    enclosure->setMimeType(enc_obj.value(QSL("mime")).toString());
    enclosure->setUrl(enc_obj.value(QSL("url")).toString());

    enclosures.append(QSharedPointer<MessageEnclosure>(enclosure));
  }

  return enclosures;
}

QString Enclosures::encodeEnclosuresToString(const QList<QSharedPointer<MessageEnclosure>>& enclosures) {
  QJsonArray enc_arr;

  for (const QSharedPointer<MessageEnclosure>& enc : enclosures) {
    QJsonObject enc_obj;

    enc_obj.insert(QSL("mime"), enc->mimeType());
    enc_obj.insert(QSL("url"), enc->url());

    enc_arr.append(enc_obj);
  }

  return QJsonDocument(enc_arr).toJson(QJsonDocument::JsonFormat::Compact);
}

Message::Message() {
  m_title = m_url = m_author = m_contents = m_rawContents = m_feedTitle = m_customId = m_customHash = QL1S("");
  m_accountId = m_id = m_feedId = 0;
  m_score = 0.0;
  m_isRead = m_isImportant = m_isDeleted = m_isPdeleted = false;
  m_rtlBehavior = RtlBehavior::NoRtl;
}

Message::Message(const Message& other) {
  m_title = other.m_title;
  m_url = other.m_url;
  m_author = other.m_author;
  m_contents = other.m_contents;
  m_rawContents = other.m_rawContents;
  m_feedId = other.m_feedId;
  m_feedTitle = other.m_feedTitle;
  m_customId = other.m_customId;
  m_customHash = other.m_customHash;
  m_created = other.m_created;
  m_createdFromFeed = other.m_createdFromFeed;
  m_insertedUpdated = other.m_insertedUpdated;

  m_enclosures = other.m_enclosures;
  m_categories = other.m_categories;

  m_accountId = other.m_accountId;
  m_id = other.m_id;
  m_score = other.m_score;
  m_isRead = other.m_isRead;
  m_isImportant = other.m_isImportant;
  m_isDeleted = other.m_isDeleted;
  m_isPdeleted = other.m_isPdeleted;
  m_rtlBehavior = other.m_rtlBehavior;
  m_assignedLabels = other.m_assignedLabels;
  m_assignedLabelsByFilter = other.m_assignedLabelsByFilter;
  m_deassignedLabelsByFilter = other.m_deassignedLabelsByFilter;
  m_assignedLabelsIds = other.m_assignedLabelsIds;
}

Message::~Message() {
  // qDeleteAll(m_categories);
  // m_categories.clear();

  // qDeleteAll(m_enclosures);
  // m_enclosures.clear();
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
      << my_obj.m_isImportant << my_obj.m_isRead << my_obj.m_isDeleted << my_obj.m_isPdeleted << my_obj.m_score
      << my_obj.m_rtlBehavior;

  return out;
}

QDataStream& operator>>(QDataStream& in, Message& my_obj) {
  int account_id;
  QString custom_hash;
  QString custom_id;
  int feed_id;
  int id;
  bool is_important;
  bool is_read;
  bool is_deleted;
  bool is_pdeleted;
  RtlBehavior is_rtl;
  double score;

  in >> account_id >> custom_hash >> custom_id >> feed_id >> id >> is_important >> is_read >> is_deleted >>
    is_pdeleted >> score >> is_rtl;

  my_obj.m_accountId = account_id;
  my_obj.m_customHash = custom_hash;
  my_obj.m_customId = custom_id;
  my_obj.m_feedId = feed_id;
  my_obj.m_id = id;
  my_obj.m_isImportant = is_important;
  my_obj.m_isRead = is_read;
  my_obj.m_isDeleted = is_deleted;
  my_obj.m_isPdeleted = is_pdeleted;
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

MessageCategory::~MessageCategory() {
  qDebugNN << LOGSEC_CORE << "Destroying article category.";
}

QString MessageCategory::title() const {
  return m_title;
}

MessageCategory& MessageCategory::operator=(const MessageCategory& other) {
  m_title = other.m_title;

  return *this;
}
