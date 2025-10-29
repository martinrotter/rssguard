// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasequeries.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/globals.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "services/abstract/category.h"

#include <QSqlDriver>
#include <QUrl>
#include <QVariant>

QStringList initMessageTableAttributes() {
  QStringList field_names;

  field_names.append(QSL("Messages.id"));
  field_names.append(QSL("Messages.is_read"));
  field_names.append(QSL("Messages.is_important"));
  field_names.append(QSL("Messages.is_deleted"));
  field_names.append(QSL("Messages.is_pdeleted"));
  field_names.append(QSL("Messages.feed"));
  field_names.append(QSL("Messages.title"));
  field_names.append(QSL("Messages.url"));
  field_names.append(QSL("Messages.author"));
  field_names.append(QSL("Messages.date_created"));
  field_names.append(QSL("Messages.contents"));
  field_names.append(QSL("Messages.enclosures"));
  field_names.append(QSL("Messages.score"));
  field_names.append(QSL("Messages.account_id"));
  field_names.append(QSL("Messages.custom_id"));
  field_names.append(QSL("Messages.custom_hash"));
  field_names.append(QSL("("
                         "SELECT GROUP_CONCAT(Labels.custom_id) "
                         "FROM LabelsInMessages lim "
                         "JOIN Labels ON lim.label = Labels.id "
                         "WHERE lim.message = Messages.id AND lim.account_id = Messages.account_id "
                         ") AS msg_labels"));

  return field_names;
}

QStringList DatabaseQueries::messageTableAttributes() {
  static QStringList field_names = initMessageTableAttributes();

  return field_names;
}

QString DatabaseQueries::serializeCustomData(const QVariantHash& data) {
  if (!data.isEmpty()) {
    return QString::fromUtf8(QJsonDocument::fromVariant(data).toJson(QJsonDocument::JsonFormat::Indented));
  }
  else {
    return QString();
  }
}

QVariantHash DatabaseQueries::deserializeCustomData(const QString& data) {
  if (data.isEmpty()) {
    return QVariantHash();
  }
  else {
    auto json = QJsonDocument::fromJson(data.toUtf8());

    return json.object().toVariantHash();
  }
}

void DatabaseQueries::purgeLabelAssignments(const QSqlDatabase& db, Label* label) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM LabelsInMessages "
                "WHERE "
                "  LabelsInMessages.label = :label AND "
                "  LabelsInMessages.account_id = :account_id;"));
  q.bindValue(QSL(":label"), label->id());
  q.bindValue(QSL(":account_id"), label->account()->accountId());

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::deassignLabelFromMessage(const QSqlDatabase& db, Label* label, const Message& msg) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM LabelsInMessages "
                "WHERE "
                "  LabelsInMessages.message = :message AND "
                "  LabelsInMessages.label = :label AND "
                "  LabelsInMessages.account_id = :account_id;"));
  q.bindValue(QSL(":message"), msg.m_id);
  q.bindValue(QSL(":label"), label->id());
  q.bindValue(QSL(":account_id"), msg.m_accountId);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::assignLabelToMessage(const QSqlDatabase& db, Label* label, const Message& msg) {
  bool is_sqlite = db.driverName() == QSL(APP_DB_SQLITE_DRIVER);
  QSqlQuery q(db);
  QString insert_cmd;

  q.setForwardOnly(true);

  if (is_sqlite) {
    insert_cmd = QSL("INSERT OR IGNORE");
  }
  else {
    insert_cmd = QSL("INSERT IGNORE");
  }

  q.prepare(QSL("%1 INTO LabelsInMessages (message, label, account_id) "
                "VALUES (:message, :label, :account_id);")
              .arg(insert_cmd));

  q.bindValue(QSL(":message"), msg.m_id);
  q.bindValue(QSL(":label"), label->id());
  q.bindValue(QSL(":account_id"), label->account()->accountId());

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::setLabelsForMessage(const QSqlDatabase& db, const QList<Label*>& labels, const Message& msg) {
  QSqlQuery q(db);

  // Remove everything first.
  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM LabelsInMessages "
                "WHERE LabelsInMessages.message = :message AND LabelsInMessages.account_id = :account_id;"));
  q.bindValue(QSL(":message"), msg.m_id);
  q.bindValue(QSL(":account_id"), msg.m_accountId);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }

  // Insert new label associations (if any).
  if (!labels.isEmpty()) {
    QString sql = QSL("INSERT INTO LabelsInMessages (message, label, account_id) VALUES ");
    QStringList values;

    values.reserve(labels.size());

    for (const Label* lbl : labels) {
      values << QStringLiteral("(%1, %2, %3)").arg(msg.m_id).arg(lbl->id()).arg(msg.m_accountId);
    }

    sql += values.join(QSL(", ")) + QL1C(';');

    if (q.exec(sql)) {
      DatabaseFactory::logLastExecutedQuery(q);
    }
    else {
      throw SqlException(q.lastError());
    }
  }
}

QList<Label*> DatabaseQueries::getLabelsForAccount(const QSqlDatabase& db, int account_id) {
  QList<Label*> labels;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT * FROM Labels WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    while (q.next()) {
      Label* lbl = new Label(q.value(QSL("name")).toString(), QColor(q.value(QSL("color")).toString()));

      lbl->setId(q.value(QSL("id")).toInt());
      lbl->setCustomId(q.value(QSL("custom_id")).toString());

      labels << lbl;
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  return labels;
}

void DatabaseQueries::updateLabel(const QSqlDatabase& db, Label* label) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Labels SET name = :name, color = :color "
                "WHERE id = :id AND account_id = :account_id;"));
  q.bindValue(QSL(":name"), label->title());
  q.bindValue(QSL(":color"), label->color().name());
  q.bindValue(QSL(":id"), label->id());
  q.bindValue(QSL(":account_id"), label->account()->accountId());

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::deleteLabel(const QSqlDatabase& db, Label* label) {
  purgeLabelAssignments(db, label);

  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Labels WHERE id = :id AND account_id = :account_id;"));
  q.bindValue(QSL(":id"), label->id());
  q.bindValue(QSL(":account_id"), label->account()->accountId());

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::createLabel(const QSqlDatabase& db, Label* label, int account_id, int new_label_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (new_label_id > 0) {
    q.prepare(QSL("INSERT INTO Labels (id, name, color, custom_id, account_id) "
                  "VALUES (:id, :name, :color, :custom_id, :account_id);"));
    q.bindValue(QSL(":id"), new_label_id);
  }
  else {
    q.prepare(QSL("INSERT INTO Labels (name, color, custom_id, account_id) "
                  "VALUES (:name, :color, :custom_id, :account_id);"));
  }

  q.bindValue(QSL(":name"), label->title());
  q.bindValue(QSL(":color"), label->color().name());
  q.bindValue(QSL(":custom_id"), label->customId());
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.lastInsertId().isValid()) {
    DatabaseFactory::logLastExecutedQuery(q);

    label->setId(q.lastInsertId().toInt());

    // NOTE: This custom ID in this object will be probably
    // overwritten in online-synchronized labels.
    if (label->customId().isEmpty()) {
      label->setCustomId(QString::number(label->id()));
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  // Fixup missing custom IDs.
  q.prepare(QSL("UPDATE Labels SET custom_id = id WHERE custom_id IS NULL OR custom_id = '';"));

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::updateProbe(const QSqlDatabase& db, Search* probe) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Probes SET name = :name, fltr = :fltr, color = :color "
                "WHERE id = :id AND account_id = :account_id;"));
  q.bindValue(QSL(":name"), probe->title());
  q.bindValue(QSL(":fltr"), probe->filter());
  q.bindValue(QSL(":color"), probe->color().name());
  q.bindValue(QSL(":id"), probe->id());
  q.bindValue(QSL(":account_id"), probe->account()->accountId());

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::createProbe(const QSqlDatabase& db, Search* probe, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("INSERT INTO Probes (name, color, fltr, account_id) "
                "VALUES (:name, :color, :fltr, :account_id);"));
  q.bindValue(QSL(":name"), probe->title());
  q.bindValue(QSL(":fltr"), probe->filter());
  q.bindValue(QSL(":color"), probe->color().name());
  q.bindValue(QSL(":account_id"), account_id);

  auto res = q.exec();

  if (res && q.lastInsertId().isValid()) {
    DatabaseFactory::logLastExecutedQuery(q);

    probe->setId(q.lastInsertId().toInt());
    probe->setCustomId(QString::number(probe->id()));
  }
  else {
    throw SqlException(q.lastError());
  }
}

QList<Search*> DatabaseQueries::getProbesForAccount(const QSqlDatabase& db, int account_id) {
  QList<Search*> probes;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT * FROM Probes WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    while (q.next()) {
      Search* prob = new Search(q.value(QSL("name")).toString(),
                                q.value(QSL("fltr")).toString(),
                                QColor(q.value(QSL("color")).toString()));

      prob->setId(q.value(QSL("id")).toInt());
      prob->setCustomId(QString::number(prob->id()));

      probes << prob;
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  return probes;
}

void DatabaseQueries::deleteProbe(const QSqlDatabase& db, Search* probe) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Probes WHERE id = :id AND account_id = :account_id;"));
  q.bindValue(QSL(":id"), probe->id());
  q.bindValue(QSL(":account_id"), probe->account()->accountId());

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

QStringList DatabaseQueries::customIdsOfMessagesByCondition(const QSqlDatabase& db,
                                                            const QString& condition,
                                                            const QVariantMap& bindings) {
  QString sql = QSL("SELECT custom_id FROM Messages WHERE %1;").arg(condition);
  QSqlQuery q(db);

  if (!q.prepare(sql)) {
    throw SqlException(q.lastError());
  }

  for (auto it = bindings.constBegin(); it != bindings.constEnd(); ++it) {
    q.bindValue(it.key(), it.value());
  }

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  QStringList ids;

  while (q.next()) {
    ids << q.value(0).toString();
  }

  return ids;
}

void DatabaseQueries::markMessagesByCondition(const QSqlDatabase& db,
                                              const QString& where_clause,
                                              RootItem::ReadStatus read,
                                              int account_id) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  QString sql = QSL("UPDATE Messages SET is_read = :read "
                    "WHERE is_deleted = 0 AND is_pdeleted = 0");

  if (account_id > 0) {
    sql += QSL(" AND account_id = :account_id");
  }

  if (!where_clause.isEmpty()) {
    sql += QSL(" AND (%1)").arg(where_clause);
  }

  sql += QL1C(';');

  q.prepare(sql);
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);

  if (account_id > 0) {
    q.bindValue(":account_id", account_id);
  }

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::markProbeReadUnread(const QSqlDatabase& db, Search* probe, RootItem::ReadStatus read) {
  markMessagesByCondition(db,
                          QSL("title REGEXP '%1' OR contents REGEXP '%1'")
                            .arg(probe->filter().replace(QSL("'"), QSL("''"))),
                          read,
                          probe->account()->accountId());
}

