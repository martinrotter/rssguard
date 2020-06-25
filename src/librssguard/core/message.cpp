// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/message.h"

#include "miscellaneous/textfactory.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

Enclosure::Enclosure(QString url, QString mime) : m_url(std::move(url)), m_mimeType(std::move(mime)) {}

QList<Enclosure> Enclosures::decodeEnclosuresFromString(const QString& enclosures_data) {
  QList<Enclosure> enclosures;

  for (const QString& single_enclosure : enclosures_data.split(ENCLOSURES_OUTER_SEPARATOR, QString::SkipEmptyParts)) {
    Enclosure enclosure;

    if (single_enclosure.contains(ECNLOSURES_INNER_SEPARATOR)) {
      QStringList mime_url = single_enclosure.split(ECNLOSURES_INNER_SEPARATOR);

      enclosure.m_mimeType = QByteArray::fromBase64(mime_url.at(0).toLocal8Bit());
      enclosure.m_url = QByteArray::fromBase64(mime_url.at(1).toLocal8Bit());
    }
    else {
      enclosure.m_url = QByteArray::fromBase64(single_enclosure.toLocal8Bit());
    }

    enclosures.append(enclosure);
  }

  return enclosures;
}

QString Enclosures::encodeEnclosuresToString(const QList<Enclosure>& enclosures) {
  QStringList enclosures_str;

  for (const Enclosure& enclosure : enclosures) {
    if (enclosure.m_mimeType.isEmpty()) {
      enclosures_str.append(enclosure.m_url.toLocal8Bit().toBase64());
    }
    else {
      enclosures_str.append(QString(enclosure.m_mimeType.toLocal8Bit().toBase64()) +
                            ECNLOSURES_INNER_SEPARATOR +
                            enclosure.m_url.toLocal8Bit().toBase64());
    }
  }

  return enclosures_str.join(QString(ENCLOSURES_OUTER_SEPARATOR));
}

Message::Message() {
  m_title = m_url = m_author = m_contents = m_feedId = m_customId = m_customHash = "";
  m_enclosures = QList<Enclosure>();
  m_accountId = m_id = 0;
  m_isRead = m_isImportant = false;
}

Message Message::fromSqlRecord(const QSqlRecord& record, bool* result) {
  if (record.count() != MSG_DB_HAS_ENCLOSURES + 1) {
    if (result != nullptr) {
      *result = false;
    }

    return Message();
  }

  Message message;

  message.m_id = record.value(MSG_DB_ID_INDEX).toInt();
  message.m_isRead = record.value(MSG_DB_READ_INDEX).toBool();
  message.m_isImportant = record.value(MSG_DB_IMPORTANT_INDEX).toBool();
  message.m_feedId = record.value(MSG_DB_FEED_CUSTOM_ID_INDEX).toString();
  message.m_title = record.value(MSG_DB_TITLE_INDEX).toString();
  message.m_url = record.value(MSG_DB_URL_INDEX).toString();
  message.m_author = record.value(MSG_DB_AUTHOR_INDEX).toString();
  message.m_created = TextFactory::parseDateTime(record.value(MSG_DB_DCREATED_INDEX).value<qint64>());
  message.m_contents = record.value(MSG_DB_CONTENTS_INDEX).toString();
  message.m_enclosures = Enclosures::decodeEnclosuresFromString(record.value(MSG_DB_ENCLOSURES_INDEX).toString());
  message.m_accountId = record.value(MSG_DB_ACCOUNT_ID_INDEX).toInt();
  message.m_customId = record.value(MSG_DB_CUSTOM_ID_INDEX).toString();
  message.m_customHash = record.value(MSG_DB_CUSTOM_HASH_INDEX).toString();

  if (result != nullptr) {
    *result = true;
  }

  return message;
}

QDataStream& operator<<(QDataStream& out, const Message& myObj) {
  out << myObj.m_accountId
      << myObj.m_customHash
      << myObj.m_customId
      << myObj.m_feedId
      << myObj.m_id
      << myObj.m_isImportant
      << myObj.m_isRead;

  return out;
}

QDataStream& operator>>(QDataStream& in, Message& myObj) {
  int accountId;
  QString customHash;
  QString customId;
  QString feedId;
  int id;
  bool isImportant;
  bool isRead;

  in >> accountId >> customHash >> customId >> feedId >> id >> isImportant >> isRead;

  myObj.m_accountId = accountId;
  myObj.m_customHash = customHash;
  myObj.m_customId = customId;
  myObj.m_feedId = feedId;
  myObj.m_id = id;
  myObj.m_isImportant = isImportant;
  myObj.m_isRead = isRead;

  return in;
}

