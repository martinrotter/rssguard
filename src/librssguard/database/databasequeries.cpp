// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasequeries.h"

#include "database/databasefactory.h"
#include "database/sqlquery.h"
#include "definitions/globals.h"
#include "exceptions/sqlexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/category.h"
#include "services/abstract/labelsnode.h"

#include <optional>

#include <QDateTime>
#include <QSet>
#include <QSqlDriver>
#include <QUrl>
#include <QVariant>
#include <QVersionNumber>

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
  field_names.append(QSL("Messages.date_retrieved"));
  field_names.append(QSL("Messages.contents"));
  field_names.append(QSL("Messages.enclosures"));
  field_names.append(QSL("Messages.score"));
  field_names.append(QSL("Messages.account_id"));
  field_names.append(QSL("Messages.custom_id"));
  field_names.append(QSL("Messages.custom_data"));
  field_names.append(QSL("("
                         "SELECT GROUP_CONCAT(Labels.custom_id) "
                         "FROM LabelsInMessages lim "
                         "JOIN Labels ON lim.label = Labels.id "
                         "WHERE lim.message = Messages.id"
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

void DatabaseQueries::storeCategoryCustomData(const QSqlDatabase& db, Category* category) {
  storeCustomData(db, QSL("Categories"), category->id(), category->customDatabaseData());
}

QVariantHash DatabaseQueries::loadAccountCustomData(const QSqlDatabase& db, int account_id) {
  SqlQuery query(db);
  query.prepare(QSL("SELECT custom_data FROM Accounts WHERE id = :id;"));
  query.bindValue(QSL(":id"), account_id);
  query.exec();
  if (!query.next()) {
    throw SqlException(SqlException::Type::GeneralError, QObject::tr("Account no longer exists in the database."));
  }
  const QString data = query.value(0).toString();
  if (data.isEmpty()) {
    return {};
  }
  QJsonParseError error;
  const auto document = QJsonDocument::fromJson(data.toUtf8(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject()) {
    throw SqlException(SqlException::Type::GeneralError, QObject::tr("Invalid account custom data."));
  }
  return document.object().toVariantHash();
}

QVariantHash DatabaseQueries::modifyAccountCustomData(const QSqlDatabase& db,
                                                      int account_id,
                                                      const std::function<void(QVariantHash&)>& edit) {
  auto data = loadAccountCustomData(db, account_id);
  edit(data);
  storeCustomData(db, QSL("Accounts"), account_id, data);
  return data;
}

void DatabaseQueries::storeFeedCustomData(const QSqlDatabase& db, Feed* feed) {
  storeCustomData(db, QSL("Feeds"), feed->id(), feed->customDatabaseData());
}

QVariantHash DatabaseQueries::loadFeedCustomData(const QSqlDatabase& db, Feed* feed) {
  SqlQuery query(db);
  query.prepare(QSL("SELECT custom_data FROM Feeds WHERE id = :id;"));
  query.bindValue(QSL(":id"), feed->id());
  query.exec();

  if (!query.next()) {
    throw SqlException(SqlException::Type::GeneralError, QObject::tr("Feed no longer exists in the database."));
  }

  return deserializeCustomData(query.value(0).toString());
}

QString DatabaseQueries::whereClauseBin(int account_id) {
  return QSL("Messages.account_id = %1 AND Messages.is_deleted = 1 AND Messages.is_pdeleted = 0").arg(account_id);
}

QString DatabaseQueries::whereClauseImportantArticles(int account_id) {
  return QSL("Messages.account_id = %1 AND "
             "Messages.is_deleted = 0 AND "
             "Messages.is_pdeleted = 0 AND "
             "Messages.is_important = 1")
    .arg(account_id);
}

QString DatabaseQueries::whereClauseUnreadArticles(int account_id) {
  return QSL("Messages.account_id = %1 AND "
             "Messages.is_deleted = 0 AND "
             "Messages.is_pdeleted = 0 AND "
             "Messages.is_read = 0")
    .arg(account_id);
}

QString DatabaseQueries::whereClauseProbe(Search* probe, int account_id) {
  if (probe->type() == Search::Type::Regex) {
    return QSL("Messages.account_id = %1 AND "
               "Messages.is_deleted = 0 AND "
               "Messages.is_pdeleted = 0 AND "
               "(Messages.title REGEXP '%2' OR Messages.contents REGEXP '%2')")
      .arg(QString::number(account_id), DatabaseFactory::escapeQuery(probe->filter()));
  }
  else {
    return QSL("Messages.account_id = %1 AND "
               "Messages.is_deleted = 0 AND "
               "Messages.is_pdeleted = 0 AND "
               "(%2)")
      .arg(QString::number(account_id), probe->filter());
  }
}

QString DatabaseQueries::whereClauseLabel(int label_id, int account_id) {
  return QSL("Messages.account_id = %1 AND "
             "Messages.is_deleted = 0 AND "
             "Messages.is_pdeleted = 0 AND "
             "EXISTS (SELECT 1 "
             "FROM LabelsInMessages "
             "WHERE "
             "  %2 "
             "  LabelsInMessages.message = Messages.id)")
    .arg(QString::number(account_id), label_id > 0 ? QSL("LabelsInMessages.label = %1 AND").arg(label_id) : QString());
}

QString DatabaseQueries::whereClauseLabels(int account_id) {
  return whereClauseLabel(0, account_id);
}

QString DatabaseQueries::whereClauseAccount(bool including_deleted, int account_id) {
  return QSL("Messages.account_id = %1 AND "
             "%2 "
             "Messages.is_pdeleted = 0")
    .arg(QString::number(account_id), including_deleted ? QString() : QSL("Messages.is_deleted = 0 AND"));
}

QString DatabaseQueries::whereClauseFeeds(const QStringList& feed_ids) {
  QString filter_clause;

  switch (feed_ids.size()) {
    case 0:
      filter_clause = QSL("Messages.feed IN (NULL)");
      break;

    case 1:
      filter_clause = QSL("Messages.feed = %1").arg(feed_ids.first());
      break;

    default:
      filter_clause = QSL("Messages.feed IN (%1)").arg(feed_ids.join(QSL(", ")));
      break;
  }

  return QSL("%1 AND "
             "Messages.is_deleted = 0 AND "
             "Messages.is_pdeleted = 0")
    .arg(filter_clause);
}

void DatabaseQueries::deassignLabelFromMessages(const QSqlDatabase& db,
                                                Label* label,
                                                const QList<Message>& messages,
                                                bool skip_permanently_deleted) {
  constexpr int batch_size = 500;

  for (int offset = 0; offset < messages.size(); offset += batch_size) {
    const int count = std::min(batch_size, int(messages.size()) - offset);
    QStringList message_ids;

    message_ids.reserve(count);

    for (int i = offset; i < offset + count; ++i) {
      message_ids.append(QString::number(messages.at(i).m_id));
    }

    SqlQuery q(db);

    const QString eligible_ids =
      skip_permanently_deleted
        ? QSL("SELECT id FROM Messages WHERE id IN (%1) AND is_pdeleted = 0").arg(message_ids.join(QSL(", ")))
        : message_ids.join(QSL(", "));
    q.prepare(QSL("DELETE FROM LabelsInMessages "
                  "WHERE label = :label AND message IN (%1);")
                .arg(eligible_ids));
    q.bindValue(QSL(":label"), label->id());
    q.exec();
  }
}

void DatabaseQueries::assignLabelToMessages(const DatabaseDriver* driver,
                                            const QSqlDatabase& db,
                                            Label* label,
                                            const QList<Message>& messages,
                                            bool skip_permanently_deleted) {
  constexpr int batch_size = 500;

  for (int offset = 0; offset < messages.size(); offset += batch_size) {
    const int count = std::min(batch_size, int(messages.size()) - offset);
    QStringList values;
    QStringList message_ids;

    values.reserve(count);

    for (int i = offset; i < offset + count; ++i) {
      values.append(QSL("(%1, %2)").arg(messages.at(i).m_id).arg(label->id()));
      if (skip_permanently_deleted) {
        message_ids.append(QString::number(messages.at(i).m_id));
      }
    }

    SqlQuery q(db);

    if (skip_permanently_deleted) {
      q.prepare(QSL("%1 INTO LabelsInMessages (message, label) "
                    "SELECT id, :label FROM Messages WHERE id IN (%2) AND is_pdeleted = 0;")
                  .arg(driver->insertIgnore(), message_ids.join(QSL(", "))));
      q.bindValue(QSL(":label"), label->id());
      q.exec();
    }
    else {
      q.exec(QSL("%1 INTO LabelsInMessages (message, label) VALUES %2;")
               .arg(driver->insertIgnore(), values.join(QSL(", "))));
    }
  }
}

void DatabaseQueries::setLabelsForMessage(const QSqlDatabase& db, const QList<Label*>& labels, const Message& msg) {
  SqlQuery q(db);

  // Remove everything first.
  q.prepare(QSL("DELETE FROM LabelsInMessages "
                "WHERE LabelsInMessages.message = :message;"));
  q.bindValue(QSL(":message"), msg.m_id);
  q.exec();

  // Insert new label associations (if any).
  if (!labels.isEmpty()) {
    QString sql = QSL("INSERT INTO LabelsInMessages (message, label) VALUES ");
    QStringList values;

    values.reserve(labels.size());

    for (const Label* lbl : labels) {
      values << QSL("(%1, %2)").arg(msg.m_id).arg(lbl->id());
    }

    sql += values.join(QSL(", ")) + QL1C(';');

    q.exec(sql);
  }
}

QList<Label*> DatabaseQueries::getLabelsForAccount(IconFactory* icons, const QSqlDatabase& db, int account_id) {
  QList<Label*> labels;
  SqlQuery q(db);

  q.prepare(QSL("SELECT * FROM Labels WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  q.exec();

  while (q.next()) {
    Label* lbl = new Label(q.value(QSL("name")).toString(), icons->fromByteArray(q.value(QSL("icon")).toByteArray()));

    lbl->setId(q.value(QSL("id")).toInt());
    lbl->setCustomId(q.value(QSL("custom_id")).toString());

    labels << lbl;
  }

  return labels;
}

void DatabaseQueries::updateLabel(IconFactory* icons, const QSqlDatabase& db, Label* label) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE Labels "
                "SET name = :name, icon = :icon "
                "WHERE id = :id;"));
  q.bindValue(QSL(":name"), label->title());
  q.bindValue(QSL(":icon"), icons->toByteArray(label->icon()));
  q.bindValue(QSL(":id"), label->id());

  q.exec();
}

void DatabaseQueries::deleteLabel(const QSqlDatabase& db, Label* label) {
  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM Labels WHERE id = :id;"));
  q.bindValue(QSL(":id"), label->id());

  q.exec();
}

void DatabaseQueries::createLabel(IconFactory* icons,
                                  const QSqlDatabase& db,
                                  Label* label,
                                  int account_id,
                                  int new_label_id) {
  SqlQuery q(db);

  if (new_label_id > 0) {
    q.prepare(QSL("INSERT INTO Labels (id, name, icon, custom_id, account_id) "
                  "VALUES (:id, :name, :icon, :custom_id, :account_id);"));
    q.bindValue(QSL(":id"), new_label_id);
  }
  else {
    q.prepare(QSL("INSERT INTO Labels (name, icon, custom_id, account_id) "
                  "VALUES (:name, :icon, :custom_id, :account_id);"));
  }

  q.bindValue(QSL(":name"), label->title());
  q.bindValue(QSL(":icon"), icons->toByteArray(label->icon()));
  q.bindValue(QSL(":custom_id"), label->customId());
  q.bindValue(QSL(":account_id"), account_id);
  q.exec();

  label->setId(q.lastInsertId().toInt());

  // NOTE: This custom ID in this object will be probably
  // overwritten in online-synchronized labels.
  if (label->customId().isEmpty()) {
    label->setCustomId(QString::number(label->id()));
  }

  // Fixup missing custom IDs.
  q.prepare(QSL("UPDATE Labels SET custom_id = id WHERE custom_id IS NULL OR custom_id = '';"));
  q.exec();
}

void DatabaseQueries::updateProbe(const QSqlDatabase& db, Search* probe) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE Probes SET name = :name, type = :type, fltr = :fltr, icon = :icon "
                "WHERE id = :id AND account_id = :account_id;"));
  q.bindValue(QSL(":name"), probe->title());
  q.bindValue(QSL(":type"), int(probe->type()));
  q.bindValue(QSL(":fltr"), probe->filter());
  q.bindValue(QSL(":icon"), IconFactory::toByteArray(probe->icon()));
  q.bindValue(QSL(":id"), probe->id());
  q.bindValue(QSL(":account_id"), probe->account()->accountId());

  q.exec();
}