void DatabaseQueries::markAllLabelledMessagesReadUnread(const QSqlDatabase& db,
                                                        int account_id,
                                                        RootItem::ReadStatus read) {
  markMessagesByCondition(db,
                          QSL("EXISTS ("
                              "  SELECT 1 "
                              "  FROM LabelsInMessages lim "
                              "  WHERE lim.account_id = Messages.account_id AND lim.message = Messages.id)"),
                          read,
                          account_id);
}

void DatabaseQueries::markLabelledMessagesReadUnread(const QSqlDatabase& db, Label* label, RootItem::ReadStatus read) {
  markMessagesByCondition(db,
                          QSL("id IN (SELECT message FROM LabelsInMessages "
                              "WHERE label = %1 AND account_id = %2)")
                            .arg(label->id())
                            .arg(label->account()->accountId()),
                          read,
                          label->account()->accountId());
}

void DatabaseQueries::markImportantMessagesReadUnread(const QSqlDatabase& db,
                                                      int account_id,
                                                      RootItem::ReadStatus read) {
  markMessagesByCondition(db, QSL("is_important = 1"), read, account_id);
}

void DatabaseQueries::markUnreadMessagesRead(const QSqlDatabase& db, int account_id) {
  markMessagesByCondition(db, QSL("is_read = 0"), RootItem::ReadStatus::Read, account_id);
}

void DatabaseQueries::markMessagesReadUnread(const QSqlDatabase& db,
                                             const QStringList& ids,
                                             RootItem::ReadStatus read) {
  markMessagesByCondition(db, QSL("id IN (%1)").arg(ids.join(QSL(", "))), read);
}

void DatabaseQueries::markFeedsReadUnread(const QSqlDatabase& db,
                                          const QStringList& ids,
                                          int account_id,
                                          RootItem::ReadStatus read) {
  markMessagesByCondition(db, QSL("feed IN (%1)").arg(ids.join(QSL(", "))), read, account_id);
}

void DatabaseQueries::markBinReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_read = :read "
                "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;"));
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::markAccountReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_read = :read WHERE is_pdeleted = 0 AND account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::markMessageImportant(const QSqlDatabase& db, int id, RootItem::Importance importance) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_important = :important WHERE id = :id;"));
  q.bindValue(QSL(":id"), id);
  q.bindValue(QSL(":important"), (int)importance);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::switchMessagesImportance(const QSqlDatabase& db, const QStringList& ids) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_important = NOT is_important WHERE id IN (%1);").arg(ids.join(QSL(", "))));

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::permanentlyDeleteMessages(const QSqlDatabase& db, const QStringList& ids) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE id IN (%1);").arg(ids.join(QSL(", "))));

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::deleteOrRestoreMessagesToFromBin(const QSqlDatabase& db, const QStringList& ids, bool deleted) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_deleted = %2, is_pdeleted = %3 WHERE id IN (%1);")
              .arg(ids.join(QSL(", ")), QString::number(deleted ? 1 : 0), QString::number(0)));

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::restoreBin(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_deleted = 0 "
                "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

bool DatabaseQueries::removeUnwantedArticlesFromFeed(const QSqlDatabase& db,
                                                     const Feed* feed,
                                                     const Feed::ArticleIgnoreLimit& feed_setup,
                                                     const Feed::ArticleIgnoreLimit& app_setup) {
  // Feed setup has higher preference.
  int amount_to_keep =
    feed_setup.m_customizeLimitting ? feed_setup.m_keepCountOfArticles : app_setup.m_keepCountOfArticles;
  bool dont_remove_unread =
    feed_setup.m_customizeLimitting ? feed_setup.m_doNotRemoveUnread : app_setup.m_doNotRemoveUnread;
  bool dont_remove_starred =
    feed_setup.m_customizeLimitting ? feed_setup.m_doNotRemoveStarred : app_setup.m_doNotRemoveStarred;
  bool recycle_dont_purge =
    feed_setup.m_customizeLimitting ? feed_setup.m_moveToBinDontPurge : app_setup.m_moveToBinDontPurge;

  if (amount_to_keep <= 0) {
    // No articles will be removed, quitting.
    return false;
  }

  // We find datetime stamp of oldest article which will be NOT moved/removed.
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT Messages.date_created "
                "FROM Messages "
                "WHERE "
                "  Messages.account_id = :account_id AND "
                "  Messages.feed = :feed AND "
                "  Messages.is_deleted = 0 AND "
                "  Messages.is_pdeleted = 0 "
                "ORDER BY Messages.date_created DESC "
                "LIMIT 1 OFFSET :offset;"));

  q.bindValue(QSL(":offset"), amount_to_keep - 1);
  q.bindValue(QSL(":feed"), feed->id());
  q.bindValue(QSL(":account_id"), feed->account()->accountId());

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  q.next();

  qlonglong last_kept_stamp = q.value(0).toLongLong();

  if (recycle_dont_purge) {
    // We mark all older articles as deleted.
    q.prepare(QSL("UPDATE Messages "
                  "SET is_deleted = 1 "
                  "WHERE "
                  "  Messages.account_id = :account_id AND "
                  "  Messages.feed = :feed AND "
                  "  Messages.is_deleted = 0 AND "
                  "  Messages.is_pdeleted = 0 AND "
                  "  Messages.is_important != :is_important AND "
                  "  Messages.is_read != :is_read AND "
                  "  Messages.date_created < :stamp"));
  }
  else {
    // We purge all older articles.
    q.prepare(QSL("DELETE FROM Messages "
                  "WHERE "
                  "  Messages.account_id = :account_id AND "
                  "  Messages.feed = :feed AND "
                  "  (Messages.is_deleted = 1 OR Messages.is_important != :is_important) AND "
                  "  (Messages.is_deleted = 1 OR Messages.is_read != :is_read) AND "
                  "  Messages.date_created < :stamp"));
  }

  q.bindValue(QSL(":is_important"), dont_remove_starred ? 1 : 2);
  q.bindValue(QSL(":is_read"), dont_remove_unread ? 0 : 2);
  q.bindValue(QSL(":feed"), feed->id());
  q.bindValue(QSL(":stamp"), last_kept_stamp);
  q.bindValue(QSL(":account_id"), feed->account()->accountId());

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  int rows_deleted = q.numRowsAffected();

  qDebugNN << LOGSEC_DB << "Feed cleanup has recycled/purged" << NONQUOTE_W_SPACE(rows_deleted)
           << "old articles from feed" << QUOTE_W_SPACE_DOT(feed->customId());

  return rows_deleted > 0;
}

void DatabaseQueries::purgeFeedArticles(const QSqlDatabase& database, const QList<Feed*>& feeds) {
  QSqlQuery q(database);

  auto feed_clauses = boolinq::from(feeds)
                        .select([](Feed* feed) {
                          return QSL("("
                                     "Messages.feed = %1 AND "
                                     "Messages.account_id = %2 AND "
                                     "Messages.is_important = 0"
                                     ")")
                            .arg(QString::number(feed->id()), QString::number(feed->account()->accountId()));
                        })
                        .toStdList();

  qDebugNN << feed_clauses;

  QStringList feed_str_clauses = FROM_STD_LIST(QStringList, feed_clauses);
  QString feed_clause = feed_str_clauses.join(QSL(" OR "));

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Messages WHERE %1;").arg(feed_clause));

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::purgeMessagesByCondition(const QSqlDatabase& db, const QString& where_clause) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  QString sql = QSL("DELETE FROM Messages WHERE %1;").arg(where_clause);
  q.prepare(sql);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::purgeMessage(const QSqlDatabase& db, int message_id) {
  purgeMessagesByCondition(db, QSL("id = %1").arg(message_id));
}

void DatabaseQueries::purgeImportantMessages(const QSqlDatabase& db) {
  purgeMessagesByCondition(db, QSL("is_important = 1 AND is_deleted = 0"));
}

void DatabaseQueries::purgeReadMessages(const QSqlDatabase& db) {
  purgeMessagesByCondition(db, QSL("is_important = 0 AND is_deleted = 0 AND is_read = 1"));
}

void DatabaseQueries::purgeOldMessages(const QSqlDatabase& db, int older_than_days) {
  const qint64 since_epoch = older_than_days == 0
                               ? QDateTime::currentDateTimeUtc().addYears(10).toMSecsSinceEpoch()
                               : QDateTime::currentDateTimeUtc().addDays(-older_than_days).toMSecsSinceEpoch();

  purgeMessagesByCondition(db, QSL("is_important = 0 AND date_created < %1").arg(since_epoch));
}

void DatabaseQueries::purgeRecycleBin(const QSqlDatabase& db) {
  purgeMessagesByCondition(db, QSL("is_important = 0 AND is_deleted = 1"));
}

void DatabaseQueries::purgeLeftoverMessages(const QSqlDatabase& db, int account_id) {
  purgeMessagesByCondition(db,
                           QSL("account_id = %1 AND "
                               "feed NOT IN (SELECT id FROM Feeds WHERE account_id = %1)")
                             .arg(account_id));
}

QMap<int, ArticleCounts> DatabaseQueries::getMessageCountsForCategory(const QSqlDatabase& db,
                                                                      const QString& custom_id,
                                                                      int account_id,
                                                                      bool include_total_counts) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (include_total_counts) {
    q.prepare(QSL("SELECT feed, SUM((is_read + 1) % 2), COUNT(*) FROM Messages "
                  "WHERE feed IN (SELECT id FROM Feeds WHERE category = :category AND account_id = :account_id) "
                  "AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
                  "GROUP BY feed;"));
  }
  else {
    q.prepare(QSL("SELECT feed, SUM((is_read + 1) % 2) FROM Messages "
                  "WHERE feed IN (SELECT id FROM Feeds WHERE category = :category AND account_id = :account_id) "
                  "AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
                  "GROUP BY feed;"));
  }

  q.bindValue(QSL(":category"), custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    QMap<int, ArticleCounts> counts;

    while (q.next()) {
      int feed_id = q.value(0).toInt();
      ArticleCounts ac;

      ac.m_unread = q.value(1).toInt();

      if (include_total_counts) {
        ac.m_total = q.value(2).toInt();
      }

      counts.insert(feed_id, ac);
    }

    return counts;
  }
  else {
    throw SqlException(q.lastError());
  }
}

QMap<int, ArticleCounts> DatabaseQueries::getMessageCountsForAccount(const QSqlDatabase& db,
                                                                     int account_id,
                                                                     bool include_total_counts) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (include_total_counts) {
    q.prepare(QSL("SELECT feed, SUM((is_read + 1) % 2), COUNT(*) FROM Messages "
                  "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
                  "GROUP BY feed;"));
  }
  else {
    q.prepare(QSL("SELECT feed, SUM((is_read + 1) % 2) FROM Messages "
                  "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
                  "GROUP BY feed;"));
  }

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    QMap<int, ArticleCounts> counts;

    while (q.next()) {
      int feed_id = q.value(0).toInt();
      ArticleCounts ac;

      ac.m_unread = q.value(1).toInt();

      if (include_total_counts) {
        ac.m_total = q.value(2).toInt();
      }

      counts.insert(feed_id, ac);
    }

    return counts;
  }
  else {
    throw SqlException(q.lastError());
  }
}

