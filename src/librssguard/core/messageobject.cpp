// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messageobject.h"

#include "3rd-party/boolinq/boolinq.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

MessageObject::MessageObject(QSqlDatabase* db, const QString& feed_custom_id,
                             int account_id, QList<Label*> available_labels,
                             QObject* parent)
  : QObject(parent), m_db(db), m_feedCustomId(feed_custom_id), m_accountId(account_id), m_message(nullptr),
  m_availableLabels(available_labels) {}

void MessageObject::setMessage(Message* message) {
  m_message = message;
}

bool MessageObject::isDuplicateWithAttribute(MessageObject::DuplicationAttributeCheck attribute_check) const {
  // Check database according to duplication attribute_check.
  QSqlQuery q(*m_db);
  QStringList where_clauses;
  QList<QPair<QString, QVariant>> bind_values;

  // Now we construct the query according to parameter.
  if ((attribute_check& DuplicationAttributeCheck::SameTitle) == DuplicationAttributeCheck::SameTitle) {
    where_clauses.append(QSL("title = :title"));
    bind_values.append({ ":title", title() });
  }

  if ((attribute_check& DuplicationAttributeCheck::SameUrl) == DuplicationAttributeCheck::SameUrl) {
    where_clauses.append(QSL("url = :url"));
    bind_values.append({ ":url", url() });
  }

  if ((attribute_check& DuplicationAttributeCheck::SameAuthor) == DuplicationAttributeCheck::SameAuthor) {
    where_clauses.append(QSL("author = :author"));
    bind_values.append({ ":author", author() });
  }

  if ((attribute_check& DuplicationAttributeCheck::SameDateCreated) == DuplicationAttributeCheck::SameDateCreated) {
    where_clauses.append(QSL("date_created = :date_created"));
    bind_values.append({ ":date_created", created().toMSecsSinceEpoch() });
  }

  where_clauses.append(QSL("account_id = :account_id"));
  bind_values.append({ ":account_id", accountId() });

  if ((attribute_check& DuplicationAttributeCheck::AllFeedsSameAccount) != DuplicationAttributeCheck::AllFeedsSameAccount) {
    // Limit to current feed.
    where_clauses.append(QSL("feed = :feed"));
    bind_values.append({ ":feed", feedCustomId() });
  }

  QString full_query = QSL("SELECT COUNT(*) FROM Messages WHERE ") + where_clauses.join(QSL(" AND ")) + QSL(";");

  qDebugNN << LOGSEC_MESSAGEMODEL
           << "Query for MSG duplicate identification is: '"
           << full_query
           << "'.";

  q.setForwardOnly(true);
  q.prepare(full_query);

  for (const auto& bind : bind_values) {
    q.bindValue(bind.first, bind.second);
  }

  if (q.exec() && q.next()) {
    if (q.record().value(0).toInt() > 0) {
      // Whoops, we have the "same" message in database.
      qDebugNN << LOGSEC_MESSAGEMODEL
               << "Message '"
               << title()
               << "' was identified as duplicate by filter script.";
      return true;
    }
  }
  else if (q.lastError().isValid()) {
    qWarningNN << LOGSEC_MESSAGEMODEL
               << "Error when checking for duplicate messages via filtering system, error: '"
               << q.lastError().text()
               << "'.";
  }

  return false;
}

bool MessageObject::assignLabel(QString label_custom_id) const {
  Label* lbl = boolinq::from(m_availableLabels).firstOrDefault([label_custom_id](Label* lbl) {
    return lbl->customId() == label_custom_id;
  });

  if (lbl != nullptr) {
    if (!m_message->m_assignedLabels.contains(lbl)) {
      m_message->m_assignedLabels.append(lbl);
    }

    return true;
  }
  else {
    return false;
  }
}

bool MessageObject::deassignLabel(QString label_custom_id) const {
  Label* lbl = boolinq::from(m_message->m_assignedLabels).firstOrDefault([label_custom_id](Label* lbl) {
    return lbl->customId() == label_custom_id;
  });

  if (lbl != nullptr) {
    m_message->m_assignedLabels.removeAll(lbl);
    return true;
  }
  else {
    return false;
  }
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

QList<Label*> MessageObject::assignedLabels() const {
  return m_message->m_assignedLabels;
}

QList<Label*> MessageObject::availableLabels() const {
  return m_availableLabels;
}