void DatabaseQueries::createProbe(const QSqlDatabase& db, Search* probe, int account_id) {
  SqlQuery q(db);

  q.prepare(QSL("INSERT INTO Probes (name, type, icon, fltr, account_id) "
                "VALUES (:name, :type, :icon, :fltr, :account_id);"));
  q.bindValue(QSL(":name"), probe->title());
  q.bindValue(QSL(":type"), int(probe->type()));
  q.bindValue(QSL(":fltr"), probe->filter());
  q.bindValue(QSL(":icon"), IconFactory::toByteArray(probe->icon()));
  q.bindValue(QSL(":account_id"), account_id);

  q.exec();

  probe->setId(q.lastInsertId().toInt());
  probe->setCustomId(QString::number(probe->id()));
}

QList<Search*> DatabaseQueries::getProbesForAccount(const QSqlDatabase& db, int account_id) {
  QList<Search*> probes;
  SqlQuery q(db);

  q.prepare(QSL("SELECT * FROM Probes WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  q.exec();

  while (q.next()) {
    Search* prob = new Search(q.value(QSL("name")).toString(),
                              Search::Type(q.value(QSL("type")).toInt()),
                              q.value(QSL("fltr")).toString(),
                              IconFactory::fromByteArray(q.value(QSL("icon")).toByteArray()));

    prob->setId(q.value(QSL("id")).toInt());
    prob->setCustomId(QString::number(prob->id()));

    probes << prob;
  }

  return probes;
}

void DatabaseQueries::deleteProbe(const QSqlDatabase& db, Search* probe) {
  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM Probes WHERE id = :id AND account_id = :account_id;"));
  q.bindValue(QSL(":id"), probe->id());
  q.bindValue(QSL(":account_id"), probe->account()->accountId());

  q.exec();
}

QStringList DatabaseQueries::customIdsOfMessagesByCondition(const QSqlDatabase& db,
                                                            const QString& where_clause,
                                                            const QVariantMap& bindings) {
  QString sql = QSL("SELECT custom_id FROM Messages WHERE %1;").arg(where_clause);
  SqlQuery q(db);

  q.prepare(sql);

  for (auto it = bindings.constBegin(); it != bindings.constEnd(); ++it) {
    q.bindValue(it.key(), it.value());
  }

  q.exec();

  QStringList ids;

  while (q.next()) {
    ids << q.value(0).toString();
  }

  return ids;
}

void DatabaseQueries::markMessagesByCondition(const QSqlDatabase& db,
                                              const QString& where_clause,
                                              RootItem::ReadStatus read) {
  SqlQuery q(db);

  QString sql = QSL("UPDATE Messages SET is_read = :read WHERE %1;").arg(where_clause);

  q.prepare(sql);
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);
  q.exec();
}

void DatabaseQueries::markProbeReadUnread(const QSqlDatabase& db, Search* probe, RootItem::ReadStatus read) {
  markMessagesByCondition(db, whereClauseProbe(probe, probe->account()->accountId()), read);
}

void DatabaseQueries::markAllLabelledMessagesReadUnread(const QSqlDatabase& db,
                                                        int account_id,
                                                        RootItem::ReadStatus read) {
  markMessagesByCondition(db, whereClauseLabels(account_id), read);
}

void DatabaseQueries::markLabelledMessagesReadUnread(const QSqlDatabase& db, Label* label, RootItem::ReadStatus read) {
  markMessagesByCondition(db, whereClauseLabel(label->id(), label->account()->accountId()), read);
}

void DatabaseQueries::markImportantMessagesReadUnread(const QSqlDatabase& db,
                                                      int account_id,
                                                      RootItem::ReadStatus read) {
  markMessagesByCondition(db, whereClauseImportantArticles(account_id), read);
}

void DatabaseQueries::markUnreadMessagesRead(const QSqlDatabase& db, int account_id) {
  markMessagesByCondition(db, whereClauseUnreadArticles(account_id), RootItem::ReadStatus::Read);
}

void DatabaseQueries::markMessagesReadUnread(const QSqlDatabase& db,
                                             const QStringList& ids,
                                             RootItem::ReadStatus read) {
  markMessagesByCondition(db, QSL("id IN (%1)").arg(ids.join(QSL(", "))), read);
}

void DatabaseQueries::markFeedsReadUnread(const QSqlDatabase& db, const QStringList& ids, RootItem::ReadStatus read) {
  markMessagesByCondition(db, whereClauseFeeds(ids), read);
}

void DatabaseQueries::markBinReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read) {
  markMessagesByCondition(db, whereClauseBin(account_id), read);
}

void DatabaseQueries::markAccountReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read) {
  markMessagesByCondition(db, whereClauseAccount(true, account_id), read);
}

void DatabaseQueries::markMessageImportant(const QSqlDatabase& db, int id, RootItem::Importance importance) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE Messages SET is_important = :important WHERE id = :id;"));
  q.bindValue(QSL(":id"), id);
  q.bindValue(QSL(":important"), (int)importance);

  q.exec();
}

void DatabaseQueries::switchMessagesImportance(const QSqlDatabase& db, const QStringList& ids) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE Messages SET is_important = NOT is_important WHERE id IN (%1);").arg(ids.join(QSL(", "))));

  q.exec();
}

void DatabaseQueries::permanentlyDeleteMessages(const QSqlDatabase& db, const QStringList& ids) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE id IN (%1);").arg(ids.join(QSL(", "))));

  q.exec();
}

void DatabaseQueries::deleteOrRestoreMessagesToFromBin(const QSqlDatabase& db, const QStringList& ids, bool deleted) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE Messages SET is_deleted = %2, is_pdeleted = %3 WHERE id IN (%1);")
              .arg(ids.join(QSL(", ")), QString::number(deleted ? 1 : 0), QString::number(0)));

  q.exec();
}

void DatabaseQueries::restoreBin(const QSqlDatabase& db, int account_id) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE Messages SET is_deleted = 0 WHERE %1;").arg(whereClauseBin(account_id)));

  q.exec();
}

QList<int> DatabaseQueries::removeUnwantedArticlesFromFeed(const QSqlDatabase& db,
                                                           const Feed* feed,
                                                           const Feed::ArticleIgnoreLimit& feed_setup,
                                                           const Feed::ArticleIgnoreLimit& app_setup) {
  if (!feed->account()->messageStoragePolicy().m_useArticleCleanup) {
    return {};
  }
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
    return {};
  }

  SqlQuery q(db);
  QList<int> removed_ids;
  QStringList removed_ids_set;

  // First we get precise set of IDs of articles to be removed.
  if (recycle_dont_purge) {
    q.prepare(QSL("SELECT Messages.id "
                  "FROM Messages "
                  "WHERE "
                  "  Messages.feed = :feed AND "
                  "  Messages.is_deleted = 0 AND "
                  "  Messages.is_pdeleted = 0 AND "
                  "  Messages.is_important != :is_important AND "
                  "  Messages.is_read != :is_read "
                  "ORDER BY Messages.date_created DESC "
                  "LIMIT 2000000000 OFFSET :offset;"));
  }
  else {
    q.prepare(QSL("SELECT Messages.id "
                  "FROM Messages "
                  "WHERE "
                  "  Messages.feed = :feed AND "
                  "  (Messages.is_deleted = 1 OR Messages.is_important != :is_important) AND "
                  "  (Messages.is_deleted = 1 OR Messages.is_read != :is_read) "
                  "ORDER BY Messages.is_deleted ASC, Messages.date_created DESC "
                  "LIMIT 2000000000 OFFSET :offset;"));
  }

  q.bindValue(QSL(":feed"), feed->id());
  q.bindValue(QSL(":is_important"), dont_remove_starred ? 1 : 2);
  q.bindValue(QSL(":is_read"), dont_remove_unread ? 0 : 2);
  q.bindValue(QSL(":offset"), amount_to_keep);

  q.exec();

  while (q.next()) {
    removed_ids.append(q.value(0).toInt());
    removed_ids_set.append(q.value(0).toString());
  }

  q.finish();

  if (removed_ids.isEmpty()) {
    return removed_ids;
  }

  // Then we remove those articles (either purging them or just marking as deleted).
  if (recycle_dont_purge) {
    // We mark all older articles as deleted.
    q.prepare(QSL("UPDATE Messages "
                  "SET is_deleted = 1 "
                  "WHERE Messages.id IN (%1);")
                .arg(removed_ids_set.join(QSL(", "))));
  }
  else {
    // We purge all older articles.
    q.prepare(QSL("DELETE FROM Messages "
                  "WHERE Messages.id IN (%1);")
                .arg(removed_ids_set.join(QSL(", "))));
  }

  q.exec();

  int rows_deleted = q.numRowsAffected();

  qDebugNN << LOGSEC_DB << "Feed cleanup has recycled/purged" << NONQUOTE_W_SPACE(rows_deleted)
           << "old articles from feed" << QUOTE_W_SPACE_DOT(feed->customId());

  return removed_ids;
}

void DatabaseQueries::purgeFeedArticles(const QSqlDatabase& db, const QList<Feed*>& feeds) {
  SqlQuery q(db);

  if (feeds.isEmpty()) {
    return;
  }

  auto feeds_ids = feeds.first()->account()->textualFeedIds(feeds);

  q.exec(QSL("DELETE FROM Messages WHERE feed IN (%1) AND is_important = 0;").arg(feeds_ids.join(QSL(", "))));
}

void DatabaseQueries::purgeMessagesByCondition(const QSqlDatabase& db, const QString& where_clause) {
  SqlQuery q(db);

  QString sql = QSL("DELETE FROM Messages WHERE %1;").arg(where_clause);

  q.prepare(sql);
  q.exec();
}

void DatabaseQueries::purgeMessage(const QSqlDatabase& db, int message_id) {
  purgeMessagesByCondition(db, QSL("id = %1").arg(message_id));
}

QString DatabaseQueries::articleCleanupCondition(const QSqlDatabase& db) {
  SqlQuery accounts(db);
  accounts.exec(QSL("SELECT id, custom_data FROM Accounts;"));
  QStringList protected_accounts;
  while (accounts.next()) {
    const QString serialized = accounts.value(1).toString();
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(serialized.toUtf8(), &error);
    if (!serialized.isEmpty() && (error.error != QJsonParseError::NoError || !document.isObject())) {
      // A damaged account may own a managed cache. Do not erase it while its policy is unreadable.
      protected_accounts.append(QString::number(accounts.value(0).toInt()));
      continue;
    }
    const auto data = document.object().toVariantHash();
    const auto policy = data.value(QSL("message_storage_policy")).toHash();
    if (!policy.value(QSL("use_article_cleanup"), true).toBool()) {
      protected_accounts.append(QString::number(accounts.value(0).toInt()));
    }
  }
  return protected_accounts.isEmpty() ? QString()
                                      : QSL(" AND account_id NOT IN (%1)").arg(protected_accounts.join(QL1C(',')));
}

void DatabaseQueries::purgeImportantMessages(const QSqlDatabase& db) {
  purgeMessagesByCondition(db, QSL("is_important = 1 AND is_deleted = 0") + articleCleanupCondition(db));
}

void DatabaseQueries::purgeReadMessages(const QSqlDatabase& db) {
  purgeMessagesByCondition(db,
                           QSL("is_important = 0 AND is_deleted = 0 AND is_read = 1") + articleCleanupCondition(db));
}

void DatabaseQueries::purgeOldMessages(const QSqlDatabase& db, int older_than_days) {
  const qint64 since_epoch = older_than_days == 0
                               ? QDateTime::currentDateTimeUtc().addYears(10).toMSecsSinceEpoch()
                               : QDateTime::currentDateTimeUtc().addDays(-older_than_days).toMSecsSinceEpoch();

  purgeMessagesByCondition(db,
                           QSL("is_important = 0 AND date_created < %1").arg(since_epoch) +
                             articleCleanupCondition(db));
}