ArticleCounts DatabaseQueries::getMessageCountsForFeed(const QSqlDatabase& db, int feed_id, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT COUNT(*), SUM(is_read) FROM Messages "
                "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;"));

  q.bindValue(QSL(":feed"), feed_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);

    ArticleCounts ac;

    ac.m_total = q.value(0).toInt();
    ac.m_unread = ac.m_total - q.value(1).toInt();

    return ac;
  }
  else {
    throw SqlException(q.lastError());
  }
}

ArticleCounts DatabaseQueries::getMessageCountsForLabel(const QSqlDatabase& db, Label* label, int account_id) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  // Count total and read messages using EXISTS for label.
  q.prepare(QSL("SELECT COUNT(*), SUM(is_read) "
                "FROM Messages m "
                "WHERE m.is_deleted = 0 "
                "  AND m.is_pdeleted = 0 "
                "  AND m.account_id = :account_id "
                "  AND EXISTS ("
                "    SELECT 1 "
                "    FROM LabelsInMessages lim "
                "    WHERE"
                "      lim.label = :label_id AND "
                "      lim.account_id = m.account_id AND "
                "      lim.message = m.id);"));

  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":label_id"), label->id());

  if (q.exec() && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);

    ArticleCounts ac;
    ac.m_total = q.value(0).toInt();
    ac.m_unread = ac.m_total - q.value(1).toInt();

    return ac;
  }
  else {
    throw SqlException(q.lastError());
  }
}

QMap<int, ArticleCounts> DatabaseQueries::getMessageCountsForAllLabels(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT lim.label, COUNT(*), SUM(CASE WHEN m.is_read = 0 THEN 1 ELSE 0 END) "
                "FROM LabelsInMessages lim "
                "JOIN Messages m "
                "  ON m.id = lim.message AND m.account_id = lim.account_id "
                "WHERE"
                "  m.is_deleted = 0 AND m.is_pdeleted = 0 AND m.account_id = :account_id "
                "GROUP BY lim.label;"));

  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  QMap<int, ArticleCounts> results;

  while (q.next()) {
    int label_id = q.value(0).toInt();

    ArticleCounts ac;
    ac.m_total = q.value(1).toInt();
    ac.m_unread = q.value(2).toInt();

    results.insert(label_id, ac);
  }

  return results;
}

ArticleCounts DatabaseQueries::getImportantMessageCounts(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT COUNT(*), SUM(is_read) FROM Messages "
                "WHERE is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = "
                ":account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);

    ArticleCounts ac;

    ac.m_total = q.value(0).toInt();
    ac.m_unread = ac.m_total - q.value(1).toInt();

    return ac;
  }
  else {
    throw SqlException(q.lastError());
  }
}

int DatabaseQueries::getUnreadMessageCounts(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT COUNT(*) FROM Messages "
                "WHERE is_read = 0 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;"));

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);

    return q.value(0).toInt();
  }
  else {
    throw SqlException(q.lastError());
  }
}

ArticleCounts DatabaseQueries::getMessageCountsForBin(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT COUNT(*), SUM(is_read) FROM Messages "
                "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;"));

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);

    ArticleCounts ac;

    ac.m_total = q.value(0).toInt();
    ac.m_unread = ac.m_total - q.value(1).toInt();

    return ac;
  }
  else {
    throw SqlException(q.lastError());
  }
}

QList<Message> DatabaseQueries::getUndeletedMessagesForFeed(const QSqlDatabase& db,
                                                            int feed_id,
                                                            const QHash<QString, Label*>& labels,
                                                            int account_id) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT %1 "
                "FROM Messages "
                "WHERE is_deleted = 0 AND is_pdeleted = 0 AND "
                "      feed = :feed AND account_id = :account_id;")
              .arg(messageTableAttributes().join(QSL(", "))));
  q.bindValue(QSL(":feed"), feed_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    while (q.next()) {
      Message message = Message::fromSqlQuery(q, labels);

      messages.append(message);
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  return messages;
}

QList<Message> DatabaseQueries::getUndeletedMessagesForAccount(const QSqlDatabase& db,
                                                               const QHash<QString, Label*>& labels,
                                                               int account_id) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT %1 "
                "FROM Messages "
                "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;")
              .arg(messageTableAttributes().join(QSL(", "))));
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    while (q.next()) {
      Message message = Message::fromSqlQuery(q, labels);

      messages.append(message);
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  return messages;
}

int DatabaseQueries::highestPrimaryIdFeeds(const QSqlDatabase& db) {
  QSqlQuery q(db);

  if (q.exec(QSL("SELECT MAX(id) FROM Feeds;")) && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);
    return q.value(0).isNull() ? 0 : q.value(0).toInt();
  }
  else {
    throw SqlException(q.lastError());
  }
}

int DatabaseQueries::highestPrimaryIdLabels(const QSqlDatabase& db) {
  QSqlQuery q(db);

  if (q.exec(QSL("SELECT MAX(id) FROM Labels;")) && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);
    return q.value(0).isNull() ? 0 : q.value(0).toInt();
  }
  else {
    throw SqlException(q.lastError());
  }
}

QStringList DatabaseQueries::bagOfMessages(const QSqlDatabase& db, ServiceRoot::BagOfMessages bag, const Feed* feed) {
  QStringList ids;
  QSqlQuery q(db);
  QString query;

  q.setForwardOnly(true);

  switch (bag) {
    case ServiceRoot::BagOfMessages::Unread:
      query = QSL("is_read = 0");
      break;

    case ServiceRoot::BagOfMessages::Starred:
      query = QSL("is_important = 1");
      break;

    case ServiceRoot::BagOfMessages::Read:
    default:
      query = QSL("is_read = 1");
      break;
  }

  q.prepare(QSL("SELECT custom_id "
                "FROM Messages "
                "WHERE %1 AND feed = :feed AND account_id = :account_id;")
              .arg(query));

  q.bindValue(QSL(":account_id"), feed->account()->accountId());
  q.bindValue(QSL(":feed"), feed->id());

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  while (q.next()) {
    ids.append(q.value(0).toString());
  }

  return ids;
}

QHash<QString, QStringList> DatabaseQueries::bagsOfMessages(const QSqlDatabase& db, const QList<Label*>& labels) {
  QHash<QString, QStringList> ids;

  // TODO: TEST
  for (Label* lbl : labels) {
    QStringList ids_one_label = customIdsOfMessagesFromLabel(db, lbl, RootItem::ReadStatus::Unknown);

    ids.insert(lbl->customId(), ids_one_label);
  }

  return ids;
}

