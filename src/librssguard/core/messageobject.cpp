// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messageobject.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "services/abstract/labelsnode.h"

#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

MessageObject::MessageObject(QSqlDatabase* db, Feed* feed, ServiceRoot* account, bool is_new_message, QObject* parent)
  : QObject(parent), m_db(db), m_feed(feed), m_account(account), m_message(nullptr),
    m_runningAfterFetching(is_new_message) {

  m_feedCustomId = m_feed != nullptr ? m_feed->customId() : QString::number(NO_PARENT_CATEGORY);
  m_accountId = m_account != nullptr ? m_account->accountId() : NO_PARENT_CATEGORY;
  m_availableLabels = m_account != nullptr ? m_account->labelsNode()->labels() : QList<Label*>();
}

void MessageObject::setMessage(Message* message) {
  m_message = message;
}

bool MessageObject::isAlreadyInDatabase(DuplicateCheck attribute_check) const {
  return isDuplicateWithAttribute(attribute_check);
}

bool MessageObject::isDuplicate(DuplicateCheck attribute_check) const {
  return isDuplicateWithAttribute(attribute_check);
}

bool MessageObject::isDuplicateWithAttribute(MessageObject::DuplicateCheck attribute_check) const {
  // Check database according to duplication attribute_check.
  QSqlQuery q(*m_db);
  QStringList where_clauses;
  QVector<QPair<QString, QVariant>> bind_values;

  // Now we construct the query according to parameter.
  if (Globals::hasFlag(attribute_check, DuplicateCheck::SameTitle)) {
    where_clauses.append(QSL("title = :title"));
    bind_values.append({QSL(":title"), title()});
  }

  if (Globals::hasFlag(attribute_check, DuplicateCheck::SameUrl)) {
    where_clauses.append(QSL("url = :url"));
    bind_values.append({QSL(":url"), url()});
  }

  if (Globals::hasFlag(attribute_check, DuplicateCheck::SameAuthor)) {
    where_clauses.append(QSL("author = :author"));
    bind_values.append({QSL(":author"), author()});
  }

  if (Globals::hasFlag(attribute_check, DuplicateCheck::SameDateCreated)) {
    where_clauses.append(QSL("date_created = :date_created"));
    bind_values.append({QSL(":date_created"), created().toMSecsSinceEpoch()});
  }

  if (Globals::hasFlag(attribute_check, DuplicateCheck::SameCustomId)) {
    where_clauses.append(QSL("custom_id = :custom_id"));
    bind_values.append({QSL(":custom_id"), customId()});
  }

  where_clauses.append(QSL("account_id = :account_id"));
  bind_values.append({QSL(":account_id"), accountId()});

  // If we have already message stored in DB, then we also must
  // make sure that we do not match the message against itself.
  if (!runningFilterWhenFetching() && m_message->m_id > 0) {
    where_clauses.append(QSL("id != :id"));
    bind_values.append({QSL(":id"), QString::number(m_message->m_id)});
  }

  if (!Globals::hasFlag(attribute_check, DuplicateCheck::AllFeedsSameAccount)) {
    // Limit to current feed.
    where_clauses.append(QSL("feed = :feed"));
    bind_values.append({QSL(":feed"), feedCustomId()});
  }

  QString full_query = QSL("SELECT COUNT(*) FROM Messages WHERE ") + where_clauses.join(QSL(" AND ")) + QSL(";");

  qDebugNN << LOGSEC_MESSAGEMODEL
           << "Prepared query for MSG duplicate identification is:" << QUOTE_W_SPACE_DOT(full_query);

  q.setForwardOnly(true);
  q.prepare(full_query);

  for (const auto& bind : bind_values) {
    q.bindValue(bind.first, bind.second);
  }

  if (q.exec() && q.next()) {
    qDebugNN << LOGSEC_DB << "Executed SQL for message duplicates check:"
             << QUOTE_W_SPACE_DOT(DatabaseFactory::lastExecutedQuery(q));

    if (q.record().value(0).toInt() > 0) {
      // Whoops, we have the "same" message in database.
      qDebugNN << LOGSEC_CORE << "Message" << QUOTE_W_SPACE(title()) << "was identified as duplicate by filter script.";
      return true;
    }
  }
  else if (q.lastError().isValid()) {
    qWarningNN << LOGSEC_CORE << "Error when checking for duplicate messages via filtering system, error:"
               << QUOTE_W_SPACE_DOT(q.lastError().text());
  }

  return false;
}

