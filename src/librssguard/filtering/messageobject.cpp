// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/messageobject.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "filtering/filteringsystem.h"
#include "services/abstract/labelsnode.h"

#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

MessageObject::MessageObject(QObject* parent) : QObject(parent), m_message(nullptr) {}

void MessageObject::setMessage(Message* message) {
  m_message = message;
}

bool MessageObject::assignLabel(const QString& label_custom_id) const {
  Label* lbl = boolinq::from(m_system->availableLabels()).firstOrDefault([label_custom_id](Label* lbl) {
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

void MessageObject::setSystem(FilteringSystem* sys) {
  m_system = sys;
}

QString MessageObject::feedCustomId() const {
  if (m_system->feed() == nullptr || m_system->feed()->customId() == QString::number(NO_PARENT_CATEGORY)) {
    return m_message->m_feedId;
  }
  else {
    return m_system->feed()->customId();
  }
}

int MessageObject::accountId() const {
  return m_system->account() != nullptr ? m_system->account()->accountId() : NO_PARENT_CATEGORY;
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

double jaro_winkler_distance(QString str1, QString str2) {
  size_t len1 = str1.size();
  size_t len2 = str2.size();
  if (len1 < len2) {
    std::swap(str1, str2);
    std::swap(len1, len2);
  }
  if (len2 == 0) {
    return len1 == 0 ? 0.0 : 1.0;
  }
  size_t delta = std::max(size_t(1), len1 / 2) - 1;
  std::vector<bool> flag(len2, false);
  std::vector<QChar> ch1_match;
  ch1_match.reserve(len1);
  for (size_t idx1 = 0; idx1 < len1; ++idx1) {
    QChar ch1 = str1[idx1];
    for (size_t idx2 = 0; idx2 < len2; ++idx2) {
      QChar ch2 = str2[idx2];
      if (idx2 <= idx1 + delta && idx2 + delta >= idx1 && ch1 == ch2 && !flag[idx2]) {
        flag[idx2] = true;
        ch1_match.push_back(ch1);
        break;
      }
    }
  }
  size_t matches = ch1_match.size();
  if (matches == 0) {
    return 1.0;
  }
  size_t transpositions = 0;
  for (size_t idx1 = 0, idx2 = 0; idx2 < len2; ++idx2) {
    if (flag[idx2]) {
      if (str2[idx2] != ch1_match[idx1]) {
        ++transpositions;
      }
      ++idx1;
    }
  }
  double m = matches;
  double jaro = (m / len1 + m / len2 + (m - transpositions / 2.0) / m) / 3.0;
  size_t common_prefix = 0;
  len2 = std::min(size_t(4), len2);
  for (size_t i = 0; i < len2; ++i) {
    if (str1[i] == str2[i]) {
      ++common_prefix;
    }
  }
  return 1.0 - (jaro + common_prefix * 0.1 * (1.0 - jaro));
}

bool MessageObject::isAlreadyInDatabaseExact(DuplicityCheck attribute_check) const {
  // Check database according to duplication attribute_check.
  QSqlQuery q(m_system->database());
  QStringList where_clauses;
  QVector<QPair<QString, QVariant>> bind_values;

  // Now we construct the query according to parameter.
  if (Globals::hasFlag(attribute_check, DuplicityCheck::SameTitle)) {
    where_clauses.append(QSL("title = :title"));
    bind_values.append({QSL(":title"), title()});
  }

  if (Globals::hasFlag(attribute_check, DuplicityCheck::SameUrl)) {
    where_clauses.append(QSL("url = :url"));
    bind_values.append({QSL(":url"), url()});
  }

  if (Globals::hasFlag(attribute_check, DuplicityCheck::SameAuthor)) {
    where_clauses.append(QSL("author = :author"));
    bind_values.append({QSL(":author"), author()});
  }

  if (Globals::hasFlag(attribute_check, DuplicityCheck::SameDateCreated)) {
    where_clauses.append(QSL("date_created = :date_created"));
    bind_values.append({QSL(":date_created"), created().toMSecsSinceEpoch()});
  }

  if (Globals::hasFlag(attribute_check, DuplicityCheck::SameCustomId)) {
    where_clauses.append(QSL("custom_id = :custom_id"));
    bind_values.append({QSL(":custom_id"), customId()});
  }

  where_clauses.append(QSL("account_id = :account_id"));
  bind_values.append({QSL(":account_id"), accountId()});

  // If we have already message stored in DB, then we also must
  // make sure that we do not match the message against itself.
  if (m_system->mode() == FilteringSystem::FiteringUseCase::ExistingArticles && id() > 0) {
    where_clauses.append(QSL("id != :id"));
    bind_values.append({QSL(":id"), QString::number(id())});
  }

  if (!Globals::hasFlag(attribute_check, DuplicityCheck::AllFeedsSameAccount)) {
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

    if (q.value(0).toInt() > 0) {
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

bool MessageObject::isAlreadyInDatabase(DuplicityCheck attribute_check, DuplicityMatcher matcher) const {
  switch (matcher) {
    case DuplicityMatcher::Winkler:
      // TODO:
      return false;

    case DuplicityMatcher::Exact:
    default:
      return isAlreadyInDatabaseExact(attribute_check);
  }
}

QList<Label*> MessageObject::assignedLabels() const {
  return m_message->m_assignedLabels;
}

QList<MessageCategory> MessageObject::categories() const {
  return m_message->m_categories;
}

bool MessageObject::hasEnclosures() const {
  return !m_message->m_enclosures.isEmpty();
}