UpdatedArticles DatabaseQueries::updateMessages(QSqlDatabase& db,
                                                QList<Message>& messages,
                                                Feed* feed,
                                                bool force_update,
                                                bool force_insert,
                                                QMutex* db_mutex) {
  if (messages.isEmpty()) {
    return {};
  }

  UpdatedArticles updated_messages;
  int account_id = feed->account()->accountId();
  auto feed_id = feed->id();
  QVector<Message*> msgs_to_insert;

  if (!force_insert) {
    // Prepare queries.
    QSqlQuery query_select_with_url(db);
    QSqlQuery query_select_with_custom_id(db);
    QSqlQuery query_select_with_custom_id_for_feed(db);
    QSqlQuery query_select_with_id(db);
    QSqlQuery query_update(db);

    // Here we have query which will check for existence of the "same" message in given feed.
    // The two message are the "same" if:
    //   1) they belong to the SAME FEED AND,
    //   2) they have same URL AND,
    //   3) they have same AUTHOR AND,
    //   4) they have same TITLE.
    // NOTE: This only applies to messages from standard RSS/ATOM/JSON feeds without ID/GUID.
    query_select_with_url.setForwardOnly(true);
    query_select_with_url.prepare(QSL("SELECT id, date_created, is_read, is_important, contents, feed FROM Messages "
                                      "WHERE feed = :feed AND title = :title AND url = :url AND author = :author AND "
                                      "account_id = :account_id;"));

    // When we have custom ID of the message which is service-specific (synchronized services).
    query_select_with_custom_id.setForwardOnly(true);
    query_select_with_custom_id
      .prepare(QSL("SELECT id, date_created, is_read, is_important, contents, feed, title, author FROM Messages "
                   "WHERE custom_id = :custom_id AND account_id = :account_id;"));

    // We have custom ID of message, but it is feed-specific not service-specific (standard RSS/ATOM/JSON).
    query_select_with_custom_id_for_feed.setForwardOnly(true);
    query_select_with_custom_id_for_feed
      .prepare(QSL("SELECT id, date_created, is_read, is_important, contents, title, author FROM Messages "
                   "WHERE feed = :feed AND custom_id = :custom_id AND account_id = :account_id;"));

    // In some case, messages are already stored in the DB and they all have primary DB ID.
    // This is particularly the case when user runs some message filter manually on existing messages
    // of some feed.
    query_select_with_id.setForwardOnly(true);
    query_select_with_id
      .prepare(QSL("SELECT date_created, is_read, is_important, contents, feed, title, author FROM Messages "
                   "WHERE id = :id AND account_id = :account_id;"));

    // Used to update existing messages.
    query_update.setForwardOnly(true);
    query_update.prepare(QSL("UPDATE Messages "
                             "SET title = :title, is_read = :is_read, is_important = :is_important, is_deleted = "
                             ":is_deleted, url = :url, author = :author, score = :score, date_created = :date_created, "
                             "contents = :contents, enclosures = :enclosures, feed = :feed "
                             "WHERE id = :id;"));

    for (Message& message : messages) {
      int id_existing_message = -1;
      qint64 date_existing_message = 0;
      bool is_read_existing_message = false;
      bool is_important_existing_message = false;
      QString contents_existing_message;
      int feed_id_existing_message;
      QString title_existing_message;
      QString author_existing_message;

      QMutexLocker lck(db_mutex);

      if (message.m_id > 0) {
        // We recognize directly existing message.
        // NOTE: Particularly for manual message filter execution.
        query_select_with_id.bindValue(QSL(":id"), message.m_id);
        query_select_with_id.bindValue(QSL(":account_id"), account_id);

        qDebugNN << LOGSEC_DB << "Checking if message with primary ID" << QUOTE_W_SPACE(message.m_id)
                 << "is present in DB.";

        if (query_select_with_id.exec() && query_select_with_id.next()) {
          DatabaseFactory::logLastExecutedQuery(query_select_with_id);

          id_existing_message = message.m_id;
          date_existing_message = query_select_with_id.value(0).value<qint64>();
          is_read_existing_message = query_select_with_id.value(1).toBool();
          is_important_existing_message = query_select_with_id.value(2).toBool();
          contents_existing_message = query_select_with_id.value(3).toString();
          feed_id_existing_message = query_select_with_id.value(4).toInt();
          title_existing_message = query_select_with_id.value(5).toString();
          author_existing_message = query_select_with_id.value(6).toString();

          qDebugNN << LOGSEC_DB << "Message with direct DB ID is already present in DB and has DB ID"
                   << QUOTE_W_SPACE_DOT(id_existing_message);
        }
        else if (query_select_with_id.lastError().isValid()) {
          qWarningNN << LOGSEC_DB << "Failed to check for existing message in DB via primary ID:"
                     << QUOTE_W_SPACE_DOT(query_select_with_id.lastError().text());
        }

        query_select_with_id.finish();
      }
      else if (message.m_customId.isEmpty()) {
        // We need to recognize existing messages according to URL & AUTHOR & TITLE.
        // NOTE: This concerns articles from RSS/ATOM/JSON which do not
        // provide unique ID/GUID.
        query_select_with_url.bindValue(QSL(":feed"), feed_id);
        query_select_with_url.bindValue(QSL(":title"), unnulifyString(message.m_title));
        query_select_with_url.bindValue(QSL(":url"), unnulifyString(message.m_url));
        query_select_with_url.bindValue(QSL(":author"), unnulifyString(message.m_author));
        query_select_with_url.bindValue(QSL(":account_id"), account_id);

        qDebugNN << LOGSEC_DB << "Checking if message with title " << QUOTE_NO_SPACE(message.m_title) << ", url "
                 << QUOTE_NO_SPACE(message.m_url) << "' and author " << QUOTE_NO_SPACE(message.m_author)
                 << " is present in DB.";

        if (query_select_with_url.exec() && query_select_with_url.next()) {
          DatabaseFactory::logLastExecutedQuery(query_select_with_url);

          id_existing_message = query_select_with_url.value(0).toInt();
          date_existing_message = query_select_with_url.value(1).value<qint64>();
          is_read_existing_message = query_select_with_url.value(2).toBool();
          is_important_existing_message = query_select_with_url.value(3).toBool();
          contents_existing_message = query_select_with_url.value(4).toString();
          feed_id_existing_message = query_select_with_url.value(5).toInt();
          title_existing_message = unnulifyString(message.m_title);
          author_existing_message = unnulifyString(message.m_author);

          qDebugNN << LOGSEC_DB << "Message with these attributes is already present in DB and has DB ID"
                   << QUOTE_W_SPACE_DOT(id_existing_message);
        }
        else if (query_select_with_url.lastError().isValid()) {
          qWarningNN << LOGSEC_DB << "Failed to check for existing message in DB via URL/TITLE/AUTHOR:"
                     << QUOTE_W_SPACE_DOT(query_select_with_url.lastError().text());
        }

        query_select_with_url.finish();
      }
      else {
        // We can recognize existing messages via their custom ID.
        if (feed->account()->isSyncable()) {
          // Custom IDs are service-wide.
          // NOTE: This concerns messages from custom accounts, like TT-RSS or Nextcloud News.
          query_select_with_custom_id.bindValue(QSL(":account_id"), account_id);
          query_select_with_custom_id.bindValue(QSL(":custom_id"), unnulifyString(message.m_customId));

          qDebugNN << LOGSEC_DB << "Checking if message with service-specific custom ID"
                   << QUOTE_W_SPACE(message.m_customId) << "is present in DB.";

          if (query_select_with_custom_id.exec() && query_select_with_custom_id.next()) {
            DatabaseFactory::logLastExecutedQuery(query_select_with_custom_id);

            id_existing_message = query_select_with_custom_id.value(0).toInt();
            date_existing_message = query_select_with_custom_id.value(1).value<qint64>();
            is_read_existing_message = query_select_with_custom_id.value(2).toBool();
            is_important_existing_message = query_select_with_custom_id.value(3).toBool();
            contents_existing_message = query_select_with_custom_id.value(4).toString();
            feed_id_existing_message = query_select_with_custom_id.value(5).toInt();
            title_existing_message = query_select_with_custom_id.value(6).toString();
            author_existing_message = query_select_with_custom_id.value(7).toString();

            qDebugNN << LOGSEC_DB << "Message with custom ID" << QUOTE_W_SPACE(message.m_customId)
                     << "is already present in DB and has DB ID '" << id_existing_message << "'.";
          }
          else if (query_select_with_custom_id.lastError().isValid()) {
            qWarningNN << LOGSEC_DB << "Failed to check for existing message in DB via ID:"
                       << QUOTE_W_SPACE_DOT(query_select_with_custom_id.lastError().text());
          }

          query_select_with_custom_id.finish();
        }
        else {
          // Custom IDs are feed-specific.
          // NOTE: This concerns articles with ID/GUID from standard RSS/ATOM/JSON feeds.
          query_select_with_custom_id_for_feed.bindValue(QSL(":account_id"), account_id);
          query_select_with_custom_id_for_feed.bindValue(QSL(":feed"), feed_id);
          query_select_with_custom_id_for_feed.bindValue(QSL(":custom_id"), unnulifyString(message.m_customId));

          qDebugNN << LOGSEC_DB << "Checking if message with feed-specific custom ID"
                   << QUOTE_W_SPACE(message.m_customId) << "is present in DB.";

          if (query_select_with_custom_id_for_feed.exec() && query_select_with_custom_id_for_feed.next()) {
            DatabaseFactory::logLastExecutedQuery(query_select_with_custom_id_for_feed);

            id_existing_message = query_select_with_custom_id_for_feed.value(0).toInt();
            date_existing_message = query_select_with_custom_id_for_feed.value(1).value<qint64>();
            is_read_existing_message = query_select_with_custom_id_for_feed.value(2).toBool();
            is_important_existing_message = query_select_with_custom_id_for_feed.value(3).toBool();
            contents_existing_message = query_select_with_custom_id_for_feed.value(4).toString();
            feed_id_existing_message = feed_id;
            title_existing_message = query_select_with_custom_id_for_feed.value(5).toString();
            author_existing_message = query_select_with_custom_id_for_feed.value(6).toString();

            qDebugNN << LOGSEC_DB << "Message with custom ID" << QUOTE_W_SPACE(message.m_customId)
                     << "is already present in DB and has DB ID" << QUOTE_W_SPACE_DOT(id_existing_message);
          }
          else if (query_select_with_custom_id_for_feed.lastError().isValid()) {
            qWarningNN << LOGSEC_DB << "Failed to check for existing message in DB via ID:"
                       << QUOTE_W_SPACE_DOT(query_select_with_custom_id_for_feed.lastError().text());
          }

          query_select_with_custom_id_for_feed.finish();
        }
      }

      // Now, check if this message is already in the DB.
      if (id_existing_message >= 0) {
        message.m_id = id_existing_message;

        // Message is already in the DB.
        //
        // Now, we update it if at least one of next conditions is true:
        //   1) FOR SYNCHRONIZED SERVICES:
        //        Message has custom ID AND (its date OR read status OR starred status are changed
        //        or message was moved from other feed to current feed - this can particularly happen in Gmail feeds).
        //
        //   2) FOR NON-SYNCHRONIZED SERVICES (RSS/ATOM/JSON):
        //        Message has custom ID/GUID and its title or author or contents are changed.
        //
        //   3) FOR ALL SERVICES:
        //        Message has its date fetched from feed AND its date is different
        //        from date in DB or content is changed. Date/time is considered different
        //        when the difference is larger than MSG_DATETIME_DIFF_THRESSHOLD
        //
        //   4) FOR ALL SERVICES:
        //        Message update is forced, we want to overwrite message as some arbitrary atribute was changed,
        //        this particularly happens when manual message filter execution happens.
        bool ignore_contents_changes =
          qApp->settings()->value(GROUP(Messages), SETTING(Messages::IgnoreContentsChanges)).toBool();
        bool cond_1 =
          !message.m_customId.isEmpty() && feed->account()->isSyncable() &&
          (message.m_created.toMSecsSinceEpoch() != date_existing_message ||
           message.m_isRead != is_read_existing_message || message.m_isImportant != is_important_existing_message ||
           (message.m_feedId != feed_id_existing_message && message.m_feedId == feed_id) ||
           message.m_title != title_existing_message ||
           (!ignore_contents_changes && message.m_contents != contents_existing_message));
        bool cond_2 = !message.m_customId.isEmpty() && !feed->account()->isSyncable() &&
                      (message.m_title != title_existing_message || message.m_author != author_existing_message ||
                       (!ignore_contents_changes && message.m_contents != contents_existing_message));
        bool cond_3 = (message.m_createdFromFeed && std::abs(message.m_created.toMSecsSinceEpoch() -
                                                             date_existing_message) > MSG_DATETIME_DIFF_THRESSHOLD) ||
                      (!ignore_contents_changes && message.m_contents != contents_existing_message);

        if (cond_1 || cond_2 || cond_3 || force_update) {
          if (!feed->account()->isSyncable() &&
              !qApp->settings()->value(GROUP(Messages), SETTING(Messages::MarkUnreadOnUpdated)).toBool()) {
            // Feed is not syncable, thus we got RSS/JSON/whatever.
            // Article is only updated, so we now prefer to keep original read state
            // pretty much the same way starred state is kept.
            message.m_isRead = is_read_existing_message;
          }

          // Message exists and is changed, update it.
          query_update.bindValue(QSL(":title"), unnulifyString(message.m_title));
          query_update.bindValue(QSL(":is_read"), int(message.m_isRead));
          query_update.bindValue(QSL(":is_important"),
                                 (feed->account()->isSyncable() || message.m_isImportant)
                                   ? int(message.m_isImportant)
                                   : is_important_existing_message);
          query_update.bindValue(QSL(":is_deleted"), int(message.m_isDeleted));
          query_update.bindValue(QSL(":url"), unnulifyString(message.m_url));
          query_update.bindValue(QSL(":author"), unnulifyString(message.m_author));
          query_update.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
          query_update.bindValue(QSL(":contents"), unnulifyString(message.m_contents));
          query_update.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
          query_update.bindValue(QSL(":feed"), message.m_feedId);
          query_update.bindValue(QSL(":score"), message.m_score);
          query_update.bindValue(QSL(":id"), id_existing_message);

          if (query_update.exec()) {
            DatabaseFactory::logLastExecutedQuery(query_update);

            qDebugNN << LOGSEC_DB << "Overwriting message with title" << QUOTE_W_SPACE(message.m_title) << "URL"
                     << QUOTE_W_SPACE(message.m_url) << "in DB.";

            if (!message.m_isRead) {
              updated_messages.m_unread.append(message);
            }

            updated_messages.m_all.append(message);
            message.m_insertedUpdated = true;
          }
          else if (query_update.lastError().isValid()) {
            qCriticalNN << LOGSEC_DB
                        << "Failed to update message in DB:" << QUOTE_W_SPACE_DOT(query_update.lastError().text());
          }

          query_update.finish();
        }
      }
      else {
        msgs_to_insert.append(&message);
      }
    }
  }
  else {
    // We insert everything no matter what.
    for (Message& message : messages) {
      msgs_to_insert.append(&message);
    }
  }

  if (!msgs_to_insert.isEmpty()) {
    QMutexLocker lck(db_mutex);

    if (!db.transaction()) {
      qFatal("transaction failed");
    }

    QString bulk_insert = QSL("INSERT INTO Messages "
                              "(feed, title, is_read, is_important, is_deleted, url, author, score, date_created, "
                              "contents, enclosures, custom_id, custom_hash, account_id) "
                              "VALUES %1;");

    for (int i = 0; i < msgs_to_insert.size(); i += 1000) {
      QStringList vals;
      int batch_length = std::min(1000, int(msgs_to_insert.size()) - i);

      for (int l = i; l < (i + batch_length); l++) {
        Message* msg = msgs_to_insert[l];

        if (msg->m_title.isEmpty()) {
          qCriticalNN << LOGSEC_DB << "Message" << QUOTE_W_SPACE(msg->m_customId)
                      << "will not be inserted to DB because it does not meet DB constraints.";

          continue;
        }

        vals.append(QSL("\n(:feed, ':title', :is_read, :is_important, :is_deleted, "
                        "':url', ':author', :score, :date_created, ':contents', ':enclosures', "
                        "':custom_id', ':custom_hash', :account_id)")
                      .replace(QSL(":feed"), QString::number(feed_id))
                      .replace(QSL(":title"), DatabaseFactory::escapeQuery(unnulifyString(msg->m_title)))
                      .replace(QSL(":is_read"), QString::number(int(msg->m_isRead)))
                      .replace(QSL(":is_important"), QString::number(int(msg->m_isImportant)))
                      .replace(QSL(":is_deleted"), QString::number(int(msg->m_isDeleted)))
                      .replace(QSL(":url"), DatabaseFactory::escapeQuery(unnulifyString(msg->m_url)))
                      .replace(QSL(":author"), DatabaseFactory::escapeQuery(unnulifyString(msg->m_author)))
                      .replace(QSL(":date_created"), QString::number(msg->m_created.toMSecsSinceEpoch()))
                      .replace(QSL(":contents"), DatabaseFactory::escapeQuery(unnulifyString(msg->m_contents)))
                      .replace(QSL(":enclosures"),
                               DatabaseFactory::escapeQuery(Enclosures::encodeEnclosuresToString(msg->m_enclosures)))
                      .replace(QSL(":custom_id"), DatabaseFactory::escapeQuery(unnulifyString(msg->m_customId)))
                      .replace(QSL(":custom_hash"), unnulifyString(msg->m_customHash))
                      .replace(QSL(":score"), QString::number(msg->m_score))
                      .replace(QSL(":account_id"), QString::number(account_id)));
      }

      if (!vals.isEmpty()) {
        QString final_bulk = bulk_insert.arg(vals.join(QSL(", ")));
        auto bulk_query = QSqlQuery(final_bulk, db);
        auto bulk_error = bulk_query.lastError();

        if (bulk_error.isValid()) {
          QString txt = bulk_error.text() + bulk_error.databaseText() + bulk_error.driverText();

          // IOFactory::writeFile(QSL("%1.sql").arg(QDateTime::currentDateTime().offsetFromUtc()), final_bulk.toUtf8());

          qCriticalNN << LOGSEC_DB << "Failed bulk insert of articles:" << QUOTE_W_SPACE_DOT(txt);
        }
        else {
          // OK, we bulk-inserted many messages but the thing is that they do not
          // have their DB IDs fetched in objects, therefore labels cannot be assigned etc.
          //
          // We can calculate real IDs because of how "auto-increment" algorithms work.
          //   https://www.sqlite.org/autoinc.html
          //   https://mariadb.com/kb/en/auto_increment
          int last_msg_id = bulk_query.lastInsertId().toInt();

          for (int l = i, c = 1; l < (i + batch_length); l++, c++) {
            Message* msg = msgs_to_insert[l];

            if (msg->m_title.isEmpty()) {
              // This article was not for sure inserted. Tweak
              // next ID calculation.
              c--;
              continue;
            }

            msg->m_insertedUpdated = true;
            msg->m_id = last_msg_id - batch_length + c;

            if (!msg->m_isRead) {
              updated_messages.m_unread.append(*msg);
            }

            updated_messages.m_all.append(*msg);
          }
        }
      }
    }

    if (!db.commit()) {
      qFatal("transaction failed");
    }
  }

  const bool uses_online_labels =
    Globals::hasFlag(feed->account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Synchronised);

  for (Message& message : messages) {
    if (!message.m_customId.isEmpty() || message.m_id > 0) {
      // bool lbls_changed = false;

      if (uses_online_labels) {
        // Store all labels obtained from server.
        setLabelsForMessage(db, message.m_assignedLabels, message);
        // lbls_changed = true;
      }

      // Adjust labels tweaked by filters.
      for (Label* assigned_by_filter : message.m_assignedLabelsByFilter) {
        assigned_by_filter->assignToMessage(message, false);
        // lbls_changed = true;
      }

      for (Label* removed_by_filter : message.m_deassignedLabelsByFilter) {
        removed_by_filter->deassignFromMessage(message, false);
        // lbls_changed = true;
      }

      // NOTE: This is likely not needed at all
      // as this feature was semi-broken anyway, it was trigerred
      // even when labels were added (via article filtering or on service server)
      // and the same labels were already assigned in local DB.
      // In other words, label assignment differences were not correctly
      // taken into account.
      /*
      if (lbls_changed && !message.m_insertedUpdated) {
        // This article was not inserted/updated in DB because its contents did not change
        // but its assigned labels were changed. Therefore we must count article
        // as updated.
        if (!message.m_isRead) {
          updated_messages.m_unread.append(message);
        }

        updated_messages.m_all.append(message);
      }
      */
    }
    else {
      qWarningNN << LOGSEC_DB << "Cannot set labels for message" << QUOTE_W_SPACE(message.m_title)
                 << "because we don't have ID or custom ID.";
    }
  }

  QSqlQuery fixup_custom_ids_query(QSL("UPDATE Messages "
                                       "SET custom_id = id "
                                       "WHERE custom_id IS NULL OR custom_id = '';"),
                                   db);
  QSqlError fixup_custom_ids_error = fixup_custom_ids_query.lastError();

  if (fixup_custom_ids_error.isValid()) {
    qCriticalNN << LOGSEC_DB
                << "Failed to set custom ID for all messages:" << QUOTE_W_SPACE_DOT(fixup_custom_ids_error.text());
  }

  return updated_messages;
}