bool MessageObject::assignLabel(const QString& label_custom_id) const {
  // NOTE: This is now not needed as the underlying bug was fixed in DB layer.
  /*
  if (m_message->m_id <= 0 && m_message->m_customId.isEmpty()) {
    return false;
  }
  */

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

bool MessageObject::deassignLabel(const QString& label_custom_id) const {
  // NOTE: This is now not needed as the underlying bug was fixed in DB layer.
  /*
  if (m_message->m_id <= 0 && m_message->m_customId.isEmpty()) {
    return false;
  }
  */

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

QString MessageObject::findLabelId(const QString& label_title) const {
  Label* found_lbl = boolinq::from(m_availableLabels).firstOrDefault([label_title](Label* lbl) {
    return lbl->title().toLower() == label_title.toLower();
  });

  if (found_lbl == nullptr) {
    qWarningNN << LOGSEC_CORE << "Label with title" << QUOTE_W_SPACE(label_title) << "not found.";
  }

  return found_lbl != nullptr ? found_lbl->customId() : QString();
}

QString MessageObject::createLabelId(const QString& title, const QString& hex_color) {
  QString lbl_id = findLabelId(title);

  if (!lbl_id.isEmpty()) {
    // Label exists.
    return lbl_id;
  }

  if (!Globals::hasFlag(m_account->supportedLabelOperations(), ServiceRoot::LabelOperation::Adding)) {
    qWarningNN << LOGSEC_CORE << "This account does not support creating labels.";
    return nullptr;
  }

  Label* new_lbl = nullptr;

  try {
    auto rnd_color = TextFactory::generateRandomColor();

    new_lbl = new Label(title, hex_color.isEmpty() ? rnd_color : hex_color);
    QSqlDatabase db = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

    DatabaseQueries::createLabel(db, new_lbl, m_account->accountId());
    m_account->requestItemReassignment(new_lbl, m_account->labelsNode());

    m_availableLabels.append(new_lbl);

    return new_lbl->customId();
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_CORE << "Cannot create label:" << QUOTE_W_SPACE_DOT(ex.message());

    if (new_lbl != nullptr) {
      new_lbl->deleteLater();
    }
  }

  return {};
}

void MessageObject::addEnclosure(const QString& url, const QString& mime_type) const {
  m_message->m_enclosures.append(Enclosure(url, mime_type));
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

QString MessageObject::rawContents() const {
  return m_message->m_rawContents;
}

void MessageObject::setRawContents(const QString& raw_contents) {
  m_message->m_rawContents = raw_contents;
}

QDateTime MessageObject::created() const {
  return m_message->m_created;
}

void MessageObject::setCreated(const QDateTime& created) {
  m_message->m_created = created;
}

bool MessageObject::createdIsMadeup() const {
  return !m_message->m_createdFromFeed;
}

void MessageObject::setCreatedIsMadeup(bool madeup) {
  m_message->m_createdFromFeed = !madeup;
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

bool MessageObject::isDeleted() const {
  return m_message->m_isDeleted;
}

void MessageObject::setIsDeleted(bool is_deleted) {
  m_message->m_isDeleted = is_deleted;
}

double MessageObject::score() const {
  return m_message->m_score;
}

void MessageObject::setScore(double score) {
  m_message->m_score = score;
}

QString MessageObject::feedCustomId() const {
  if (m_feedCustomId.isEmpty() || m_feedCustomId == QString::number(NO_PARENT_CATEGORY)) {
    return m_message->m_feedId;
  }
  else {
    return m_feedCustomId;
  }
}

int MessageObject::accountId() const {
  return m_accountId;
}

QString MessageObject::customId() const {
  return m_message->m_customId;
}

void MessageObject::setCustomId(const QString& custom_id) {
  m_message->m_customId = custom_id;
}

int MessageObject::id() const {
  return m_message->m_id;
}

QList<Label*> MessageObject::assignedLabels() const {
  return m_message->m_assignedLabels;
}

QList<Label*> MessageObject::availableLabels() const {
  return m_availableLabels;
}

QList<MessageCategory> MessageObject::categories() const {
  return m_message->m_categories;
}

bool MessageObject::runningFilterWhenFetching() const {
  return m_runningAfterFetching;
}