void DatabaseQueries::purgeRecycleBin(const QSqlDatabase& db) {
  purgeMessagesByCondition(db, QSL("is_important = 0 AND is_deleted = 1") + articleCleanupCondition(db));
}

void DatabaseQueries::purgeLeftoverMessages(const QSqlDatabase& db, int account_id) {
  purgeMessagesByCondition(db,
                           QSL("feed NOT IN (SELECT id FROM Feeds WHERE account_id = %1) AND "
                               "account_id = %1")
                             .arg(account_id));
}

QMap<int, ArticleCounts> DatabaseQueries::getMessageCountsForFeeds(const QSqlDatabase& db, const QStringList& ids) {
  auto res = messageCountsByCondition(db, whereClauseFeeds(ids), true);
  return res;
}

QMap<int, ArticleCounts> DatabaseQueries::messageCountsByCondition(const QSqlDatabase& db,
                                                                   const QString& where_clause,
                                                                   bool group_by_feed,
                                                                   const QVariantMap& bindings) {
  SqlQuery q(db);

  q.prepare(QSL("SELECT feed, COUNT(*), SUM(NOT is_read) "
                "FROM Messages "
                "WHERE %1 %2;")
              .arg(where_clause, group_by_feed ? QSL("GROUP BY feed") : QString()));

  for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) {
    q.bindValue(it.key(), it.value());
  }

  q.exec();

  QMap<int, ArticleCounts> res;

  while (q.next()) {
    ArticleCounts res_one;

    res_one.m_total = q.value(1).toInt();
    res_one.m_unread = q.value(2).toInt();

    res.insert(q.value(0).toInt(), res_one);
  }

  return res;
}

ArticleCounts DatabaseQueries::getMessageCountsForFeed(const QSqlDatabase& db, int feed_id) {
  auto res = messageCountsByCondition(db, whereClauseFeeds({QString::number(feed_id)}), false);

  return res.first();
}

ArticleCounts DatabaseQueries::getImportantMessageCounts(const QSqlDatabase& db, int account_id) {
  auto res = messageCountsByCondition(db, whereClauseImportantArticles(account_id), false);

  return res.first();
}

ArticleCounts DatabaseQueries::getUnreadMessageCounts(const QSqlDatabase& db, int account_id) {
  auto res = messageCountsByCondition(db, whereClauseUnreadArticles(account_id), false);

  return res.first();
}

ArticleCounts DatabaseQueries::getMessageCountsForBin(const QSqlDatabase& db, int account_id) {
  auto res = messageCountsByCondition(db, whereClauseBin(account_id), false);

  return res.first();
}

ArticleCounts DatabaseQueries::getMessageCountsForLabel(const QSqlDatabase& db, Label* label, int account_id) {
  auto res = messageCountsByCondition(db, whereClauseLabel(label->id(), account_id), false);

  return res.first();
}

QMap<int, ArticleCounts> DatabaseQueries::getMessageCountsForAllLabels(const QSqlDatabase& db, int account_id) {
  SqlQuery q(db);

  q.prepare(QSL("SELECT lim.label, COUNT(*), SUM(NOT m.is_read) "
                "FROM LabelsInMessages lim "
                "JOIN Messages m "
                "  ON m.id = lim.message "
                "WHERE"
                "  m.account_id = :account_id AND m.is_deleted = 0 AND m.is_pdeleted = 0 "
                "GROUP BY lim.label;"));

  q.bindValue(QSL(":account_id"), account_id);
  q.exec();

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

QList<Message> DatabaseQueries::getUndeletedMessagesForFeed(const QSqlDatabase& db,
                                                            int feed_id,
                                                            const QHash<QString, Label*>& labels) {
  QList<Message> messages;
  SqlQuery q(db);

  q.prepare(QSL("SELECT %1 "
                "FROM Messages "
                "WHERE %2;")
              .arg(messageTableAttributes().join(QSL(", ")), whereClauseFeeds({QString::number(feed_id)})));
  q.exec();

  while (q.next()) {
    Message message = Message::fromSqlQuery(q, labels);
    messages.append(message);
  }

  return messages;
}

QList<Message> DatabaseQueries::getUndeletedMessagesForAccount(const QSqlDatabase& db,
                                                               int account_id,
                                                               const QHash<QString, Label*>& labels) {
  QList<Message> messages;
  SqlQuery q(db);

  q.prepare(QSL("SELECT %1 "
                "FROM Messages "
                "WHERE %2;")
              .arg(messageTableAttributes().join(QSL(", ")), whereClauseAccount(false, account_id)));
  q.exec();

  while (q.next()) {
    Message message = Message::fromSqlQuery(q, labels);
    messages.append(message);
  }

  return messages;
}

int DatabaseQueries::highestPrimaryIdFeeds(const QSqlDatabase& db) {
  SqlQuery q(db);

  q.exec(QSL("SELECT MAX(id) FROM Feeds;"));
  q.next();

  return q.value(0).isNull() ? 0 : q.value(0).toInt();
}

int DatabaseQueries::highestPrimaryIdLabels(const QSqlDatabase& db) {
  SqlQuery q(db);

  q.exec(QSL("SELECT MAX(id) FROM Labels;"));
  q.next();

  return q.value(0).isNull() ? 0 : q.value(0).toInt();
}

QStringList DatabaseQueries::bagOfMessages(const QSqlDatabase& db, ServiceRoot::BagOfMessages bag, const Feed* feed) {
  QStringList ids;
  SqlQuery q(db);
  QString query;

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
                "WHERE feed = :feed AND %1;")
              .arg(query));

  q.bindValue(QSL(":feed"), feed->id());

  q.exec();

  while (q.next()) {
    ids.append(q.value(0).toString());
  }

  return ids;
}

QHash<QString, QStringList> DatabaseQueries::bagsOfMessages(const QSqlDatabase& db, const QList<Label*>& labels) {
  QHash<QString, QStringList> ids;

  for (Label* lbl : labels) {
    QStringList ids_one_label = customIdsOfMessagesFromLabel(db, lbl, RootItem::ReadStatus::Unknown);

    ids.insert(lbl->customId(), ids_one_label);
  }

  return ids;
}

namespace {

  qint64 articleTimestamp(const QDateTime& date_time) {
    // Qt 5 leaves conversion of invalid dates undefined; SQL and preview use the same epoch fallback.
    return date_time.isValid() ? date_time.toMSecsSinceEpoch() : 0;
  }

  QString unnulifyString(const QString& str) {
    return str.isNull() ? QSL("") : str;
  }

  // A failed commit is never retried. Its caller only publishes rows after commit succeeds.
  class ArticleWriteTransaction {
      Q_DISABLE_COPY(ArticleWriteTransaction)

    public:
      explicit ArticleWriteTransaction(const QSqlDatabase& db) : m_db(db) {
        if (!m_db.transaction()) {
          THROW_EX(SqlException, m_db.lastError());
        }
      }

      ~ArticleWriteTransaction() {
        if (m_active) {
          m_db.rollback();
        }
      }

      bool rollback() {
        m_active = !m_db.rollback();
        if (m_active && m_db.driverName() == QL1S(APP_DB_SQLITE_DRIVER) && m_db.transaction()) {
          // SQLite can roll back the entire transaction on a statement error.
          // Starting a new one confirms the old one is gone before replaying its prefix.
          m_active = !m_db.rollback();
        }
        return !m_active;
      }

      void commit() {
        if (!m_db.commit()) {
          THROW_EX(SqlException, m_db.lastError());
        }
        m_active = false;
      }

    private:
      QSqlDatabase m_db;
      bool m_active = true;
  };

  int bindArticleFields(SqlQuery& query, int pos, const Message& message, int feed_id, const QString& enclosures) {
    query.bindValue(pos++, unnulifyString(message.m_title));
    query.bindValue(pos++, int(message.m_isRead));
    query.bindValue(pos++, int(message.m_isImportant));
    query.bindValue(pos++, int(message.m_isDeleted));
    query.bindValue(pos++, unnulifyString(message.m_url));
    query.bindValue(pos++, unnulifyString(message.m_author));
    query.bindValue(pos++, message.m_score);
    query.bindValue(pos++, articleTimestamp(message.m_created));
    query.bindValue(pos++, articleTimestamp(message.m_retrieved));
    query.bindValue(pos++, unnulifyString(message.m_contents));
    query.bindValue(pos++, enclosures);
    query.bindValue(pos++, feed_id);
    query.bindValue(pos++, unnulifyString(message.m_customData));
    return pos;
  }

  constexpr int maria_insert_group = 32;
  constexpr int insert_field_count = 15;

  qint64 mariaInsertPacketLimit(const QSqlDatabase& db, int count) {
    if (count < maria_insert_group || db.driverName() != QL1S(APP_DB_MYSQL_DRIVER) ||
        !db.driver()->hasFeature(QSqlDriver::PreparedQueries)) {
      return 0;
    }
    SqlQuery query(db);
    if (!query.exec(QSL("SELECT VERSION(), @@SESSION.max_allowed_packet"), false) || !query.next()) {
      return 0;
    }
    QString version = query.value(0).toString();
    if (version.startsWith(QL1S("5.5.5-"))) {
      version.remove(0, 6); // MariaDB's legacy compatibility prefix.
    }
    if (!version.contains(QL1S("MariaDB"), Qt::CaseInsensitive) ||
        QVersionNumber::fromString(version) < QVersionNumber(10, 6, 1)) {
      return 0;
    }
    return query.value(1).toLongLong();
  }

  // QMYSQL does not expose the result columns of a prepared INSERT ... RETURNING.
  // Bind the data on the writer connection, then fetch the IDs with a direct statement.
  class MariaInsertGroups {
      Q_DISABLE_COPY(MariaInsertGroups)

    public:
      MariaInsertGroups(const QSqlDatabase& db, const QString& insert, qint64 packet_limit)
        : m_db(db), m_insert(insert), m_packetLimit(packet_limit), m_set(db) {}

      ~MariaInsertGroups() {
        if (!m_clear.isEmpty()) {
          SqlQuery cleanup(m_db);
          if (!cleanup.exec(m_clear, false)) {
            qWarningNN << LOGSEC_DB << "Could not clear staged article data:" << cleanup.lastError().text();
            cleanup.clear();
            m_set.clear();
            m_db.close(); // Session variables must not retain the last article payloads.
          }
        }
      }

      int prepare(const QList<Message>& messages, const QList<int>& indices, int start) {
        m_enclosures.clear();
        // Allow for the command header, column list and RETURNING clause. Per field,
        // 32 bytes covers a generated variable assignment (including cleanup), or
        // the type, length, null-bit and numeric value in the binary SET packet.
        qint64 remaining = m_packetLimit - 256;
        const int end = std::min(start + maria_insert_group, int(indices.size()));
        for (int i = start; i < end; ++i) {
          const Message& msg = messages.at(indices.at(i));
          const QString enclosures = Enclosures::encodeEnclosuresToString(msg.m_enclosures);
          qint64 bytes = insert_field_count * 32;
          for (const QString* field : {&msg.m_title, &msg.m_url, &msg.m_author, &msg.m_contents, &enclosures,
                                       &msg.m_customData, &msg.m_customId}) {
            bytes += field->toUtf8().size();
          }
          if (bytes > remaining) {
            break;
          }
          remaining -= bytes;
          m_enclosures.append(enclosures);
        }
        const int rows = int(m_enclosures.size());
        if (rows < 2) {
          return 1; // Large individual rows and single tails use the portable statement.
        }
        if (rows != m_preparedRows) {
          QStringList assignments, tuples, clear;
          for (int row = 0; row < rows; ++row) {
            QStringList vars;
            for (int field = 0; field < insert_field_count; ++field) {
              const QString var = QSL("@rssguard_value_%1").arg(row * insert_field_count + field);
              vars.append(var);
              assignments.append(var + QL1S("=?"));
              clear.append(var + QL1S("=NULL"));
            }
            tuples.append(QSL("(%1)").arg(vars.join(QChar(','))));
          }
          if (!m_set.prepare(QSL("SET ") + assignments.join(QChar(',')))) {
            THROW_EX(SqlException, m_set.lastError());
          }
          m_returning = m_insert + tuples.join(QChar(',')) + QL1S(" RETURNING id, ROWNUM()");
          if (rows > m_maxRows) {
            m_clear = QSL("SET ") + clear.join(QChar(','));
            m_maxRows = rows;
          }
          m_preparedRows = rows;
        }
        return rows;
      }