void DatabaseQueries::purgeMessagesFromBin(const QSqlDatabase& db, bool clear_only_read, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (clear_only_read) {
    q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE is_read = 1 AND is_deleted = 1 AND account_id = "
                  ":account_id;"));
  }
  else {
    q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE is_deleted = 1 AND account_id = :account_id;"));
  }

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::deleteAccount(const QSqlDatabase& db, ServiceRoot* account) {
  moveItem(account, false, true, {}, db);

  QSqlQuery q(db);

  q.setForwardOnly(true);
  QStringList queries;

  queries << QSL("DELETE FROM MessageFiltersInFeeds WHERE account_id = :account_id;")
          << QSL("DELETE FROM Messages WHERE account_id = :account_id;")
          << QSL("DELETE FROM Feeds WHERE account_id = :account_id;")
          << QSL("DELETE FROM Categories WHERE account_id = :account_id;")
          << QSL("DELETE FROM LabelsInMessages WHERE account_id = :account_id;")
          << QSL("DELETE FROM Labels WHERE account_id = :account_id;")
          << QSL("DELETE FROM Probes WHERE account_id = :account_id;")
          << QSL("DELETE FROM Accounts WHERE id = :account_id;");

  for (const QString& q_str : std::as_const(queries)) {
    q.prepare(q_str);
    q.bindValue(QSL(":account_id"), account->accountId());

    if (!q.exec()) {
      throw SqlException(q.lastError());
    }
    else {
      DatabaseFactory::logLastExecutedQuery(q);

      q.finish();
    }
  }
}