uint qHash(const Message& key, uint seed) {
  Q_UNUSED(seed)
  return (uint(key.m_accountId) * 10000) + uint(key.m_id);
}

uint qHash(const Message& key) {
  return (uint(key.m_accountId) * 10000) + uint(key.m_id);
}

MessageObject::MessageObject(QSqlDatabase* db, const QString& feed_custom_id, int account_id, QObject* parent)
  : QObject(parent), m_db(db), m_feedCustomId(feed_custom_id), m_accountId(account_id), m_message(nullptr) {}

void MessageObject::setMessage(Message* message) {
  m_message = message;
}

bool MessageObject::isDuplicateWithAttribute(int attribute_check) const {
  if (attribute_check <= 0) {
    qCritical("Bad DuplicationAttributeCheck value '%d' was passed from JS filter script.", attribute_check);
    return true;
  }

  // Check database according to duplication attribute_check.
  DuplicationAttributeCheck attrs = static_cast<DuplicationAttributeCheck>(attribute_check);
  QSqlQuery q(*m_db);
  QStringList where_clauses;
  QList<QPair<QString, QVariant>> bind_values;

  // Now we construct the query according to parameter.
  if ((attrs& DuplicationAttributeCheck::SameTitle) == DuplicationAttributeCheck::SameTitle) {
    where_clauses.append(QSL("title = :title"));
    bind_values.append({":title", title()});
  }

  if ((attrs& DuplicationAttributeCheck::SameUrl) == DuplicationAttributeCheck::SameUrl) {
    where_clauses.append(QSL("url = :url"));
    bind_values.append({":url", url()});
  }

  if ((attrs& DuplicationAttributeCheck::SameAuthor) == DuplicationAttributeCheck::SameAuthor) {
    where_clauses.append(QSL("author = :author"));
    bind_values.append({":author", author()});
  }

  if ((attrs& DuplicationAttributeCheck::SameDateCreated) == DuplicationAttributeCheck::SameDateCreated) {
    where_clauses.append(QSL("date_created = :date_created"));
    bind_values.append({":date_created", created().toMSecsSinceEpoch()});
  }

  where_clauses.append(QSL("account_id = :account_id"));
  bind_values.append({":account_id", accountId()});

  if ((attrs& DuplicationAttributeCheck::AllFeedsSameAccount) != DuplicationAttributeCheck::AllFeedsSameAccount) {
    // Limit to current feed.
    where_clauses.append(QSL("feed = :feed"));
    bind_values.append({":feed", feedCustomId()});
  }

  QString full_query = QSL("SELECT COUNT(*) FROM Messages WHERE ") + where_clauses.join(QSL(" AND ")) + QSL(";");

  q.setForwardOnly(true);
  q.prepare(full_query);

  for (const auto& bind : bind_values) {
    q.bindValue(bind.first, bind.second);
  }

  if (q.exec() && q.next()) {
    if (q.record().value(0).toInt() > 0) {
      // Whoops, we have the "same" message in database.
      qDebug("Message '%s' was identified as duplicate by filter script.", qPrintable(title()));
      return true;
    }
  }
  else if (q.lastError().isValid()) {
    qWarning("Error when checking for duplicate messages via filtering system, error: '%s'.",
             qPrintable(q.lastError().text()));
  }

  return false;
}

QString MessageObject::title() const {
  return m_message->m_title;
}

void MessageObject::setTitle(const QString& title) {
  m_message->m_title = title;
}

QString MessageObject::url() const {
  return m_message->m_url;
}

void MessageObject::setUrl(const QString& url) {
  m_message->m_url = url;
}

QString MessageObject::author() const {
  return m_message->m_author;
}

void MessageObject::setAuthor(const QString& author) {
  m_message->m_author = author;
}

QString MessageObject::contents() const {
  return m_message->m_contents;
}

void MessageObject::setContents(const QString& contents) {
  m_message->m_contents = contents;
}

QDateTime MessageObject::created() const {
  return m_message->m_created;
}

void MessageObject::setCreated(const QDateTime& created) {
  m_message->m_created = created;
}

bool MessageObject::isRead() const {
  return m_message->m_isRead;
}

void MessageObject::setIsRead(bool is_read) {
  m_message->m_isRead = is_read;
}

bool MessageObject::isImportant() const {
  return m_message->m_isImportant;
}

void MessageObject::setIsImportant(bool is_important) {
  m_message->m_isImportant = is_important;
}

QString MessageObject::feedCustomId() const {
  return m_feedCustomId;
}

int MessageObject::accountId() const {
  return m_accountId;
}