      SqlQuery& stagingQuery() { return m_set; }
      const QString& enclosures(int row) const { return m_enclosures.at(row); }

      void collectIds(QList<int>& ids) {
        SqlQuery returned(m_db);
        returned.exec(m_returning);
        QVector<int> group_ids(m_preparedRows);
        while (returned.next()) {
          const int id = returned.value(0).toInt();
          const int row = returned.value(1).toInt();
          if (id <= 0 || row < 1 || row > m_preparedRows || group_ids.at(row - 1) != 0) {
            throw ApplicationException(QObject::tr("Could not obtain the inserted article ID."));
          }
          group_ids[row - 1] = id;
        }
        if (returned.lastError().isValid()) {
          THROW_EX(SqlException, returned.lastError());
        }
        if (group_ids.contains(0)) {
          throw ApplicationException(QObject::tr("Could not obtain the inserted article ID."));
        }
        for (int id : std::as_const(group_ids)) {
          ids.append(id);
        }
      }

    private:
      QSqlDatabase m_db;
      QString m_insert;
      qint64 m_packetLimit;
      SqlQuery m_set;
      QStringList m_enclosures;
      QString m_returning;
      QString m_clear;
      int m_preparedRows = 0;
      int m_maxRows = 0;
  };

  struct ExistingMessageInfo {
      int m_id;
      qint64 m_created;
      bool m_isRead;
      bool m_isImportant;
      QString m_contents;
      int m_feedId;
      QString m_title;
      QString m_author;
      bool m_isPdeleted;
      QString m_customData;
  };

  struct MessageSavePolicy {
      bool m_syncable;
      bool m_ignoreContentsChanges;
      bool m_markUnreadOnUpdated;
      bool m_forceUpdate;
      int m_feedId;
      MessageStoragePolicy m_storage;
  };

  MessageSavePolicy messageSavePolicy(Settings* settings, Feed* feed, bool force_update) {
    return {feed->account()->isSyncable(),
            settings->value(GROUP(Messages), SETTING(Messages::IgnoreContentsChanges)).toBool(),
            settings->value(GROUP(Messages), SETTING(Messages::MarkUnreadOnUpdated)).toBool(),
            force_update,
            feed->id(),
            feed->account()->messageStoragePolicy()};
  }

  QHash<int, ExistingMessageInfo> findExistingMessages(const QSqlDatabase& db,
                                                       const QList<Message>& messages,
                                                       Feed* feed,
                                                       const MessageSavePolicy& policy) {
    const bool is_syncable = policy.m_syncable;
    const bool ignore_contents_changes = policy.m_ignoreContentsChanges;
    const int account_id = feed->account()->accountId();
    const int feed_id = feed->id();
    constexpr int lookup_batch_size = 200;
    QVector<int> direct_id_indices;
    QVector<int> custom_id_indices;
    QVector<int> anonymous_indices;

    direct_id_indices.reserve(messages.size());
    custom_id_indices.reserve(messages.size());
    anonymous_indices.reserve(messages.size());

    for (int i = 0; i < messages.size(); ++i) {
      const Message& message = messages.at(i);

      if (message.m_id > 0) {
        direct_id_indices.append(i);
      }
      else if (!message.m_customId.isEmpty()) {
        custom_id_indices.append(i);
      }
      else {
        anonymous_indices.append(i);
      }
    }

    QHash<int, ExistingMessageInfo> existing_messages;
    existing_messages.reserve(messages.size());

    const QString contents_column = ignore_contents_changes ? QSL("NULL") : QSL("Messages.contents");
    const QString lookup_columns =
      QSL("Messages.id, Messages.date_created, Messages.is_read, Messages.is_important, %1, "
          "Messages.feed, Messages.title, Messages.author, Messages.is_pdeleted, %2")
        .arg(contents_column, policy.m_storage.m_trackCustomDataChanges ? QSL("Messages.custom_data") : QSL("NULL"));
    const auto lookup_query = [&lookup_columns](const QStringList& requested_rows,
                                                const QString& join_condition,
                                                const QString& match_order = {}) {
      return QSL("SELECT requested.message_index, %1 "
                 "FROM (%2) AS requested "
                 "JOIN Messages ON Messages.id = ("
                 "  SELECT candidate.id FROM Messages AS candidate WHERE %3 %4 LIMIT 1"
                 ") "
                 "ORDER BY requested.message_index;")
        .arg(lookup_columns, requested_rows.join(QSL(" UNION ALL ")), join_condition, match_order);
    };

    const auto execute_lookup = [&existing_messages](SqlQuery& query,
                                                      const QString& lookup_description,
                                                      const QHash<int, QList<int>>* inputs_by_id = nullptr) {
      if (!query.exec(false)) {
        const QSqlError lookup_error = query.lastError();

        if (lookup_error.isValid()) {
          qWarningNN << LOGSEC_DB << "Failed to batch-check existing articles by" << lookup_description << ":"
                     << QUOTE_W_SPACE_DOT(lookup_error.text());
        }

        query.finish();

        if (lookup_error.isValid()) {
          THROW_EX(SqlException, lookup_error);
        }

        return;
      }

      while (query.next()) {
        // Direct lookups return a database ID; other lookups return an input index.
        const int lookup_key = query.value(0).toInt();

        // Match the first database row, like the original individual SELECT queries did.
        if (inputs_by_id == nullptr && existing_messages.contains(lookup_key)) {
          continue;
        }

        ExistingMessageInfo existing_message;

        existing_message.m_id = query.value(1).toInt();
        existing_message.m_created = query.value(2).value<qint64>();
        existing_message.m_isRead = query.value(3).toBool();
        existing_message.m_isImportant = query.value(4).toBool();
        existing_message.m_contents = query.value(5).toString();
        existing_message.m_feedId = query.value(6).toInt();
        existing_message.m_title = query.value(7).toString();
        existing_message.m_author = query.value(8).toString();
        existing_message.m_isPdeleted = query.value(9).toBool();
        existing_message.m_customData = query.value(10).toString();

        if (inputs_by_id == nullptr) {
          existing_messages.insert(lookup_key, existing_message);
        }
        else {
          // Repeated IDs still have independent decisions and receipts for every input.
          for (int message_index : inputs_by_id->value(lookup_key)) {
            if (!existing_messages.contains(message_index)) {
              existing_messages.insert(message_index, existing_message);
            }
          }
        }
      }

      const QSqlError lookup_error = query.lastError();

      query.finish();

      if (lookup_error.isValid()) {
        qWarningNN << LOGSEC_DB << "Failed while retrieving existing articles by" << lookup_description << ":"
                   << QUOTE_W_SPACE_DOT(lookup_error.text());
        THROW_EX(SqlException, lookup_error);
      }
    };

    for (int batch_start = 0; batch_start < direct_id_indices.size(); batch_start += lookup_batch_size) {
      const int batch_end = std::min(batch_start + lookup_batch_size, int(direct_id_indices.size()));
      QHash<int, QList<int>> inputs_by_id;
      QStringList ids;
      inputs_by_id.reserve(batch_end - batch_start);
      ids.reserve(batch_end - batch_start);

      for (int i = batch_start; i < batch_end; ++i) {
        const int message_index = direct_id_indices.at(i);
        const int id = messages.at(message_index).m_id;
        if (!inputs_by_id.contains(id)) {
          ids.append(QString::number(id));
        }
        inputs_by_id[id].append(message_index);
      }

      SqlQuery query(db);
      // IDs are positive integers, and the first column keeps the shared decoder's layout.
      query.prepare(QSL("SELECT Messages.id, %1 FROM Messages WHERE Messages.id IN (%2);")
                      .arg(lookup_columns, ids.join(QL1C(','))));
      execute_lookup(query, QSL("primary ID"), &inputs_by_id);
    }

    for (int batch_start = 0; batch_start < custom_id_indices.size(); batch_start += lookup_batch_size) {
      const int batch_end = std::min(batch_start + lookup_batch_size, int(custom_id_indices.size()));
      QStringList requested_rows;

      requested_rows.reserve(batch_end - batch_start);

      for (int i = batch_start; i < batch_end; ++i) {
        const int placeholder = i - batch_start;

        requested_rows.append(QSL("SELECT CAST(:message_index_%1 AS SIGNED INTEGER) AS message_index, "
                                  ":custom_id_%1 AS custom_id")
                                .arg(placeholder));
      }

      const QString scope_condition =
        is_syncable ? QSL("candidate.account_id = :scope_id") : QSL("candidate.feed = :scope_id");
      SqlQuery query(db);

      query.prepare(lookup_query(requested_rows,
                                 QSL("candidate.custom_id = requested.custom_id AND %1").arg(scope_condition)));
      query.bindValue(QSL(":scope_id"), is_syncable ? account_id : feed_id);

      for (int i = batch_start; i < batch_end; ++i) {
        const int placeholder = i - batch_start;
        const int message_index = custom_id_indices.at(i);

        query.bindValue(QSL(":message_index_%1").arg(placeholder), message_index);
        query.bindValue(QSL(":custom_id_%1").arg(placeholder), unnulifyString(messages.at(message_index).m_customId));
      }

      execute_lookup(query, is_syncable ? QSL("account-wide custom ID") : QSL("feed-specific custom ID"));
    }

    for (int batch_start = 0; batch_start < anonymous_indices.size(); batch_start += lookup_batch_size) {
      const int batch_end = std::min(batch_start + lookup_batch_size, int(anonymous_indices.size()));
      QStringList requested_rows;

      requested_rows.reserve(batch_end - batch_start);

      for (int i = batch_start; i < batch_end; ++i) {
        const int placeholder = i - batch_start;

        requested_rows.append(QSL("SELECT CAST(:message_index_%1 AS SIGNED INTEGER) AS message_index, "
                                  ":title_%1 AS title, :url_%1 AS url, :author_%1 AS author")
                                .arg(placeholder));
      }

      SqlQuery query(db);

      query.prepare(lookup_query(requested_rows,
                                 QSL("candidate.feed = :feed AND candidate.title = requested.title AND "
                                     "candidate.url = requested.url AND candidate.author = requested.author"),
                                 QSL("ORDER BY candidate.id")));
      query.bindValue(QSL(":feed"), feed_id);

      for (int i = batch_start; i < batch_end; ++i) {
        const int placeholder = i - batch_start;
        const int message_index = anonymous_indices.at(i);
        const Message& message = messages.at(message_index);

        query.bindValue(QSL(":message_index_%1").arg(placeholder), message_index);
        query.bindValue(QSL(":title_%1").arg(placeholder), unnulifyString(message.m_title));
        query.bindValue(QSL(":url_%1").arg(placeholder), unnulifyString(message.m_url));
        query.bindValue(QSL(":author_%1").arg(placeholder), unnulifyString(message.m_author));
      }

      execute_lookup(query, QSL("feed, title, URL and author"));
    }

    qDebugNN << LOGSEC_DB << "Batch article lookup matched" << NONQUOTE_W_SPACE(existing_messages.size()) << "of"
             << NONQUOTE_W_SPACE(messages.size()) << "incoming articles.";
    return existing_messages;
  }