void DatabaseQueries::deleteAccountData(const QSqlDatabase& db,
                                        int account_id,
                                        bool delete_messages_too,
                                        bool delete_labels_too) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (delete_messages_too) {
    q.prepare(QSL("DELETE FROM Messages WHERE account_id = :account_id;"));
    q.bindValue(QSL(":account_id"), account_id);

    if (!q.exec()) {
      throw SqlException(q.lastError());
    }

    q.prepare(QSL("DELETE FROM LabelsInMessages WHERE account_id = :account_id;"));
    q.bindValue(QSL(":account_id"), account_id);

    if (!q.exec()) {
      throw SqlException(q.lastError());
    }

    DatabaseFactory::logLastExecutedQuery(q);
  }

  q.prepare(QSL("DELETE FROM Feeds WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  q.prepare(QSL("DELETE FROM Categories WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  if (delete_labels_too) {
    q.prepare(QSL("DELETE FROM Labels WHERE account_id = :account_id;"));
    q.bindValue(QSL(":account_id"), account_id);

    if (!q.exec()) {
      throw SqlException(q.lastError());
    }

    DatabaseFactory::logLastExecutedQuery(q);
  }
}

void DatabaseQueries::cleanMessagesByCondition(const QSqlDatabase& db,
                                               const QString& where_clause,
                                               bool clean_read_only,
                                               int account_id) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  QString sql = QSL("UPDATE Messages SET is_deleted = 1 "
                    "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id AND (%1)")
                  .arg(where_clause);

  if (clean_read_only) {
    sql += QSL(" AND is_read = 1");
  }

  sql += QL1C(';');

  q.prepare(sql);
  q.bindValue(":account_id", account_id);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::cleanImportantMessages(const QSqlDatabase& db, bool clean_read_only, int account_id) {
  cleanMessagesByCondition(db, QSL("is_important = 1"), clean_read_only, account_id);
}

void DatabaseQueries::cleanUnreadMessages(const QSqlDatabase& db, int account_id) {
  cleanMessagesByCondition(db, QSL("is_read = 0"), false, account_id);
}

void DatabaseQueries::cleanFeeds(const QSqlDatabase& db, const QStringList& ids, bool clean_read_only, int account_id) {
  if (ids.isEmpty()) {
    return;
  }

  cleanMessagesByCondition(db, QSL("feed IN (%1)").arg(ids.join(QSL(", "))), clean_read_only, account_id);
}

void DatabaseQueries::cleanLabelledMessages(const QSqlDatabase& db, bool clean_read_only, Label* label) {
  cleanMessagesByCondition(db,
                           QSL("id IN (SELECT message FROM LabelsInMessages WHERE label = %1 AND account_id = %2)")
                             .arg(label->id())
                             .arg(label->account()->accountId()),
                           clean_read_only,
                           label->account()->accountId());
}

void DatabaseQueries::cleanProbedMessages(const QSqlDatabase& db, bool clean_read_only, Search* probe) {
  cleanMessagesByCondition(db,
                           QSL("WHERE title REGEXP '%1' OR contents REGEXP '%1'")
                             .arg(probe->filter().replace(QSL("'"), QSL("''"))),
                           clean_read_only,
                           probe->account()->accountId());
}

void DatabaseQueries::purgeLeftoverMessageFilterAssignments(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM MessageFiltersInFeeds "
                "WHERE account_id = :account_id AND "
                "feed NOT IN (SELECT id FROM Feeds WHERE account_id = :account_id);"));
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::purgeLeftoverLabelAssignments(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);
  q.setForwardOnly(true);

  if (account_id > 0) {
    // Delete leftover assignments for a specific account.
    q.prepare(QSL("DELETE FROM LabelsInMessages "
                  "WHERE account_id = :account_id "
                  "AND message NOT IN (SELECT id FROM Messages WHERE account_id = :account_id);"));
    q.bindValue(QSL(":account_id"), account_id);
  }
  else {
    // Delete leftover assignments for all accounts.
    q.prepare(QSL("DELETE FROM LabelsInMessages "
                  "WHERE message NOT IN (SELECT id FROM Messages);"));
  }

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::storeAccountTree(const QSqlDatabase& db,
                                       RootItem* tree_root,
                                       int next_feed_id,
                                       int next_label_id,
                                       int account_id) {
  // Iterate all children.
  auto str = tree_root->getSubTree<RootItem>();

  for (RootItem* child : std::as_const(str)) {
    if (child->kind() == RootItem::Kind::Category) {
      createOverwriteCategory(db, child->toCategory(), account_id, child->parent()->id());
    }
    else if (child->kind() == RootItem::Kind::Feed) {
      int new_feed_id = 0;

      if (child->id() > 0) {
        // NOTE: This item has specified physical database ID, we re-use it.
        new_feed_id = child->id();
        child->setId(0);
      }
      else {
        // We generate new non-colision ID.
        new_feed_id = next_feed_id++;
      }

      createOverwriteFeed(db, child->toFeed(), account_id, child->parent()->id(), new_feed_id);
    }
    else if (child->kind() == RootItem::Kind::Labels) {
      // Add all labels.
      auto ch = child->childItems();

      for (RootItem* lbl : std::as_const(ch)) {
        Label* label = lbl->toLabel();

        int new_lbl_id = 0;

        if (lbl->id() > 0) {
          // NOTE: This item has specified physical database ID, we re-use it.
          new_lbl_id = lbl->id();
          lbl->setId(0);
        }
        else {
          // We generate new non-colision ID.
          new_lbl_id = next_label_id++;
        }

        createLabel(db, label, account_id, new_lbl_id);
      }
    }
  }
}

QStringList DatabaseQueries::customIdsOfMessagesFromLabel(const QSqlDatabase& db,
                                                          Label* label,
                                                          RootItem::ReadStatus read) {
  QString cond = QSL("Messages.is_deleted = 0 AND "
                     "Messages.is_pdeleted = 0 AND "
                     "Messages.account_id = :account AND "
                     "EXISTS ("
                     "  SELECT 1 FROM LabelsInMessages "
                     "  WHERE "
                     "    LabelsInMessages.label = :label AND "
                     "    LabelsInMessages.account_id = :account AND "
                     "    LabelsInMessages.message = Messages.id)");
  QMap<QString, QVariant> bindings{{QSL(":label"), label->id()}, {QSL(":account"), label->account()->accountId()}};

  if (read != RootItem::ReadStatus::Unknown) {
    cond += QSL(" AND Messages.is_read = :read");
    bindings[QSL(":read")] = read == RootItem::ReadStatus::Read ? 0 : 1;
  }

  return customIdsOfMessagesByCondition(db, cond, bindings);
}