  MessageSaveDecision decideMessageSave(const Message& incoming,
                                        const ExistingMessageInfo* stored,
                                        const MessageSavePolicy& policy) {
    if (stored == nullptr) {
      return incoming.m_title.isEmpty()
               ? MessageSaveDecision{MessageSaveAction::Skip, MessageSaveReason::InvalidArticle}
               : MessageSaveDecision{MessageSaveAction::Insert, MessageSaveReason::NewArticle};
    }
    if (stored->m_isPdeleted) {
      return {MessageSaveAction::Skip, MessageSaveReason::PermanentlyDeleted};
    }

    // These are the existing update conditions, shared by preview and persistence.
    const bool body_changed = !policy.m_ignoreContentsChanges && incoming.m_contents != stored->m_contents;
    const bool preserve_local = policy.m_storage.m_preserveLocalFields && !incoming.m_overwrite;
    const bool metadata_changed =
      policy.m_storage.m_trackCustomDataChanges && incoming.m_customData != stored->m_customData;
    const bool synchronized_update = !incoming.m_customId.isEmpty() && policy.m_syncable &&
                                     (articleTimestamp(incoming.m_created) != stored->m_created ||
                                      (!preserve_local && (incoming.m_isRead != stored->m_isRead ||
                                                           incoming.m_isImportant != stored->m_isImportant)) ||
                                      (incoming.m_feedId != stored->m_feedId && incoming.m_feedId == policy.m_feedId) ||
                                      incoming.m_title != stored->m_title || body_changed);
    const bool local_content_update =
      !incoming.m_customId.isEmpty() && !policy.m_syncable &&
      (incoming.m_title != stored->m_title || incoming.m_author != stored->m_author || body_changed);
    const bool date_or_body_update =
      (incoming.m_createdFromFeed &&
       std::abs(articleTimestamp(incoming.m_created) - stored->m_created) > MSG_DATETIME_DIFF_THRESSHOLD) ||
      body_changed;

    if (!synchronized_update && !local_content_update && !date_or_body_update && !metadata_changed &&
        !policy.m_forceUpdate && !incoming.m_overwrite) {
      return {MessageSaveAction::Skip, MessageSaveReason::NoUpdateTrigger};
    }

    MessageSaveDecision result{MessageSaveAction::Update,
                               incoming.m_overwrite   ? MessageSaveReason::ExplicitOverwrite
                               : policy.m_forceUpdate ? MessageSaveReason::ForcedUpdate
                                                      : MessageSaveReason::OrdinaryUpdate};
    result.m_preserveReadState =
      preserve_local || (!incoming.m_overwrite && !policy.m_syncable && !policy.m_markUnreadOnUpdated);
    result.m_preserveImportantState =
      preserve_local || (!incoming.m_overwrite && !policy.m_syncable && !incoming.m_isImportant);
    return result;
  }

  Message prepareMessageForUpdate(const Message& incoming,
                                  const ExistingMessageInfo& stored,
                                  const MessageSaveDecision& decision) {
    Message result(incoming);
    result.m_id = stored.m_id;
    if (decision.m_preserveReadState) {
      result.m_isRead = stored.m_isRead;
    }
    if (decision.m_preserveImportantState) {
      result.m_isImportant = stored.m_isImportant;
    }
    return result;
  }

  QStringList messageIds(const QList<Message>& messages) {
    QSet<int> unique;
    QStringList ids;
    for (const auto& message : messages) {
      if (message.m_id > 0 && !unique.contains(message.m_id)) {
        unique.insert(message.m_id);
        ids.append(QString::number(message.m_id));
      }
    }
    return ids;
  }

  QHash<int, QSet<int>> readMessageLabels(const QSqlDatabase& db, const QStringList& ids) {
    QHash<int, QSet<int>> result;
    constexpr int batch_size = 500;
    for (int offset = 0; offset < ids.size(); offset += batch_size) {
      SqlQuery query(db);
      query.exec(QSL("SELECT message, label FROM LabelsInMessages WHERE message IN (%1);")
                   .arg(ids.mid(offset, batch_size).join(QSL(","))));
      while (query.next()) {
        result[query.value(0).toInt()].insert(query.value(1).toInt());
      }
    }
    return result;
  }

  QSet<int> projectedMessageLabels(QSet<int> stored, const Message& incoming, bool online_labels) {
    if (online_labels) {
      stored.clear();
      for (Label* label : incoming.m_assignedLabels) {
        stored.insert(label->id());
      }
    }
    for (Label* label : incoming.m_assignedLabelsByFilter) {
      stored.insert(label->id());
    }
    for (Label* label : incoming.m_deassignedLabelsByFilter) {
      stored.remove(label->id());
    }
    return stored;
  }

  QHash<int, Message> readMessageSnapshots(const QSqlDatabase& db,
                                           const QStringList& ids,
                                           const QHash<int, Label*>& labels,
                                           const QHash<int, QSet<int>>& stored_labels) {
    QHash<int, Message> result;
    auto columns = DatabaseQueries::messageTableAttributes();
    // Labels were already read in bulk; fromSqlQuery receives no label map here.
    columns[MSG_DB_LABELS_IDS] = QSL("NULL AS msg_labels");
    constexpr int batch_size = 200;
    for (int offset = 0; offset < ids.size(); offset += batch_size) {
      SqlQuery query(db);
      query.exec(QSL("SELECT %1 FROM Messages WHERE Messages.id IN (%2);")
                   .arg(columns.join(QSL(", ")), ids.mid(offset, batch_size).join(QSL(","))));
      while (query.next()) {
        Message message = Message::fromSqlQuery(query, {});
        for (int id : stored_labels.value(message.m_id)) {
          if (labels.contains(id)) {
            message.m_assignedLabels.append(labels.value(id));
          }
        }
        result.insert(message.m_id, message);
      }
    }
    return result;
  }

  Message expectedStoredUpdate(const Message& incoming, const Message& stored, bool preserve_local) {
    // Overlay precisely the writable SQL fields. In particular, an existing
    // custom ID/account ID is not replaced merely because a script assigned it.
    Message result(stored);
    result.m_title = incoming.m_title;
    result.m_isRead = incoming.m_isRead;
    result.m_isImportant = incoming.m_isImportant;
    result.m_isDeleted = incoming.m_isDeleted;
    result.m_url = incoming.m_url;
    result.m_author = incoming.m_author;
    result.m_score = incoming.m_score;
    // Preview the SQL timestamp round trip, including invalid dates becoming
    // the epoch, without changing the incoming date or update detection.
    result.m_created = TextFactory::parseDateTime(articleTimestamp(incoming.m_created));
    result.m_retrieved = TextFactory::parseDateTime(articleTimestamp(incoming.m_retrieved));
    result.m_contents = incoming.m_contents;
    result.m_enclosures = incoming.m_enclosures;
    result.m_feedId = incoming.m_feedId;
    result.m_customData = incoming.m_customData;
    if (preserve_local) {
      result.m_isRead = stored.m_isRead;
      result.m_isImportant = stored.m_isImportant;
      result.m_isDeleted = stored.m_isDeleted;
      result.m_retrieved = stored.m_retrieved;
    }
    if (result.m_customId.isEmpty()) {
      result.m_customId = QString::number(result.m_id);
    }
    return result;
  }

} // namespace

QList<MessageSavePreview> DatabaseQueries::previewMessageUpdates(DatabaseFactory* db_factory,
                                                                 Settings* settings,
                                                                 const QList<Message>& messages,
                                                                 Feed* feed) {
  if (messages.isEmpty()) {
    return {};
  }
  const auto policy = messageSavePolicy(settings, feed, false);
  const bool online_labels =
    Globals::hasFlag(feed->account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Synchronised);
  QHash<int, Label*> labels;
  if (feed->account()->labelsNode() != nullptr) {
    for (Label* label : feed->account()->labelsNode()->labels()) {
      labels.insert(label->id(), label);
    }
  }
  for (const Message& message : messages) {
    for (const auto& list :
         {message.m_assignedLabels, message.m_assignedLabelsByFilter, message.m_deassignedLabelsByFilter}) {
      for (Label* label : list) {
        labels.insert(label->id(), label);
      }
    }
  }

  return db_factory->worker()->read<QList<MessageSavePreview>>([&](const QSqlDatabase& db) {
    auto existing = findExistingMessages(db, messages, feed, policy);
    QVector<MessageSaveDecision> decisions(messages.size());
    for (int index = 0; index < messages.size(); ++index) {
      const auto stored = existing.constFind(index);
      decisions[index] =
        decideMessageSave(messages.at(index), stored == existing.constEnd() ? nullptr : &stored.value(), policy);
    }
    // Decisions no longer need comparison bodies. Retain only the metadata needed
    // for preservation while the full expected-value snapshots are loaded.
    for (auto stored = existing.begin(); stored != existing.end(); ++stored) {
      stored->m_contents.clear();
    }
    QStringList ids;
    for (const auto& stored : existing) {
      ids.append(QString::number(stored.m_id));
    }
    ids.removeDuplicates();
    const auto stored_labels = readMessageLabels(db, ids);
    const auto snapshots = readMessageSnapshots(db, ids, labels, stored_labels);
    QList<MessageSavePreview> results;
    results.reserve(messages.size());

    for (int index = 0; index < messages.size(); ++index) {
      const Message& incoming = messages.at(index);
      const auto stored = existing.constFind(index);
      MessageSavePreview result;
      result.m_inputIndex = index;
      result.m_id = stored == existing.constEnd() ? 0 : stored->m_id;
      result.m_decision = decisions.at(index);
      result.m_hasStoredMessage = snapshots.contains(result.m_id);
      if (stored != existing.constEnd() && !result.m_hasStoredMessage) {
        // A record may disappear between the matching and snapshot queries.
        result.m_decision = {MessageSaveAction::Skip, MessageSaveReason::MissingArticle};
      }
      if (result.m_hasStoredMessage && snapshots.value(result.m_id).m_isPdeleted) {
        result.m_decision = {MessageSaveAction::Skip, MessageSaveReason::PermanentlyDeleted};
      }

      if (result.m_decision.m_action == MessageSaveAction::Update) {
        result.m_expectedStored =
          expectedStoredUpdate(prepareMessageForUpdate(incoming, stored.value(), result.m_decision),
                               snapshots.value(result.m_id),
                               policy.m_storage.m_preserveLocalFields && !incoming.m_overwrite);
      }
      else if (result.m_decision.m_action == MessageSaveAction::Insert) {
        result.m_expectedStored = incoming;
        // The generated database ID is unknown until insertion succeeds.
        result.m_expectedStored.m_id = 0;
        result.m_expectedStored.m_isPdeleted = false;
        result.m_expectedStored.m_feedId = feed->id();
        result.m_expectedStored.m_accountId = feed->account()->accountId();
        if (!result.m_expectedStored.m_retrieved.isValid()) {
          result.m_expectedStored.m_retrieved = QDateTime::currentDateTimeUtc();
        }
        result.m_expectedStored.m_created =
          TextFactory::parseDateTime(articleTimestamp(result.m_expectedStored.m_created));
        result.m_expectedStored.m_retrieved =
          TextFactory::parseDateTime(articleTimestamp(result.m_expectedStored.m_retrieved));
      }
      else if (result.m_hasStoredMessage) {
        result.m_expectedStored = snapshots.value(result.m_id);
      }
      // The existing final fixup also repairs active rows skipped by update detection.
      if (result.m_hasStoredMessage && !result.m_expectedStored.m_isPdeleted &&
          result.m_expectedStored.m_customId.isEmpty()) {
        result.m_expectedStored.m_customId = QString::number(result.m_id);
      }

      if (result.m_decision.m_reason != MessageSaveReason::PermanentlyDeleted &&
          result.m_decision.m_reason != MessageSaveReason::MissingArticle &&
          result.m_decision.m_reason != MessageSaveReason::InvalidArticle) {
        const QSet<int> before = stored_labels.value(result.m_id);
        const QSet<int> after = projectedMessageLabels(before, incoming, online_labels);
        result.m_labelsChanged = before != after;
        result.m_expectedStored.m_assignedLabels.clear();
        for (int id : after) {
          if (labels.contains(id)) {
            result.m_expectedStored.m_assignedLabels.append(labels.value(id));
          }
        }
      }
      results.append(result);
    }
    return results;
  });
}