QStringList DatabaseQueries::customIdsOfMessagesFromFeed(const QSqlDatabase& db,
                                                         int feed_id,
                                                         RootItem::ReadStatus read,
                                                         int account_id) {
  QString cond = QSL("Messages.feed = :feed AND "
                     "Messages.account_id = :acc_id AND "
                     "Messages.is_deleted = 0 AND "
                     "Messages.is_pdeleted = 0 AND "
                     "Messages.is_read = :read");

  return customIdsOfMessagesByCondition(db,
                                        cond,
                                        {{QSL(":feed"), feed_id},
                                         {QSL(":acc_id"), account_id},
                                         {QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfMessagesFromAccount(const QSqlDatabase& db,
                                                            RootItem::ReadStatus read,
                                                            int account_id) {
  QString cond = QSL("Messages.account_id = :acc_id AND "
                     "Messages.is_deleted = 0 AND "
                     "Messages.is_pdeleted = 0 AND "
                     "Messages.is_read = :read");

  return customIdsOfMessagesByCondition(db,
                                        cond,
                                        {{QSL(":acc_id"), account_id},
                                         {QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfImportantMessages(const QSqlDatabase& db,
                                                          RootItem::ReadStatus read,
                                                          int account_id) {
  QString cond = QSL("Messages.account_id = :acc_id AND "
                     "Messages.is_important = 1 AND "
                     "Messages.is_deleted = 0 AND "
                     "Messages.is_pdeleted = 0 AND "
                     "Messages.is_read = :read");

  return customIdsOfMessagesByCondition(db,
                                        cond,
                                        {{QSL(":acc_id"), account_id},
                                         {QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfUnreadMessages(const QSqlDatabase& db, int account_id) {
  QString cond = QSL("Messages.account_id = :acc_id AND "
                     "Messages.is_read = 0 AND "
                     "Messages.is_deleted = 0 AND "
                     "Messages.is_pdeleted = 0");

  return customIdsOfMessagesByCondition(db, cond, {{QSL(":acc_id"), account_id}});
}

QStringList DatabaseQueries::customIdsOfMessagesFromBin(const QSqlDatabase& db,
                                                        RootItem::ReadStatus read,
                                                        int account_id) {
  QString cond = QSL("Messages.account_id = :acc_id AND "
                     "Messages.is_deleted = 1 AND "
                     "Messages.is_read = :read");

  return customIdsOfMessagesByCondition(db,
                                        cond,
                                        {{QSL(":acc_id"), account_id},
                                         {QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfMessagesFromProbe(const QSqlDatabase& db,
                                                          Search* probe,
                                                          RootItem::ReadStatus read) {
  QString cond = QSL("(Messages.title REGEXP :fltr OR Messages.contents REGEXP :fltr) AND "
                     "Messages.account_id = :acc_id AND "
                     "Messages.is_deleted = 0 AND "
                     "Messages.is_pdeleted = 0 AND "
                     "Messages.is_read = :read");

  return customIdsOfMessagesByCondition(db,
                                        cond,
                                        {{QSL(":fltr"), probe->filter()},
                                         {QSL(":acc_id"), probe->account()->accountId()},
                                         {QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

void DatabaseQueries::createOverwriteCategory(const QSqlDatabase& db,
                                              Category* category,
                                              int account_id,
                                              int new_parent_id) {
  QSqlQuery q(db);
  int next_sort_order;

  if (category->id() <= 0 || (category->parent() != nullptr && category->parent()->id() != new_parent_id)) {
    q.prepare(QSL("SELECT MAX(ordr) FROM Categories WHERE account_id = :account_id AND parent_id = :parent_id;"));
    q.bindValue(QSL(":account_id"), account_id);
    q.bindValue(QSL(":parent_id"), new_parent_id);

    if (!q.exec() || !q.next()) {
      throw SqlException(q.lastError());
    }

    DatabaseFactory::logLastExecutedQuery(q);

    next_sort_order = (q.value(0).isNull() ? -1 : q.value(0).toInt()) + 1;
    q.finish();
  }
  else {
    next_sort_order = category->sortOrder();
  }

  if (category->id() <= 0) {
    // We need to insert category first.
    q.prepare(QSL("INSERT INTO "
                  "Categories (parent_id, ordr, title, date_created, account_id) "
                  "VALUES (0, 0, 'new', 0, %1);")
                .arg(QString::number(account_id)));

    if (!q.exec()) {
      throw SqlException(q.lastError());
    }
    else {
      DatabaseFactory::logLastExecutedQuery(q);

      category->setId(q.lastInsertId().toInt());
    }
  }
  else if (category->parent() != nullptr && category->parent()->id() != new_parent_id) {
    // Category is moving between parents.
    // 1. Move category to bottom of current parent.
    // 2. Assign proper new sort order.
    //
    // NOTE: The category will get reassigned to new parent usually after this method
    // completes by the caller.
    moveItem(category, false, true, {}, db);
  }

  // Restore to correct sort order.
  category->setSortOrder(next_sort_order);

  q.prepare("UPDATE Categories "
            "SET parent_id = :parent_id, ordr = :ordr, title = :title, description = :description, date_created = "
            ":date_created, "
            "    icon = :icon, account_id = :account_id, custom_id = :custom_id "
            "WHERE id = :id;");
  q.bindValue(QSL(":parent_id"), new_parent_id);
  q.bindValue(QSL(":title"), category->title());
  q.bindValue(QSL(":description"), category->description());
  q.bindValue(QSL(":date_created"), category->creationDate().toMSecsSinceEpoch());
  q.bindValue(QSL(":icon"), qApp->icons()->toByteArray(category->icon()));
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":custom_id"), category->customId());
  q.bindValue(QSL(":id"), category->id());
  q.bindValue(QSL(":ordr"), category->sortOrder());

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::createOverwriteFeed(const QSqlDatabase& db,
                                          Feed* feed,
                                          int account_id,
                                          int new_parent_id,
                                          int new_feed_id) {
  QSqlQuery q(db);
  int next_sort_order;

  if (feed->id() <= 0 || (feed->parent() != nullptr && feed->parent()->id() != new_parent_id)) {
    // We either insert completely new feed or we move feed
    // to new parent. Get new viable sort order.
    q.prepare(QSL("SELECT MAX(ordr) FROM Feeds WHERE account_id = :account_id AND category = :category;"));
    q.bindValue(QSL(":account_id"), account_id);
    q.bindValue(QSL(":category"), new_parent_id);

    if (!q.exec() || !q.next()) {
      throw SqlException(q.lastError());
    }

    DatabaseFactory::logLastExecutedQuery(q);

    next_sort_order = (q.value(0).isNull() ? -1 : q.value(0).toInt()) + 1;
    q.finish();
  }
  else {
    next_sort_order = feed->sortOrder();
  }

  if (feed->id() <= 0) {
    // We need to insert feed first.
    if (new_feed_id > 0) {
      q.prepare(QSL("INSERT INTO "
                    "Feeds (id, title, ordr, date_created, category, update_type, update_interval, account_id, "
                    "custom_id) "
                    "VALUES (%2, 'new', 0, 0, 0, 0, 1, %1, 'new');")
                  .arg(QString::number(account_id), QString::number(new_feed_id)));
    }
    else {
      q.prepare(QSL("INSERT INTO "
                    "Feeds (title, ordr, date_created, category, update_type, update_interval, account_id, custom_id) "
                    "VALUES ('new', 0, 0, 0, 0, 1, %1, 'new');")
                  .arg(QString::number(account_id)));
    }

    if (!q.exec()) {
      throw SqlException(q.lastError());
    }
    else {
      DatabaseFactory::logLastExecutedQuery(q);

      feed->setId(q.lastInsertId().toInt());

      if (feed->customId().isEmpty()) {
        feed->setCustomId(QString::number(feed->id()));
      }
    }
  }
  else if (feed->parent() != nullptr && feed->parent()->id() != new_parent_id) {
    // Feed is moving between categories.
    // 1. Move feed to bottom of current category.
    // 2. Assign proper new sort order.
    //
    // NOTE: The feed will get reassigned to new parent usually after this method
    // completes by the caller.
    moveItem(feed, false, true, {}, db);
  }

  // Restore to correct sort order.
  feed->setSortOrder(next_sort_order);

  q.prepare("UPDATE Feeds "
            "SET "
            "title = :title, "
            "ordr = :ordr, "
            "description = :description, "
            "date_created = :date_created, "
            "icon = :icon, "
            "category = :category, "
            "source = :source, "
            "update_type = :update_type, "
            "update_interval = :update_interval, "
            "is_off = :is_off, "
            "is_quiet = :is_quiet, "
            "is_rtl = :is_rtl, "
            "add_any_datetime_articles = :add_any_datetime_articles, "
            "datetime_to_avoid = :datetime_to_avoid, "
            "keep_article_customize = :keep_article_customize, "
            "keep_article_count = :keep_article_count, "
            "keep_unread_articles = :keep_unread_articles, "
            "keep_starred_articles = :keep_starred_articles, "
            "recycle_articles = :recycle_articles, "
            "account_id = :account_id, "
            "custom_id = :custom_id, "
            "custom_data = :custom_data "
            "WHERE id = :id;");
  q.bindValue(QSL(":title"), feed->title());
  q.bindValue(QSL(":description"), feed->description());
  q.bindValue(QSL(":date_created"), feed->creationDate().toMSecsSinceEpoch());
  q.bindValue(QSL(":icon"), qApp->icons()->toByteArray(feed->icon()));
  q.bindValue(QSL(":category"), new_parent_id);
  q.bindValue(QSL(":source"), feed->source());
  q.bindValue(QSL(":update_type"), int(feed->autoUpdateType()));
  q.bindValue(QSL(":update_interval"), feed->autoUpdateInterval());
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":custom_id"), feed->customId());
  q.bindValue(QSL(":id"), feed->id());
  q.bindValue(QSL(":ordr"), feed->sortOrder());
  q.bindValue(QSL(":is_off"), feed->isSwitchedOff());
  q.bindValue(QSL(":is_quiet"), feed->isQuiet());
  q.bindValue(QSL(":is_rtl"), int(feed->rtlBehavior()));

  const Feed::ArticleIgnoreLimit art = feed->articleIgnoreLimit();

  q.bindValue(QSL(":add_any_datetime_articles"), art.m_addAnyArticlesToDb);
  q.bindValue(QSL(":datetime_to_avoid"),
              (art.m_dtToAvoid.isValid() && art.m_dtToAvoid.toMSecsSinceEpoch() > 0)
                ? art.m_dtToAvoid.toMSecsSinceEpoch()
                : art.m_hoursToAvoid);

  q.bindValue(QSL(":keep_article_customize"), art.m_customizeLimitting);
  q.bindValue(QSL(":keep_article_count"), art.m_keepCountOfArticles);
  q.bindValue(QSL(":keep_unread_articles"), art.m_doNotRemoveUnread);
  q.bindValue(QSL(":keep_starred_articles"), art.m_doNotRemoveStarred);
  q.bindValue(QSL(":recycle_articles"), art.m_moveToBinDontPurge);

  auto custom_data = feed->customDatabaseData();
  QString serialized_custom_data = serializeCustomData(custom_data);

  q.bindValue(QSL(":custom_data"), serialized_custom_data);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::createOverwriteAccount(const QSqlDatabase& db, ServiceRoot* account) {
  QSqlQuery q(db);

  if (account->accountId() <= 0) {
    // We need to insert account and generate sort order first.
    if (account->sortOrder() < 0) {
      if (!q.exec(QSL("SELECT MAX(ordr) FROM Accounts;"))) {
        throw SqlException(q.lastError());
      }

      DatabaseFactory::logLastExecutedQuery(q);

      q.next();

      int next_order = (q.value(0).isNull() ? -1 : q.value(0).toInt()) + 1;

      account->setSortOrder(next_order);
      q.finish();
    }

    q.prepare(QSL("INSERT INTO Accounts (ordr, type) "
                  "VALUES (0, :type);"));
    q.bindValue(QSL(":type"), account->code());

    if (!q.exec()) {
      throw SqlException(q.lastError());
    }
    else {
      DatabaseFactory::logLastExecutedQuery(q);

      account->setAccountId(q.lastInsertId().toInt());
    }
  }

  // Now we construct the SQL update query.
  auto proxy = account->networkProxy();

  q.prepare(QSL("UPDATE Accounts "
                "SET proxy_type = :proxy_type, proxy_host = :proxy_host, proxy_port = :proxy_port, "
                "    proxy_username = :proxy_username, proxy_password = :proxy_password, ordr = :ordr, "
                "    custom_data = :custom_data "
                "WHERE id = :id"));
  q.bindValue(QSL(":proxy_type"), proxy.type());
  q.bindValue(QSL(":proxy_host"), proxy.hostName());
  q.bindValue(QSL(":proxy_port"), proxy.port());
  q.bindValue(QSL(":proxy_username"), proxy.user());
  q.bindValue(QSL(":proxy_password"), TextFactory::encrypt(proxy.password()));
  q.bindValue(QSL(":id"), account->accountId());
  q.bindValue(QSL(":ordr"), account->sortOrder());

  auto custom_data = account->customDatabaseData();
  QString serialized_custom_data = serializeCustomData(custom_data);

  q.bindValue(QSL(":custom_data"), serialized_custom_data);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::deleteFeed(const QSqlDatabase& db, Feed* feed, int account_id) {
  moveItem(feed, false, true, {}, db);

  QSqlQuery q(db);

  q.prepare(QSL("DELETE FROM Messages WHERE feed = :feed AND account_id = :account_id;"));
  q.bindValue(QSL(":feed"), feed->id());
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseQueries::purgeLeftoverLabelAssignments(db, account_id);
  DatabaseFactory::logLastExecutedQuery(q);

  // Remove feed itself.
  q.prepare(QSL("DELETE FROM Feeds WHERE custom_id = :custom_id AND account_id = :account_id;"));
  q.bindValue(QSL(":custom_id"), feed->customId());
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  purgeLeftoverMessageFilterAssignments(db, account_id);
}

void DatabaseQueries::deleteCategory(const QSqlDatabase& db, Category* category) {
  moveItem(category, false, true, {}, db);

  QSqlQuery q(db);

  // Remove this category from database.
  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Categories WHERE id = :category;"));
  q.bindValue(QSL(":category"), category->id());

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::moveItem(RootItem* item,
                               bool move_top,
                               bool move_bottom,
                               int move_index,
                               const QSqlDatabase& db) {
  if (item->kind() != RootItem::Kind::Feed && item->kind() != RootItem::Kind::Category &&
      item->kind() != RootItem::Kind::ServiceRoot) {
    return;
  }

  auto neighbors = item->parent()->childItems();
  int max_sort_order = boolinq::from(neighbors)
                         .select([=](RootItem* it) {
                           return it->kind() == item->kind() ? it->sortOrder() : 0;
                         })
                         .max();

  if ((!move_top && !move_bottom && item->sortOrder() == move_index) || /* Item is already sorted OK. */
      (!move_top && !move_bottom &&
       move_index < 0) || /* Order cannot be smaller than 0 if we do not move to begin/end. */
      (!move_top && !move_bottom && move_index > max_sort_order) || /* Cannot move past biggest sort order. */
      (move_top && item->sortOrder() == 0) ||                       /* Item is already on top. */
      (move_bottom && item->sortOrder() == max_sort_order) ||       /* Item is already on bottom. */
      max_sort_order <= 0) {                                        /* We only have 1 item, nothing to sort. */
    return;
  }

  QSqlQuery q(db);

  if (move_top) {
    move_index = 0;
  }
  else if (move_bottom) {
    move_index = max_sort_order;
  }

  int move_low = qMin(move_index, item->sortOrder());
  int move_high = qMax(move_index, item->sortOrder());
  QString parent_field, table_name;

  switch (item->kind()) {
    case RootItem::Kind::Feed:
      parent_field = QSL("category");
      table_name = QSL("Feeds");
      break;

    case RootItem::Kind::Category:
      parent_field = QSL("parent_id");
      table_name = QSL("Categories");
      break;

    case RootItem::Kind::ServiceRoot:
      table_name = QSL("Accounts");
      break;

    default:
      throw ApplicationException(QObject::tr("cannot move item of kind %1").arg(int(item->kind())));
  }

  if (item->kind() == RootItem::Kind::ServiceRoot) {
    if (item->sortOrder() > move_index) {
      q.prepare(QSL("UPDATE Accounts SET ordr = ordr + 1 "
                    "WHERE ordr < :move_high AND ordr >= :move_low;"));
    }
    else {
      q.prepare(QSL("UPDATE Accounts SET ordr = ordr - 1 "
                    "WHERE ordr > :move_low AND ordr <= :move_high;"));
    }
  }
  else {
    if (item->sortOrder() > move_index) {
      q.prepare(QSL("UPDATE %1 SET ordr = ordr + 1 "
                    "WHERE account_id = :account_id AND %2 = :category AND ordr < :move_high AND ordr >= :move_low;")
                  .arg(table_name, parent_field));
    }
    else {
      q.prepare(QSL("UPDATE %1 SET ordr = ordr - 1 "
                    "WHERE account_id = :account_id AND %2 = :category AND ordr > :move_low AND ordr <= :move_high;")
                  .arg(table_name, parent_field));
    }

    q.bindValue(QSL(":account_id"), item->account()->accountId());
    q.bindValue(QSL(":category"), item->parent()->id());
  }

  q.bindValue(QSL(":move_low"), move_low);
  q.bindValue(QSL(":move_high"), move_high);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  q.prepare(QSL("UPDATE %1 SET ordr = :ordr WHERE id = :id;").arg(table_name));
  q.bindValue(QSL(":id"),
              item->kind() == RootItem::Kind::ServiceRoot ? item->toServiceRoot()->accountId() : item->id());
  q.bindValue(QSL(":ordr"), move_index);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  // Fix live sort orders.
  if (item->sortOrder() > move_index) {
    boolinq::from(neighbors)
      .where([=](RootItem* it) {
        return it->kind() == item->kind() && it->sortOrder() < move_high && it->sortOrder() >= move_low;
      })
      .for_each([](RootItem* it) {
        it->setSortOrder(it->sortOrder() + 1);
      });
  }
  else {
    boolinq::from(neighbors)
      .where([=](RootItem* it) {
        return it->kind() == item->kind() && it->sortOrder() > move_low && it->sortOrder() <= move_high;
      })
      .for_each([](RootItem* it) {
        it->setSortOrder(it->sortOrder() - 1);
      });
  }

  item->setSortOrder(move_index);
}

void DatabaseQueries::moveMessageFilter(QList<MessageFilter*> all_filters,
                                        MessageFilter* filter,
                                        bool move_top,
                                        bool move_bottom,
                                        int move_index,
                                        const QSqlDatabase& db) {
  int max_sort_order = boolinq::from(all_filters)
                         .select([=](MessageFilter* it) {
                           return it->sortOrder();
                         })
                         .max();

  if ((!move_top && !move_bottom && filter->sortOrder() == move_index) || /* Item is already sorted OK. */
      (!move_top && !move_bottom &&
       move_index < 0) || /* Order cannot be smaller than 0 if we do not move to begin/end. */
      (!move_top && !move_bottom && move_index > max_sort_order) || /* Cannot move past biggest sort order. */
      (move_top && filter->sortOrder() == 0) ||                     /* Item is already on top. */
      (move_bottom && filter->sortOrder() == max_sort_order) ||     /* Item is already on bottom. */
      max_sort_order <= 0) {                                        /* We only have 1 item, nothing to sort. */
    return;
  }

  QSqlQuery q(db);

  if (move_top) {
    move_index = 0;
  }
  else if (move_bottom) {
    move_index = max_sort_order;
  }

  int move_low = qMin(move_index, filter->sortOrder());
  int move_high = qMax(move_index, filter->sortOrder());

  if (filter->sortOrder() > move_index) {
    q.prepare(QSL("UPDATE MessageFilters SET ordr = ordr + 1 "
                  "WHERE ordr < :move_high AND ordr >= :move_low;"));
  }
  else {
    q.prepare(QSL("UPDATE MessageFilters SET ordr = ordr - 1 "
                  "WHERE ordr > :move_low AND ordr <= :move_high;"));
  }

  q.bindValue(QSL(":move_low"), move_low);
  q.bindValue(QSL(":move_high"), move_high);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  q.prepare(QSL("UPDATE MessageFilters SET ordr = :ordr WHERE id = :id;"));
  q.bindValue(QSL(":id"), filter->id());
  q.bindValue(QSL(":ordr"), move_index);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  // Fix live sort orders.
  if (filter->sortOrder() > move_index) {
    boolinq::from(all_filters)
      .where([=](MessageFilter* it) {
        return it->sortOrder() < move_high && it->sortOrder() >= move_low;
      })
      .for_each([](MessageFilter* it) {
        it->setSortOrder(it->sortOrder() + 1);
      });
  }
  else {
    boolinq::from(all_filters)
      .where([=](MessageFilter* it) {
        return it->sortOrder() > move_low && it->sortOrder() <= move_high;
      })
      .for_each([](MessageFilter* it) {
        it->setSortOrder(it->sortOrder() - 1);
      });
  }

  filter->setSortOrder(move_index);
}

MessageFilter* DatabaseQueries::addMessageFilter(const QSqlDatabase& db, const QString& title, const QString& script) {
  if (!db.driver()->hasFeature(QSqlDriver::DriverFeature::LastInsertId)) {
    throw ApplicationException(QObject::tr("Cannot insert article filter, because current database cannot return last "
                                           "inserted row ID."));
  }

  QSqlQuery q(db);

  if (!q.exec(QSL("SELECT COUNT(*) FROM MessageFilters;"))) {
    throw SqlException(q.lastError());
  }

  q.next();

  DatabaseFactory::logLastExecutedQuery(q);

  int new_ordr = q.value(0).toInt();

  q.prepare(QSL("INSERT INTO MessageFilters (name, script, ordr) VALUES(:name, :script, :ordr);"));
  q.bindValue(QSL(":name"), title);
  q.bindValue(QSL(":script"), script);
  q.bindValue(QSL(":ordr"), new_ordr);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  auto* fltr = new MessageFilter(q.lastInsertId().toInt());

  fltr->setName(title);
  fltr->setScript(script);
  fltr->setSortOrder(new_ordr);

  return fltr;
}

void DatabaseQueries::removeMessageFilter(const QSqlDatabase& db, int filter_id) {
  QSqlQuery q(db);

  q.prepare(QSL("DELETE FROM MessageFilters WHERE id = :id;"));

  q.bindValue(QSL(":id"), filter_id);
  q.setForwardOnly(true);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

void DatabaseQueries::removeMessageFilterAssignments(const QSqlDatabase& db, int filter_id) {
  QSqlQuery q(db);

  q.prepare(QSL("DELETE FROM MessageFiltersInFeeds WHERE filter = :filter;"));

  q.bindValue(QSL(":filter"), filter_id);
  q.setForwardOnly(true);

  if (!q.exec()) {
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

QList<MessageFilter*> DatabaseQueries::getMessageFilters(const QSqlDatabase& db) {
  QSqlQuery q(db);
  QList<MessageFilter*> filters;

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT id, name, script, is_enabled, ordr FROM MessageFilters;"));

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    while (q.next()) {
      auto* filter = new MessageFilter(q.value(0).toInt());

      filter->setName(q.value(1).toString());
      filter->setScript(q.value(2).toString());
      filter->setEnabled(q.value(3).toBool());
      filter->setSortOrder(q.value(4).toInt());

      filters.append(filter);
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  return filters;
}

void DatabaseQueries::assignMessageFilterToFeed(const QSqlDatabase& db, int feed_id, int filter_id, int account_id) {
  QSqlQuery q(db);

  q.prepare(QSL("SELECT COUNT(*) FROM MessageFiltersInFeeds "
                "WHERE filter = :filter AND feed = :feed AND account_id = :account_id;"));
  q.setForwardOnly(true);
  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed"), feed_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    DatabaseFactory::logLastExecutedQuery(q);

    auto already_included_count = q.value(0).toInt();

    if (already_included_count > 0) {
      return;
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  q.prepare(QSL("INSERT INTO MessageFiltersInFeeds (filter, feed, account_id) "
                "VALUES(:filter, :feed, :account_id);"));
  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed"), feed_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::updateMessageFilter(const QSqlDatabase& db, MessageFilter* filter) {
  QSqlQuery q(db);

  q.prepare(QSL("UPDATE MessageFilters SET name = :name, script = :script, is_enabled = :is_enabled, ordr = :ordr "
                "WHERE id = :id;"));

  q.bindValue(QSL(":name"), filter->name());
  q.bindValue(QSL(":script"), filter->script());
  q.bindValue(QSL(":id"), filter->id());
  q.bindValue(QSL(":is_enabled"), filter->enabled() ? 1 : 0);
  q.bindValue(QSL(":ordr"), filter->sortOrder());
  q.setForwardOnly(true);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

void DatabaseQueries::removeMessageFilterFromFeed(const QSqlDatabase& db, int feed_id, int filter_id, int account_id) {
  QSqlQuery q(db);

  q.prepare(QSL("DELETE FROM MessageFiltersInFeeds "
                "WHERE filter = :filter AND feed = :feed AND account_id = :account_id;"));

  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed"), feed_id);
  q.bindValue(QSL(":account_id"), account_id);
  q.setForwardOnly(true);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);
  }
  else {
    throw SqlException(q.lastError());
  }
}

QStringList DatabaseQueries::getAllGmailRecipients(const QSqlDatabase& db, int account_id) {
  QSqlQuery query(db);
  QStringList rec;

  query.prepare(QSL("SELECT DISTINCT author "
                    "FROM Messages "
                    "WHERE account_id = :account_id AND author IS NOT NULL AND author != '' "
                    "ORDER BY lower(author) ASC;"));
  query.bindValue(QSL(":account_id"), account_id);

  if (query.exec()) {
    DatabaseFactory::logLastExecutedQuery(query);

    while (query.next()) {
      rec.append(query.value(0).toString());
    }
  }
  else {
    qWarningNN << LOGSEC_GMAIL << "Query for all recipients failed: '" << query.lastError().text() << "'.";
  }

  return rec;
}

QMultiMap<int, int> DatabaseQueries::messageFiltersInFeeds(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);
  QMultiMap<int, int> filters_in_feeds;

  q.prepare(QSL("SELECT filter, feed FROM MessageFiltersInFeeds WHERE account_id = :account_id;"));

  q.bindValue(QSL(":account_id"), account_id);
  q.setForwardOnly(true);

  if (q.exec()) {
    DatabaseFactory::logLastExecutedQuery(q);

    while (q.next()) {
      filters_in_feeds.insert(q.value(1).toInt(), q.value(0).toInt());
    }
  }
  else {
    throw SqlException(q.lastError());
  }

  return filters_in_feeds;
}

void DatabaseQueries::storeNewOauthTokens(const QSqlDatabase& db, const QString& refresh_token, int account_id) {
  QSqlQuery q(db);

  q.prepare(QSL("SELECT custom_data FROM Accounts WHERE id = :id;"));
  q.bindValue(QSL(":id"), account_id);

  if (!q.exec() || !q.next()) {
    qWarningNN << LOGSEC_OAUTH << "Cannot fetch custom data column for storing of OAuth tokens, because of error:"
               << QUOTE_W_SPACE_DOT(q.lastError().text());
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);

  QVariantHash custom_data = deserializeCustomData(q.value(0).toString());

  custom_data[QSL("refresh_token")] = refresh_token;

  q.clear();
  q.prepare(QSL("UPDATE Accounts SET custom_data = :custom_data WHERE id = :id;"));
  q.bindValue(QSL(":custom_data"), serializeCustomData(custom_data));
  q.bindValue(QSL(":id"), account_id);

  if (!q.exec()) {
    qWarningNN << LOGSEC_OAUTH
               << "Cannot store OAuth tokens, because of error:" << QUOTE_W_SPACE_DOT(q.lastError().text());
    throw SqlException(q.lastError());
  }

  DatabaseFactory::logLastExecutedQuery(q);
}

QString DatabaseQueries::unnulifyString(const QString& str) {
  return str.isNull() ? QSL("") : str;
}