MessageUpdateReport DatabaseQueries::updateMessages(DatabaseFactory* db_factory,
                                                    Settings* settings,
                                                    QList<Message>& messages,
                                                    Feed* feed,
                                                    bool force_update,
                                                    bool force_insert,
                                                    MessageUpdateDetail detail) {
  MessageUpdateReport report;
  if (messages.isEmpty()) {
    return report;
  }

  report.m_results.reserve(messages.size());
  for (int i = 0; i < messages.size(); ++i) {
    MessageWriteResult result;
    result.m_inputIndex = i;
    report.m_results.append(result);
  }

  const int account_id = feed->account()->accountId();
  const int feed_id = feed->id();
  QList<int> inserts;
  const auto storage_policy = feed->account()->messageStoragePolicy();
  QList<QPair<int, Message>> updates;
  QSet<int> skipped_message_ids;
  QHash<int, QSet<int>> labels_before;
  QSet<int> labels_processed;
  bool labels_before_loaded = false;
  const auto verify_label_changes = [&]() {
    const auto labels_after = db_factory->worker()->read<QHash<int, QSet<int>>>([&](const QSqlDatabase& db) {
      return readMessageLabels(db, messageIds(messages));
    });
    QSet<int> reported_changes;
    for (auto& receipt : report.m_results) {
      if (!labels_processed.contains(receipt.m_inputIndex)) {
        continue;
      }
      receipt.m_labelsChecked = true;
      receipt.m_labelsChanged = false;
      if (receipt.m_id > 0 && !skipped_message_ids.contains(receipt.m_id) &&
          labels_before.value(receipt.m_id) != labels_after.value(receipt.m_id) &&
          !reported_changes.contains(receipt.m_id)) {
        receipt.m_labelsChanged = true;
        reported_changes.insert(receipt.m_id);
      }
    }
  };
  int active_input = -1;
  QList<int> unconfirmed_batch;
  const auto confirm_write = [&](int index, Message& saved, MessageWriteOutcome outcome) {
    auto& receipt = report.m_results[index];
    receipt.m_id = saved.m_id;
    receipt.m_outcome = outcome;
    saved.m_insertedUpdated = true;
    messages[index].m_insertedUpdated = true;
    if (storage_policy.m_preserveLocalFields && !messages[index].m_overwrite) {
      messages[index].m_isRead = saved.m_isRead;
      messages[index].m_isImportant = saved.m_isImportant;
      messages[index].m_isDeleted = saved.m_isDeleted;
      messages[index].m_retrieved = saved.m_retrieved;
    }
    report.m_all.append(saved);
    if (!saved.m_isRead && !saved.m_isDeleted) {
      report.m_unread.append(saved);
    }
  };

  try {
    // Release the stored comparison bodies before writing the prepared articles.
    {
      const auto policy = messageSavePolicy(settings, feed, force_update);
      const auto existing = force_insert
                              ? QHash<int, ExistingMessageInfo>()
                              : db_factory->worker()->readForArticleUpdate<QHash<int, ExistingMessageInfo>>(
                                  [&](const QSqlDatabase& db) { return findExistingMessages(db, messages, feed, policy); });
      for (int index = 0; index < messages.size(); ++index) {
        Message& message = messages[index];
        auto& receipt = report.m_results[index];
        const auto stored = existing.constFind(index);
        receipt.m_decision =
          decideMessageSave(message, stored == existing.constEnd() ? nullptr : &stored.value(), policy);
        if (stored != existing.constEnd()) {
          message.m_id = stored->m_id;
          receipt.m_id = stored->m_id;
        }
        switch (receipt.m_decision.m_action) {
          case MessageSaveAction::Skip:
            receipt.m_outcome = MessageWriteOutcome::Skipped;
            if (receipt.m_decision.m_reason == MessageSaveReason::PermanentlyDeleted) {
              skipped_message_ids.insert(receipt.m_id);
            }
            break;
          case MessageSaveAction::Insert:
            inserts.append(index);
            break;
          case MessageSaveAction::Update: {
            Message prepared = prepareMessageForUpdate(message, stored.value(), receipt.m_decision);
            // Preserve the existing caller-visible adjustment of read state.
            message.m_isRead = prepared.m_isRead;
            updates.append(qMakePair(index, prepared));
            break;
          }
        }
      }
    }

    if (detail == MessageUpdateDetail::Detailed) {
      labels_before = db_factory->worker()->read<QHash<int, QSet<int>>>([&](const QSqlDatabase& db) {
        return readMessageLabels(db, messageIds(messages));
      });
      labels_before_loaded = true;
    }

    if (!updates.isEmpty()) {
      db_factory->worker()->write([&](const QSqlDatabase& db) {
        SqlQuery query_update(db);
        query_update.prepare(QSL("UPDATE Messages "
                                 "SET title = ?, is_read = ?, is_important = ?, is_deleted = ?, url = ?, "
                                 "author = ?, score = ?, date_created = ?, date_retrieved = ?, contents = ?, "
                                 "enclosures = ?, feed = ?, custom_data = ? WHERE id = ? AND is_pdeleted = 0;"));
        SqlQuery query_local(db);
        if (storage_policy.m_preserveLocalFields) {
          query_local.prepare(QSL("UPDATE Messages SET title = ?, url = ?, author = ?, score = ?, date_created = ?, "
                                  "contents = ?, enclosures = ?, feed = ?, custom_data = ? "
                                  "WHERE id = ? AND is_pdeleted = 0;"));
        }
        const auto write_update = [&](Message& message) -> std::optional<MessageSaveReason> {
          const bool preserve_local = storage_policy.m_preserveLocalFields && !message.m_overwrite;
          SqlQuery& query = preserve_local ? query_local : query_update;
          const QString enclosures = Enclosures::encodeEnclosuresToString(message.m_enclosures);
          int id_pos = 0;
          if (preserve_local) {
            query.bindValue(id_pos++, unnulifyString(message.m_title));
            query.bindValue(id_pos++, unnulifyString(message.m_url));
            query.bindValue(id_pos++, unnulifyString(message.m_author));
            query.bindValue(id_pos++, message.m_score);
            query.bindValue(id_pos++, articleTimestamp(message.m_created));
            query.bindValue(id_pos++, unnulifyString(message.m_contents));
            query.bindValue(id_pos++, enclosures);
            query.bindValue(id_pos++, message.m_feedId);
            query.bindValue(id_pos++, unnulifyString(message.m_customData));
          }
          else {
            id_pos = bindArticleFields(query, 0, message, message.m_feedId, enclosures);
          }
          query.bindValue(id_pos, message.m_id);
          query.exec();
          const bool check_match = query.numRowsAffected() == 0;
          query.finish();
          if (preserve_local) {
            SqlQuery current(db);
            current.prepare(QSL("SELECT is_read, is_important, is_deleted, date_retrieved, is_pdeleted "
                                "FROM Messages WHERE id = :id;"));
            current.bindValue(QSL(":id"), message.m_id);
            current.exec();
            if (!current.next()) {
              return MessageSaveReason::MissingArticle;
            }
            if (current.value(4).toBool()) {
              return MessageSaveReason::PermanentlyDeleted;
            }
            message.m_isRead = current.value(0).toBool();
            message.m_isImportant = current.value(1).toBool();
            message.m_isDeleted = current.value(2).toBool();
            message.m_retrieved = TextFactory::parseDateTime(current.value(3).toLongLong());
          }
          if (check_match) {
            // Some drivers also report zero affected rows for a successful no-op update.
            SqlQuery current(db);
            current.prepare(QSL("SELECT is_pdeleted FROM Messages WHERE id = :id;"));
            current.bindValue(QSL(":id"), message.m_id);
            current.exec();
            if (!current.next()) {
              return MessageSaveReason::MissingArticle;
            }
            if (current.value(0).toBool()) {
              return MessageSaveReason::PermanentlyDeleted;
            }
          }
          return std::nullopt;
        };
        const auto confirm_update = [&](int i, std::optional<MessageSaveReason> skipped) {
          auto& update = updates[i];
          if (skipped) {
            auto& receipt = report.m_results[update.first];
            receipt.m_outcome = MessageWriteOutcome::Skipped;
            receipt.m_decision = {MessageSaveAction::Skip, *skipped};
            skipped_message_ids.insert(update.second.m_id);
          }
          else {
            confirm_write(update.first, update.second, MessageWriteOutcome::Updated);
          }
        };

        constexpr int batch_size = 128;
        for (int start = 0; start < updates.size(); start += batch_size) {
          const int end = std::min(start + batch_size, int(updates.size()));
          if (end == start + 1) {
            active_input = updates.at(start).first;
            confirm_update(start, write_update(updates[start].second));
            active_input = -1;
            continue;
          }
          QList<std::optional<MessageSaveReason>> outcomes;
          outcomes.reserve(end - start);
          ArticleWriteTransaction transaction(db);
          try {
            for (int i = start; i < end; ++i) {
              active_input = updates.at(i).first;
              unconfirmed_batch.append(active_input);
              outcomes.append(write_update(updates[i].second));
            }
          }
          catch (const SqlException&) {
            // Restore the old per-row failure boundary, but only after a confirmed
            // rollback. Never replay the failed statement or an uncertain commit.
            const int failed_input = active_input;
            query_update.finish();
            if (transaction.rollback()) {
              unconfirmed_batch.clear();
              for (int i = 0; i < outcomes.size(); ++i) {
                auto& update = updates[start + i];
                active_input = update.first;
                confirm_update(start + i, write_update(update.second));
              }
              active_input = failed_input;
            }
            throw;
          }
          transaction.commit();
          for (int i = 0; i < outcomes.size(); ++i) {
            confirm_update(start + i, outcomes.at(i));
          }
          unconfirmed_batch.clear();
          active_input = -1;
        }
      });
    }

    if (!inserts.isEmpty()) {
      db_factory->worker()->write([&](const QSqlDatabase& db) {
        const QString insert = QSL("INSERT INTO Messages "
                                   "(title, is_read, is_important, is_deleted, url, author, score, date_created, "
                                   "date_retrieved, contents, enclosures, feed, custom_data, custom_id, account_id) "
                                   "VALUES ");
        std::optional<MariaInsertGroups> groups;
        if (const qint64 limit = mariaInsertPacketLimit(db, int(inserts.size())); limit > 0) {
          groups.emplace(db, insert, limit);
        }
        // Destroy the prepared statement before the group cleanup can close the session.
        SqlQuery single(db);
        if (!single.prepare(insert + QL1S("(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"))) {
          THROW_EX(SqlException, single.lastError());
        }

        // Preserve the former multi-row INSERT's atomic boundary. Collect actual
        // generated IDs so triggers and non-unit auto-increment steps are safe.
        constexpr int batch_size = 1000;
        for (int start = 0; start < inserts.size(); start += batch_size) {
          unconfirmed_batch = inserts.mid(start, batch_size);
          QList<int> ids;
          ids.reserve(unconfirmed_batch.size());
          const auto retrieval_time = QDateTime::currentDateTimeUtc();
          ArticleWriteTransaction transaction(db);
          for (int offset = 0; offset < unconfirmed_batch.size();) {
            const int rows = groups ? groups->prepare(messages, unconfirmed_batch, offset) : 1;
            SqlQuery& query = rows > 1 ? groups->stagingQuery() : single;
            int pos = 0;
            for (int row = 0; row < rows; ++row) {
              auto& message = messages[unconfirmed_batch.at(offset + row)];
              if (!message.m_retrieved.isValid()) {
                message.m_retrieved = retrieval_time;
              }
              pos = bindArticleFields(query, pos, message, feed_id,
                                      rows > 1 ? groups->enclosures(row)
                                               : Enclosures::encodeEnclosuresToString(message.m_enclosures));
              query.bindValue(pos++, unnulifyString(message.m_customId));
              query.bindValue(pos++, account_id);
            }
            query.exec();
            if (rows == 1) {
              bool valid_id;
              const int id = query.lastInsertId().toInt(&valid_id);
              if (query.numRowsAffected() != 1 || !valid_id || id <= 0) {
                throw ApplicationException(QObject::tr("Could not obtain the inserted article ID."));
              }
              ids.append(id);
            }
            query.finish();
            if (rows > 1) {
              groups->collectIds(ids);
            }
            offset += rows;
          }
          transaction.commit();
          for (int i = 0; i < ids.size(); ++i) {
            const int index = unconfirmed_batch.at(i);
            messages[index].m_id = ids.at(i);
            confirm_write(index, messages[index], MessageWriteOutcome::Inserted);
          }
          unconfirmed_batch.clear();
        }
      });
    }

    const bool uses_online_labels =
      Globals::hasFlag(feed->account()->supportedLabelOperations(), ServiceRoot::LabelOperation::Synchronised);
    for (int index = 0; index < messages.size(); ++index) {
      Message& message = messages[index];
      auto& receipt = report.m_results[index];
      if (skipped_message_ids.contains(message.m_id) ||
          receipt.m_decision.m_reason == MessageSaveReason::InvalidArticle) {
        labels_processed.insert(index);
        continue;
      }
      if (!message.m_customId.isEmpty() || message.m_id > 0) {
        active_input = index;
        if (uses_online_labels) {
          db_factory->worker()->write([&](const QSqlDatabase& db) {
            SqlQuery updatable(db);
            updatable.prepare(QSL("SELECT id FROM Messages WHERE id = :id AND is_pdeleted = 0;"));
            updatable.bindValue(QSL(":id"), message.m_id);
            updatable.exec();
            if (updatable.next()) {
              setLabelsForMessage(db, message.m_assignedLabels, message);
            }
          });
        }
        for (Label* label : std::as_const(message.m_assignedLabelsByFilter)) {
          label->assignToMessage(message, false, true);
        }
        for (Label* label : std::as_const(message.m_deassignedLabelsByFilter)) {
          label->deassignFromMessage(message, false, true);
        }
        active_input = -1;
      }
      labels_processed.insert(index);
    }

    if (detail == MessageUpdateDetail::Detailed) {
      verify_label_changes();
    }

    db_factory->worker()->write([&](const QSqlDatabase& db) {
      SqlQuery fixup(db);
      fixup.exec(QSL("UPDATE Messages SET custom_id = id "
                     "WHERE (custom_id IS NULL OR custom_id = '') AND is_pdeleted = 0;"));
    });
  }
  catch (...) {
    report.captureFailure();
    // Earlier label operations may have committed before a later operation failed.
    // Verify completed articles without masking the original failure. The current
    // article remains unconfirmed if only part of its label operations succeeded.
    if (labels_before_loaded && !labels_processed.isEmpty()) {
      try {
        verify_label_changes();
      }
      catch (...) {
        // No confirmed label result is available if verification itself fails.
      }
    }
    if (active_input >= 0) {
      auto& receipt = report.m_results[active_input];
      receipt.m_error = report.m_error;
      if (receipt.m_outcome == MessageWriteOutcome::NotAttempted) {
        receipt.m_outcome = MessageWriteOutcome::Failed;
      }
    }
    for (int index : unconfirmed_batch) {
      auto& receipt = report.m_results[index];
      if (receipt.m_outcome == MessageWriteOutcome::NotAttempted) {
        receipt.m_outcome = MessageWriteOutcome::Failed;
        receipt.m_error = report.m_error;
      }
    }
  }
  return report;
}

void DatabaseQueries::cleanBin(const QSqlDatabase& db, bool clear_only_read, int account_id) {
  SqlQuery q(db);

  if (clear_only_read) {
    q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE %1 AND is_read = 1;").arg(whereClauseBin(account_id)));
  }
  else {
    q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE %1;").arg(whereClauseBin(account_id)));
  }

  q.exec();
}

void DatabaseQueries::deleteAccount(const QSqlDatabase& db, ServiceRoot* account) {
  moveItem(account, false, true, {}, db);

  SqlQuery q(db);

  QStringList queries;

  // NOTE: We manually remove only from Accounts table, everything else just cascades.
  queries << QSL("DELETE FROM Accounts WHERE id = :account_id;");

  for (const QString& q_str : std::as_const(queries)) {
    q.prepare(q_str);
    q.bindValue(QSL(":account_id"), account->accountId());

    q.exec();
    q.finish();
  }
}

void DatabaseQueries::deleteAccountData(const QSqlDatabase& db,
                                        int account_id,
                                        bool delete_messages_too,
                                        bool delete_labels_too) {
  SqlQuery q(db);

  if (delete_messages_too) {
    q.prepare(QSL("DELETE FROM Messages WHERE account_id = :account_id;"));
    q.bindValue(QSL(":account_id"), account_id);

    q.exec();
  }

  q.prepare(QSL("DELETE FROM Feeds WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  q.exec();

  q.prepare(QSL("DELETE FROM Categories WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  q.exec();

  if (delete_labels_too) {
    q.prepare(QSL("DELETE FROM Labels WHERE account_id = :account_id;"));
    q.bindValue(QSL(":account_id"), account_id);

    q.exec();
  }
}

void DatabaseQueries::cleanMessagesByCondition(const QSqlDatabase& db,
                                               const QString& where_clause,
                                               bool clean_read_only) {
  SqlQuery q(db);

  QString sql = QSL("UPDATE Messages SET is_deleted = 1 WHERE (%1)").arg(where_clause);

  if (clean_read_only) {
    sql += QSL(" AND is_read = 1");
  }

  sql += QL1C(';');

  q.prepare(sql);
  q.exec();
}

void DatabaseQueries::cleanImportantMessages(const QSqlDatabase& db, bool clean_read_only, int account_id) {
  cleanMessagesByCondition(db, whereClauseImportantArticles(account_id), clean_read_only);
}

void DatabaseQueries::cleanUnreadMessages(const QSqlDatabase& db, int account_id) {
  cleanMessagesByCondition(db, whereClauseUnreadArticles(account_id), false);
}

void DatabaseQueries::cleanFeeds(const QSqlDatabase& db, const QStringList& feed_ids, bool clean_read_only) {
  if (feed_ids.isEmpty()) {
    return;
  }

  cleanMessagesByCondition(db, whereClauseFeeds(feed_ids), clean_read_only);
}

void DatabaseQueries::cleanLabelledMessages(const QSqlDatabase& db, Label* label, bool clean_read_only) {
  cleanMessagesByCondition(db, whereClauseLabel(label->id(), label->account()->accountId()), clean_read_only);
}

void DatabaseQueries::cleanProbedMessages(const QSqlDatabase& db, Search* probe, bool clean_read_only) {
  cleanMessagesByCondition(db, whereClauseProbe(probe, probe->account()->accountId()), clean_read_only);
}

void DatabaseQueries::purgeLeftoverMessageFilterAssignments(const QSqlDatabase& db) {
  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM MessageFiltersInFeeds "
                "WHERE feed NOT IN (SELECT id FROM Feeds);"));
  q.exec();
}

void DatabaseQueries::purgeLeftoverLabelAssignments(const QSqlDatabase& db) {
  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM LabelsInMessages "
                "WHERE "
                "  label NOT IN (SELECT id FROM Labels) OR "
                "  message NOT IN (SELECT id FROM Messages);"));
  q.exec();
}

void DatabaseQueries::storeAccountTree(IconFactory* icons,
                                       const QSqlDatabase& db,
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

        createLabel(icons, db, label, account_id, new_lbl_id);
      }
    }
  }
}

QStringList DatabaseQueries::customIdsOfMessagesFromLabels(const QSqlDatabase& db,
                                                           RootItem::ReadStatus target_read,
                                                           int account_id) {
  QString cond = whereClauseLabel(0, account_id);
  QMap<QString, QVariant> bindings;

  if (target_read != RootItem::ReadStatus::Unknown) {
    cond += QSL(" AND Messages.is_read = :read");
    bindings[QSL(":read")] = target_read == RootItem::ReadStatus::Read ? 0 : 1;
  }

  return customIdsOfMessagesByCondition(db, cond, bindings);
}

QStringList DatabaseQueries::customIdsOfMessagesFromLabel(const QSqlDatabase& db,
                                                          Label* label,
                                                          RootItem::ReadStatus read) {
  QString cond = whereClauseLabel(label->id(), label->account()->accountId());
  QMap<QString, QVariant> bindings;

  if (read != RootItem::ReadStatus::Unknown) {
    cond += QSL(" AND Messages.is_read = :read");
    bindings[QSL(":read")] = read == RootItem::ReadStatus::Read ? 0 : 1;
  }

  return customIdsOfMessagesByCondition(db, cond, bindings);
}

QStringList DatabaseQueries::customIdsOfMessagesFromFeeds(const QSqlDatabase& db,
                                                          const QStringList& ids,
                                                          RootItem::ReadStatus read) {
  QString cond = QSL("%1 AND Messages.is_read = :read").arg(whereClauseFeeds(ids));

  return customIdsOfMessagesByCondition(db, cond, {{QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfMessagesFromAccount(const QSqlDatabase& db,
                                                            RootItem::ReadStatus read,
                                                            int account_id) {
  QString cond = QSL("%1 AND Messages.is_read = :read").arg(whereClauseAccount(false, account_id));

  return customIdsOfMessagesByCondition(db, cond, {{QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfImportantMessages(const QSqlDatabase& db,
                                                          RootItem::ReadStatus read,
                                                          int account_id) {
  QString cond = QSL("%1 AND Messages.is_read = :read").arg(whereClauseImportantArticles(account_id));

  return customIdsOfMessagesByCondition(db, cond, {{QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfUnreadMessages(const QSqlDatabase& db, int account_id) {
  QString cond = whereClauseUnreadArticles(account_id);

  return customIdsOfMessagesByCondition(db, cond);
}

QStringList DatabaseQueries::customIdsOfMessagesFromBin(const QSqlDatabase& db,
                                                        RootItem::ReadStatus read,
                                                        int account_id) {
  QString cond = QSL("%1 AND Messages.is_read = :read").arg(whereClauseBin(account_id));

  return customIdsOfMessagesByCondition(db, cond, {{QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

QStringList DatabaseQueries::customIdsOfMessagesFromProbe(const QSqlDatabase& db,
                                                          Search* probe,
                                                          RootItem::ReadStatus read) {
  QString cond = QSL("%1 AND Messages.is_read = :read").arg(whereClauseProbe(probe, probe->account()->accountId()));

  return customIdsOfMessagesByCondition(db, cond, {{QSL(":read"), read == RootItem::ReadStatus::Read ? 0 : 1}});
}

void DatabaseQueries::createOverwriteCategory(const QSqlDatabase& db,
                                              Category* category,
                                              int account_id,
                                              int new_parent_id) {
  SqlQuery q(db);
  int next_sort_order;

  if (category->id() <= 0 || (category->parent() != nullptr && category->parent()->id() != new_parent_id)) {
    q.prepare(QSL("SELECT MAX(ordr) FROM Categories WHERE account_id = :account_id AND parent_id = :parent_id;"));
    q.bindValue(QSL(":account_id"), account_id);
    q.bindValue(QSL(":parent_id"), new_parent_id);

    q.exec();

    if (!q.next()) {
      THROW_EX(SqlException, q.lastError());
    }

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

    q.exec();
    category->setId(q.lastInsertId().toInt());
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
            "SET "
            "parent_id = :parent_id, "
            "ordr = :ordr, "
            "title = :title, "
            "description = :description, "
            "date_created = :date_created, "
            "icon = :icon, "
            "account_id = :account_id, "
            "custom_id = :custom_id, "
            "custom_data = :custom_data "
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

  auto custom_data = category->customDatabaseData();
  QString serialized_custom_data = serializeCustomData(custom_data);

  q.bindValue(QSL(":custom_data"), serialized_custom_data);

  q.exec();
}

void DatabaseQueries::createOverwriteFeed(const QSqlDatabase& db,
                                          Feed* feed,
                                          int account_id,
                                          int new_parent_id,
                                          int new_feed_id) {
  SqlQuery q(db);
  int next_sort_order;

  if (feed->id() <= 0 || (feed->parent() != nullptr && feed->parent()->id() != new_parent_id)) {
    // We either insert completely new feed or we move feed
    // to new parent. Get new viable sort order.
    q.prepare(QSL("SELECT MAX(ordr) FROM Feeds WHERE account_id = :account_id AND category = :category;"));
    q.bindValue(QSL(":account_id"), account_id);
    q.bindValue(QSL(":category"), new_parent_id);

    q.exec();

    if (!q.next()) {
      THROW_EX(SqlException, q.lastError());
    }

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

    q.exec();
    feed->setId(q.lastInsertId().toInt());

    if (feed->customId().isEmpty()) {
      feed->setCustomId(QString::number(feed->id()));
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
            "open_articles = :open_articles, "
            "exclude_global_unread = :exclude_global_unread, "
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
  q.bindValue(QSL(":open_articles"), feed->openArticlesDirectly());
  q.bindValue(QSL(":exclude_global_unread"), feed->excludesFromGlobalUnreadCount());
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

  q.exec();
}

void DatabaseQueries::updateFeedCommonSettings(const QSqlDatabase& db, const Feed* feed, int account_id) {
  SqlQuery query(db);
  query.prepare(QSL("UPDATE Feeds SET update_type = :update_type, update_interval = :update_interval, "
                    "is_off = :is_off, is_quiet = :is_quiet, is_rtl = :is_rtl, "
                    "add_any_datetime_articles = :add_any_datetime_articles, datetime_to_avoid = :datetime_to_avoid, "
                    "keep_article_customize = :keep_article_customize, keep_article_count = :keep_article_count, "
                    "keep_unread_articles = :keep_unread_articles, keep_starred_articles = :keep_starred_articles, "
                    "recycle_articles = :recycle_articles, open_articles = :open_articles, "
                    "exclude_global_unread = :exclude_global_unread WHERE id = :id AND account_id = :account;"));
  query.bindValue(QSL(":id"), feed->id());
  query.bindValue(QSL(":account"), account_id);
  query.bindValue(QSL(":update_type"), int(feed->autoUpdateType()));
  query.bindValue(QSL(":update_interval"), feed->autoUpdateInterval());
  query.bindValue(QSL(":is_off"), feed->isSwitchedOff());
  query.bindValue(QSL(":is_quiet"), feed->isQuiet());
  query.bindValue(QSL(":is_rtl"), int(feed->rtlBehavior()));
  query.bindValue(QSL(":open_articles"), feed->openArticlesDirectly());
  query.bindValue(QSL(":exclude_global_unread"), feed->excludesFromGlobalUnreadCount());
  const auto& limit = feed->articleIgnoreLimit();
  query.bindValue(QSL(":add_any_datetime_articles"), limit.m_addAnyArticlesToDb);
  query.bindValue(QSL(":datetime_to_avoid"),
                  limit.m_dtToAvoid.isValid() && limit.m_dtToAvoid.toMSecsSinceEpoch() > 0
                    ? limit.m_dtToAvoid.toMSecsSinceEpoch()
                    : limit.m_hoursToAvoid);
  query.bindValue(QSL(":keep_article_customize"), limit.m_customizeLimitting);
  query.bindValue(QSL(":keep_article_count"), limit.m_keepCountOfArticles);
  query.bindValue(QSL(":keep_unread_articles"), limit.m_doNotRemoveUnread);
  query.bindValue(QSL(":keep_starred_articles"), limit.m_doNotRemoveStarred);
  query.bindValue(QSL(":recycle_articles"), limit.m_moveToBinDontPurge);
  query.exec();
}

void DatabaseQueries::storeCustomData(const QSqlDatabase& db,
                                      const QString& table,
                                      int item_id,
                                      const QVariantHash& data) {
  QString serialized_custom_data = serializeCustomData(data);
  SqlQuery q(db);

  q.prepare(QSL("UPDATE %1 "
                "SET custom_data = :custom_data "
                "WHERE id = :id;")
              .arg(table));
  q.bindValue(QSL(":id"), item_id);
  q.bindValue(QSL(":custom_data"), serialized_custom_data);
  q.exec();
}

void DatabaseQueries::createOverwriteAccount(const QSqlDatabase& db, ServiceRoot* account) {
  createOverwriteAccountImpl(db, account, nullptr);
}

void DatabaseQueries::createOverwriteAccount(const QSqlDatabase& db,
                                             ServiceRoot* account,
                                             const QVariantHash& custom_data) {
  createOverwriteAccountImpl(db, account, &custom_data);
}

void DatabaseQueries::createOverwriteAccountImpl(const QSqlDatabase& db,
                                                 ServiceRoot* account,
                                                 const QVariantHash* custom_data) {
  SqlQuery q(db);

  if (account->accountId() <= 0) {
    // We need to insert account and generate sort order first.
    if (account->sortOrder() < 0) {
      q.exec(QSL("SELECT MAX(ordr) FROM Accounts;"));
      q.next();

      int next_order = (q.value(0).isNull() ? -1 : q.value(0).toInt()) + 1;

      account->setSortOrder(next_order);
      q.finish();
    }

    q.prepare(QSL("INSERT INTO Accounts (ordr, type) "
                  "VALUES (0, :type);"));
    q.bindValue(QSL(":type"), account->code());

    q.exec();
    account->setAccountId(q.lastInsertId().toInt());
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

  QString serialized_custom_data = serializeCustomData(custom_data == nullptr ? account->customDatabaseData() : *custom_data);

  q.bindValue(QSL(":custom_data"), serialized_custom_data);

  q.exec();
}

void DatabaseQueries::deleteFeed(const QSqlDatabase& db, Feed* feed, int account_id) {
  moveItem(feed, false, true, {}, db);

  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM Feeds WHERE id = :feed;"));
  q.bindValue(QSL(":feed"), feed->id());

  q.exec();
}

void DatabaseQueries::deleteCategory(const QSqlDatabase& db, Category* category) {
  moveItem(category, false, true, {}, db);

  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM Categories WHERE id = :category;"));
  q.bindValue(QSL(":category"), category->id());

  q.exec();
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
  int max_sort_order = qlinq::from(neighbors)
                         .max([=](RootItem* it) {
                           return it->kind() == item->kind() ? it->sortOrder() : 0;
                         })
                         .value_or(0);

  if ((!move_top && !move_bottom && item->sortOrder() == move_index) || /* Item is already sorted OK. */
      (!move_top && !move_bottom &&
       move_index < 0) || /* Order cannot be smaller than 0 if we do not move to begin/end. */
      (!move_top && !move_bottom && move_index > max_sort_order) || /* Cannot move past biggest sort order. */
      (move_top && item->sortOrder() == 0) ||                       /* Item is already on top. */
      (move_bottom && item->sortOrder() == max_sort_order) ||       /* Item is already on bottom. */
      max_sort_order <= 0) {                                        /* We only have 1 item, nothing to sort. */
    return;
  }

  SqlQuery q(db);

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
  q.exec();

  q.prepare(QSL("UPDATE %1 SET ordr = :ordr WHERE id = :id;").arg(table_name));
  q.bindValue(QSL(":id"),
              item->kind() == RootItem::Kind::ServiceRoot ? item->toServiceRoot()->accountId() : item->id());
  q.bindValue(QSL(":ordr"), move_index);
  q.exec();

  // Fix live sort orders.
  if (item->sortOrder() > move_index) {
    qlinq::from(neighbors)
      .where([=](RootItem* it) {
        return it->kind() == item->kind() && it->sortOrder() < move_high && it->sortOrder() >= move_low;
      })
      .for_each([](RootItem* it) {
        it->setSortOrder(it->sortOrder() + 1);
      });
  }
  else {
    qlinq::from(neighbors)
      .where([=](RootItem* it) {
        return it->kind() == item->kind() && it->sortOrder() > move_low && it->sortOrder() <= move_high;
      })
      .for_each([](RootItem* it) {
        it->setSortOrder(it->sortOrder() - 1);
      });
  }

  item->setSortOrder(move_index);
}

void DatabaseQueries::moveMessageFilter(const QList<MessageFilter*>& all_filters,
                                        MessageFilter* filter,
                                        bool move_top,
                                        bool move_bottom,
                                        int move_index,
                                        const QSqlDatabase& db) {
  int max_sort_order = qlinq::from(all_filters)
                         .max([=](MessageFilter* it) {
                           return it->sortOrder();
                         })
                         .value_or(0);

  if ((!move_top && !move_bottom && filter->sortOrder() == move_index) || /* Item is already sorted OK. */
      (!move_top && !move_bottom &&
       move_index < 0) || /* Order cannot be smaller than 0 if we do not move to begin/end. */
      (!move_top && !move_bottom && move_index > max_sort_order) || /* Cannot move past biggest sort order. */
      (move_top && filter->sortOrder() == 0) ||                     /* Item is already on top. */
      (move_bottom && filter->sortOrder() == max_sort_order) ||     /* Item is already on bottom. */
      max_sort_order <= 0) {                                        /* We only have 1 item, nothing to sort. */
    return;
  }

  SqlQuery q(db);

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

  q.exec();

  q.prepare(QSL("UPDATE MessageFilters SET ordr = :ordr WHERE id = :id;"));
  q.bindValue(QSL(":id"), filter->id());
  q.bindValue(QSL(":ordr"), move_index);

  q.exec();

  // Fix live sort orders.
  if (filter->sortOrder() > move_index) {
    qlinq::from(all_filters)
      .where([=](MessageFilter* it) {
        return it->sortOrder() < move_high && it->sortOrder() >= move_low;
      })
      .for_each([](MessageFilter* it) {
        it->setSortOrder(it->sortOrder() + 1);
      });
  }
  else {
    qlinq::from(all_filters)
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

  SqlQuery q(db);

  q.exec(QSL("SELECT COUNT(*) FROM MessageFilters;"));
  q.next();

  int new_ordr = q.value(0).toInt();

  q.prepare(QSL("INSERT INTO MessageFilters (name, script, ordr) VALUES (:name, :script, :ordr);"));
  q.bindValue(QSL(":name"), title);
  q.bindValue(QSL(":script"), script);
  q.bindValue(QSL(":ordr"), new_ordr);

  q.exec();

  auto* fltr = new MessageFilter(q.lastInsertId().toInt());

  fltr->setName(title);
  fltr->setScript(script);
  fltr->setSortOrder(new_ordr);

  return fltr;
}

void DatabaseQueries::removeMessageFilter(const QSqlDatabase& db, int filter_id) {
  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM MessageFilters WHERE id = :id;"));
  q.bindValue(QSL(":id"), filter_id);
  q.exec();
}

QList<MessageFilter*> DatabaseQueries::getMessageFilters(const QSqlDatabase& db) {
  SqlQuery q(db);
  QList<MessageFilter*> filters;

  q.prepare(QSL("SELECT id, name, script, is_enabled, ordr FROM MessageFilters;"));
  q.exec();

  while (q.next()) {
    auto* filter = new MessageFilter(q.value(0).toInt());

    filter->setName(q.value(1).toString());
    filter->setScript(q.value(2).toString());
    filter->setEnabled(q.value(3).toBool());
    filter->setSortOrder(q.value(4).toInt());

    filters.append(filter);
  }

  return filters;
}

void DatabaseQueries::assignMessageFilterToFeed(const QSqlDatabase& db, int feed_id, int filter_id) {
  SqlQuery q(db);

  q.prepare(QSL("SELECT COUNT(*) FROM MessageFiltersInFeeds "
                "WHERE filter = :filter AND feed = :feed;"));

  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed"), feed_id);

  if (q.exec() && q.next()) {
    auto already_included_count = q.value(0).toInt();

    if (already_included_count > 0) {
      return;
    }
  }
  else {
    THROW_EX(SqlException, q.lastError());
  }

  q.prepare(QSL("INSERT INTO MessageFiltersInFeeds (filter, feed) "
                "VALUES (:filter, :feed);"));

  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed"), feed_id);

  q.exec();
}

void DatabaseQueries::updateMessageFilter(const QSqlDatabase& db, MessageFilter* filter) {
  SqlQuery q(db);

  q.prepare(QSL("UPDATE MessageFilters SET name = :name, script = :script, is_enabled = :is_enabled, ordr = :ordr "
                "WHERE id = :id;"));

  q.bindValue(QSL(":name"), filter->name());
  q.bindValue(QSL(":script"), filter->script());
  q.bindValue(QSL(":id"), filter->id());
  q.bindValue(QSL(":is_enabled"), filter->enabled() ? 1 : 0);
  q.bindValue(QSL(":ordr"), filter->sortOrder());

  q.exec();
}

void DatabaseQueries::removeMessageFilterFromFeed(const QSqlDatabase& db, int feed_id, int filter_id) {
  SqlQuery q(db);

  q.prepare(QSL("DELETE FROM MessageFiltersInFeeds "
                "WHERE filter = :filter AND feed = :feed;"));

  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed"), feed_id);

  q.exec();
}

QMultiMap<int, int> DatabaseQueries::messageFiltersInFeeds(const QSqlDatabase& db, int account_id) {
  SqlQuery q(db);
  QMultiMap<int, int> filters_in_feeds;

  q.prepare(QSL("SELECT filter, feed "
                "FROM MessageFiltersInFeeds "
                "WHERE feed IN (SELECT id FROM Feeds WHERE account_id = :account_id);"));
  q.bindValue(QSL(":account_id"), account_id);
  q.exec();

  while (q.next()) {
    filters_in_feeds.insert(q.value(1).toInt(), q.value(0).toInt());
  }

  return filters_in_feeds;
}

void DatabaseQueries::storeNewOauthTokens(const QSqlDatabase& db, const QString& refresh_token, int account_id) {
  modifyAccountCustomData(db, account_id, [&](QVariantHash& data) {
    data[QSL("refresh_token")] = refresh_token;
  });
}
