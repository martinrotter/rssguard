// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/databasequeries.h"

#include "3rd-party/boolinq/boolinq.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/abstract/category.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardserviceroot.h"

#include <QSqlDriver>
#include <QUrl>
#include <QVariant>

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
    auto json_obj = json.object();

    return json.object().toVariantHash();
  }
}

bool DatabaseQueries::isLabelAssignedToMessage(const QSqlDatabase& db, Label* label, const Message& msg) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("SELECT COUNT(*) FROM LabelsInMessages WHERE label = :label AND message = :message AND account_id = :account_id;");
  q.bindValue(QSL(":label"), label->customId());
  q.bindValue(QSL(":message"), msg.m_customId);
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());

  q.exec() && q.next();

  return q.record().value(0).toInt() > 0;
}

bool DatabaseQueries::deassignLabelFromMessage(const QSqlDatabase& db, Label* label, const Message& msg) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("DELETE FROM LabelsInMessages WHERE label = :label AND message = :message AND account_id = :account_id;");
  q.bindValue(QSL(":label"), label->customId());
  q.bindValue(QSL(":message"), msg.m_customId);
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());

  return q.exec();
}

bool DatabaseQueries::assignLabelToMessage(const QSqlDatabase& db, Label* label, const Message& msg) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("DELETE FROM LabelsInMessages WHERE label = :label AND message = :message AND account_id = :account_id;");
  q.bindValue(QSL(":label"), label->customId());
  q.bindValue(QSL(":message"), msg.m_customId);
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());

  auto succ = q.exec();

  if (succ) {
    q.prepare("INSERT INTO LabelsInMessages (label, message, account_id) VALUES (:label, :message, :account_id);");
    q.bindValue(QSL(":label"), label->customId());
    q.bindValue(QSL(":message"), msg.m_customId);
    q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());

    succ = q.exec();
  }

  return succ;
}

bool DatabaseQueries::setLabelsForMessage(const QSqlDatabase& db, const QList<Label*>& labels, const Message& msg) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM LabelsInMessages WHERE message = :message AND account_id = :account_id"));
  q.bindValue(QSL(":account_id"), msg.m_accountId);
  q.bindValue(QSL(":message"), msg.m_customId.isEmpty() ? QString::number(msg.m_id) : msg.m_customId);

  auto succ = q.exec();

  if (!succ) {
    return false;
  }

  q.prepare(QSL("INSERT INTO LabelsInMessages (message, label, account_id) VALUES (:message, :label, :account_id);"));

  for (const Label* label : labels) {
    q.bindValue(QSL(":account_id"), msg.m_accountId);
    q.bindValue(QSL(":message"), msg.m_customId.isEmpty() ? QString::number(msg.m_id) : msg.m_customId);
    q.bindValue(QSL(":label"), label->customId());

    if (!q.exec()) {
      return false;
    }
  }

  return true;
}

QList<Label*> DatabaseQueries::getLabels(const QSqlDatabase& db, int account_id) {
  QList<Label*> labels;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("SELECT * FROM Labels WHERE account_id = :account_id;");

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      Label* lbl = new Label(q.value(QSL("name")).toString(), QColor(q.value(QSL("color")).toString()));

      lbl->setId(q.value(QSL("id")).toInt());
      lbl->setCustomId(q.value(QSL("custom_id")).toString());

      labels << lbl;
    }
  }

  return labels;
}

QList<Label*> DatabaseQueries::getLabelsForMessage(const QSqlDatabase& db,
                                                   const Message& msg,
                                                   const QList<Label*> installed_labels) {
  QList<Label*> labels;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("SELECT DISTINCT label FROM LabelsInMessages WHERE message = :message AND account_id = :account_id;");

  q.bindValue(QSL(":account_id"), msg.m_accountId);
  q.bindValue(QSL(":message"), msg.m_customId);

  if (q.exec()) {
    auto iter = boolinq::from(installed_labels);

    while (q.next()) {
      auto lbl_id = q.value(0).toString();
      Label* candidate_label = iter.firstOrDefault([&](const Label* lbl) {
        return lbl->customId() == lbl_id;
      });

      if (candidate_label != nullptr) {
        labels << candidate_label;
      }
    }
  }

  return labels;
}

bool DatabaseQueries::updateLabel(const QSqlDatabase& db, Label* label) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("UPDATE Labels SET name = :name, color = :color "
            "WHERE id = :id AND account_id = :account_id;");
  q.bindValue(QSL(":name"), label->title());
  q.bindValue(QSL(":color"), label->color().name());
  q.bindValue(QSL(":id"), label->id());
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());

  return q.exec();
}

bool DatabaseQueries::deleteLabel(const QSqlDatabase& db, Label* label) {
  // NOTE: All dependecies are done via SQL foreign cascaded keys, so no
  // extra removals are needed.
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("DELETE FROM Labels WHERE id = :id AND account_id = :account_id;");
  q.bindValue(QSL(":id"), label->id());
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());

  if (q.exec()) {
    q.prepare("DELETE FROM LabelsInMessages WHERE label = :custom_id AND account_id = :account_id;");
    q.bindValue(QSL(":custom_id"), label->customId());
    q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());

    return q.exec();
  }
  else {
    return false;
  }
}

bool DatabaseQueries::createLabel(const QSqlDatabase& db, Label* label, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("INSERT INTO Labels (name, color, custom_id, account_id) "
            "VALUES (:name, :color, :custom_id, :account_id);");
  q.bindValue(QSL(":name"), label->title());
  q.bindValue(QSL(":color"), label->color().name());
  q.bindValue(QSL(":custom_id"), label->customId());
  q.bindValue(QSL(":account_id"), account_id);
  auto res = q.exec();

  if (res && q.lastInsertId().isValid()) {
    label->setId(q.lastInsertId().toInt());

    // NOTE: This custom ID in this object will be probably
    // overwritten in online-synchronized labels.
    if (label->customId().isEmpty()) {
      label->setCustomId(QString::number(label->id()));
    }
  }

  // Fixup missing custom IDs.
  q.prepare("UPDATE Labels SET custom_id = id WHERE custom_id IS NULL OR custom_id = '';");

  return q.exec() && res;
}

bool DatabaseQueries::markLabelledMessagesReadUnread(const QSqlDatabase& db, Label* label, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("UPDATE Messages SET is_read = :read "
            "WHERE "
            "    is_deleted = 0 AND "
            "    is_pdeleted = 0 AND "
            "    account_id = :account_id AND "
            "    EXISTS (SELECT * FROM LabelsInMessages WHERE LabelsInMessages.label = :label AND Messages.account_id = LabelsInMessages.account_id AND Messages.custom_id = LabelsInMessages.message);");
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());
  q.bindValue(QSL(":label"), label->customId());

  return q.exec();
}

bool DatabaseQueries::markImportantMessagesReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("UPDATE Messages SET is_read = :read "
            "WHERE is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);
  q.bindValue(QSL(":account_id"), account_id);
  return q.exec();
}

bool DatabaseQueries::markMessagesReadUnread(const QSqlDatabase& db, const QStringList& ids, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  return q.exec(QString(QSL("UPDATE Messages SET is_read = %2 WHERE id IN (%1);"))
                .arg(ids.join(QSL(", ")), read == RootItem::ReadStatus::Read ? QSL("1") : QSL("0")));
}

bool DatabaseQueries::markMessageImportant(const QSqlDatabase& db, int id, RootItem::Importance importance) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (!q.prepare(QSL("UPDATE Messages SET is_important = :important WHERE id = :id;"))) {
    qWarningNN << LOGSEC_DB
               << "Query preparation failed for message importance switch.";
    return false;
  }

  q.bindValue(QSL(":id"), id);
  q.bindValue(QSL(":important"), (int) importance);

  // Commit changes.
  return q.exec();
}

bool DatabaseQueries::markFeedsReadUnread(const QSqlDatabase& db, const QStringList& ids, int account_id, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QString("UPDATE Messages SET is_read = :read "
                    "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;").arg(ids.join(QSL(", "))));
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);
  q.bindValue(QSL(":account_id"), account_id);
  return q.exec();
}

bool DatabaseQueries::markBinReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("UPDATE Messages SET is_read = :read "
            "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);
  q.bindValue(QSL(":account_id"), account_id);
  return q.exec();
}

bool DatabaseQueries::markAccountReadUnread(const QSqlDatabase& db, int account_id, RootItem::ReadStatus read) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("UPDATE Messages SET is_read = :read WHERE is_pdeleted = 0 AND account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":read"), read == RootItem::ReadStatus::Read ? 1 : 0);
  return q.exec();
}

bool DatabaseQueries::switchMessagesImportance(const QSqlDatabase& db, const QStringList& ids) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  return q.exec(QString(QSL("UPDATE Messages SET is_important = NOT is_important WHERE id IN (%1);")).arg(ids.join(QSL(", "))));
}

bool DatabaseQueries::permanentlyDeleteMessages(const QSqlDatabase& db, const QStringList& ids) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  return q.exec(QString(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE id IN (%1);")).arg(ids.join(QSL(", "))));
}

bool DatabaseQueries::deleteOrRestoreMessagesToFromBin(const QSqlDatabase& db, const QStringList& ids, bool deleted) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  return q.exec(QString(QSL("UPDATE Messages SET is_deleted = %2, is_pdeleted = %3 WHERE id IN (%1);")).arg(ids.join(QSL(", ")),
                                                                                                            QString::number(deleted ? 1 : 0),
                                                                                                            QString::number(0)));
}

bool DatabaseQueries::restoreBin(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("UPDATE Messages SET is_deleted = 0 "
            "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  q.bindValue(QSL(":account_id"), account_id);
  return q.exec();
}

bool DatabaseQueries::purgeMessage(const QSqlDatabase& db, int message_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("DELETE FROM Messages WHERE id = :id;");
  q.bindValue(QSL(":id"), message_id);

  return q.exec();
}

bool DatabaseQueries::purgeImportantMessages(const QSqlDatabase& db) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Messages WHERE is_important = 1;"));
  return q.exec();
}

bool DatabaseQueries::purgeReadMessages(const QSqlDatabase& db) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND is_deleted = :is_deleted AND is_read = :is_read;"));
  q.bindValue(QSL(":is_read"), 1);

  // Remove only messages which are NOT in recycle bin.
  q.bindValue(QSL(":is_deleted"), 0);

  // Remove only messages which are NOT starred.
  q.bindValue(QSL(":is_important"), 0);
  return q.exec();
}

bool DatabaseQueries::purgeOldMessages(const QSqlDatabase& db, int older_than_days) {
  QSqlQuery q(db);
  const qint64 since_epoch = QDateTime::currentDateTimeUtc().addDays(-older_than_days).toMSecsSinceEpoch();

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND date_created < :date_created;"));
  q.bindValue(QSL(":date_created"), since_epoch);

  // Remove only messages which are NOT starred.
  q.bindValue(QSL(":is_important"), 0);
  return q.exec();
}

bool DatabaseQueries::purgeRecycleBin(const QSqlDatabase& db) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Messages WHERE is_important = :is_important AND is_deleted = :is_deleted;"));
  q.bindValue(QSL(":is_deleted"), 1);

  // Remove only messages which are NOT starred.
  q.bindValue(QSL(":is_important"), 0);
  return q.exec();
}

QMap<QString, QPair<int, int>> DatabaseQueries::getMessageCountsForCategory(const QSqlDatabase& db,
                                                                            const QString& custom_id,
                                                                            int account_id,
                                                                            bool only_total_counts,
                                                                            bool* ok) {
  QMap<QString, QPair<int, int>> counts;
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (only_total_counts) {
    q.prepare("SELECT feed, sum((is_read + 1) % 2), count(*) FROM Messages "
              "WHERE feed IN (SELECT custom_id FROM Feeds WHERE category = :category AND account_id = :account_id) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
              "GROUP BY feed;");
  }
  else {
    q.prepare("SELECT feed, sum((is_read + 1) % 2) FROM Messages "
              "WHERE feed IN (SELECT custom_id FROM Feeds WHERE category = :category AND account_id = :account_id) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
              "GROUP BY feed;");
  }

  q.bindValue(QSL(":category"), custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      QString feed_custom_id = q.value(0).toString();
      int unread_count = q.value(1).toInt();

      if (only_total_counts) {
        int total_count = q.value(2).toInt();

        counts.insert(feed_custom_id, QPair<int, int>(unread_count, total_count));
      }
      else {
        counts.insert(feed_custom_id, QPair<int, int>(unread_count, 0));
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return counts;
}

QMap<QString, QPair<int, int>> DatabaseQueries::getMessageCountsForAccount(const QSqlDatabase& db, int account_id,
                                                                           bool only_total_counts, bool* ok) {
  QMap<QString, QPair<int, int>> counts;
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (only_total_counts) {
    q.prepare("SELECT feed, sum((is_read + 1) % 2), count(*) FROM Messages "
              "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
              "GROUP BY feed;");
  }
  else {
    q.prepare("SELECT feed, sum((is_read + 1) % 2) FROM Messages "
              "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id "
              "GROUP BY feed;");
  }

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      QString feed_id = q.value(0).toString();
      int unread_count = q.value(1).toInt();

      if (only_total_counts) {
        int total_count = q.value(2).toInt();

        counts.insert(feed_id, QPair<int, int>(unread_count, total_count));
      }
      else {
        counts.insert(feed_id, QPair<int, int>(unread_count, 0));
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return counts;
}

int DatabaseQueries::getMessageCountsForFeed(const QSqlDatabase& db, const QString& feed_custom_id,
                                             int account_id, bool only_total_counts, bool* ok) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (only_total_counts) {
    q.prepare("SELECT count(*) FROM Messages "
              "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  }
  else {
    q.prepare("SELECT count(*) FROM Messages "
              "WHERE feed = :feed AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 0 AND account_id = :account_id;");
  }

  q.bindValue(QSL(":feed"), feed_custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    if (ok != nullptr) {
      *ok = true;
    }

    return q.value(0).toInt();
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }

    return 0;
  }
}

int DatabaseQueries::getMessageCountsForLabel(const QSqlDatabase& db, Label* label, int account_id, bool only_total_counts, bool* ok) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (only_total_counts) {
    q.prepare("SELECT COUNT(*) FROM Messages "
              "INNER JOIN LabelsInMessages "
              "ON "
              "  Messages.is_pdeleted = 0 AND Messages.is_deleted = 0 AND "
              "  LabelsInMessages.account_id = :account_id AND LabelsInMessages.account_id = Messages.account_id AND "
              "  LabelsInMessages.label = :label AND LabelsInMessages.message = Messages.custom_id;");
  }
  else {
    q.prepare("SELECT COUNT(*) FROM Messages "
              "INNER JOIN LabelsInMessages "
              "ON "
              "  Messages.is_pdeleted = 0 AND Messages.is_deleted = 0 AND Messages.is_read = 0 AND "
              "  LabelsInMessages.account_id = :account_id AND LabelsInMessages.account_id = Messages.account_id AND "
              "  LabelsInMessages.label = :label AND LabelsInMessages.message = Messages.custom_id;");
  }

  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":label"), label->customId());

  if (q.exec() && q.next()) {
    if (ok != nullptr) {
      *ok = true;
    }

    return q.value(0).toInt();
  }
  else {
    auto aa = q.lastError().text();

    if (ok != nullptr) {
      *ok = false;
    }

    return 0;
  }
}

int DatabaseQueries::getImportantMessageCounts(const QSqlDatabase& db, int account_id, bool only_total_counts, bool* ok) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (only_total_counts) {
    q.prepare("SELECT count(*) FROM Messages "
              "WHERE is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  }
  else {
    q.prepare("SELECT count(*) FROM Messages "
              "WHERE is_read = 0 AND is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  }

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    if (ok != nullptr) {
      *ok = true;
    }

    return q.value(0).toInt();
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }

    return 0;
  }
}

int DatabaseQueries::getMessageCountsForBin(const QSqlDatabase& db, int account_id, bool including_total_counts, bool* ok) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (including_total_counts) {
    q.prepare("SELECT count(*) FROM Messages "
              "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  }
  else {
    q.prepare("SELECT count(*) FROM Messages "
              "WHERE is_read = 0 AND is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  }

  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec() && q.next()) {
    if (ok != nullptr) {
      *ok = true;
    }

    return q.value(0).toInt();
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }

    return 0;
  }
}

QList<Message> DatabaseQueries::getUndeletedMessagesWithLabel(const QSqlDatabase& db, const Label* label, bool* ok) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.prepare(QSL(
              "SELECT Messages.id, Messages.is_read, Messages.is_deleted, Messages.is_important, Feeds.title, Messages.title, Messages.url, Messages.author, Messages.date_created, Messages.contents, Messages.is_pdeleted, Messages.enclosures, Messages.account_id, Messages.custom_id, Messages.custom_hash, Messages.feed, CASE WHEN length(Messages.enclosures) > 10 THEN 'true' ELSE 'false' END AS has_enclosures "
              "FROM Messages "
              "INNER JOIN Feeds "
              "ON Messages.feed = Feeds.custom_id AND Messages.account_id = :account_id AND Messages.account_id = Feeds.account_id "
              "INNER JOIN LabelsInMessages "
              "ON "
              "  Messages.is_pdeleted = 0 AND Messages.is_deleted = 0 AND "
              "  LabelsInMessages.account_id = :account_id AND LabelsInMessages.account_id = Messages.account_id AND "
              "  LabelsInMessages.label = :label AND LabelsInMessages.message = Messages.custom_id;"));
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());
  q.bindValue(QSL(":label"), label->customId());

  if (q.exec()) {
    while (q.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(q.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return messages;
}

QList<Message> DatabaseQueries::getUndeletedLabelledMessages(const QSqlDatabase& db, int account_id, bool* ok) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.prepare(QSL(
              "SELECT Messages.id, Messages.is_read, Messages.is_deleted, Messages.is_important, Feeds.title, Messages.title, Messages.url, Messages.author, Messages.date_created, Messages.contents, Messages.is_pdeleted, Messages.enclosures, Messages.account_id, Messages.custom_id, Messages.custom_hash, Messages.feed, CASE WHEN length(Messages.enclosures) > 10 THEN 'true' ELSE 'false' END AS has_enclosures "
              "FROM Messages "
              "LEFT JOIN Feeds "
              "ON Messages.feed = Feeds.custom_id AND Messages.account_id = Feeds.account_id "
              "WHERE Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.account_id = :account_id AND (SELECT COUNT(*) FROM LabelsInMessages WHERE account_id = :account_id AND message = Messages.custom_id) > 0;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(q.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return messages;
}

QList<Message> DatabaseQueries::getUndeletedImportantMessages(const QSqlDatabase& db, int account_id, bool* ok) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("SELECT id, is_read, is_deleted, is_important, custom_id, title, url, author, date_created, contents, is_pdeleted, enclosures, account_id, custom_id, custom_hash, feed, CASE WHEN length(Messages.enclosures) > 10 THEN 'true' ELSE 'false' END AS has_enclosures "
            "FROM Messages "
            "WHERE is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(q.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return messages;
}

QList<Message> DatabaseQueries::getUndeletedMessagesForFeed(const QSqlDatabase& db, const QString& feed_custom_id,
                                                            int account_id, bool* ok) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("SELECT id, is_read, is_deleted, is_important, custom_id, title, url, author, date_created, contents, is_pdeleted, enclosures, account_id, custom_id, custom_hash, feed, CASE WHEN length(Messages.enclosures) > 10 THEN 'true' ELSE 'false' END AS has_enclosures "
            "FROM Messages "
            "WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;");
  q.bindValue(QSL(":feed"), feed_custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(q.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return messages;
}

QList<Message> DatabaseQueries::getUndeletedMessagesForBin(const QSqlDatabase& db, int account_id, bool* ok) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("SELECT id, is_read, is_deleted, is_important, custom_id, title, url, author, date_created, contents, is_pdeleted, enclosures, account_id, custom_id, custom_hash, feed, CASE WHEN length(Messages.enclosures) > 10 THEN 'true' ELSE 'false' END AS has_enclosures "
            "FROM Messages "
            "WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;");
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(q.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return messages;
}

QList<Message> DatabaseQueries::getUndeletedMessagesForAccount(const QSqlDatabase& db, int account_id, bool* ok) {
  QList<Message> messages;
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare("SELECT id, is_read, is_deleted, is_important, custom_id, title, url, author, date_created, contents, is_pdeleted, enclosures, account_id, custom_id, custom_hash, feed, CASE WHEN length(Messages.enclosures) > 10 THEN 'true' ELSE 'false' END AS has_enclosures "
            "FROM Messages "
            "WHERE is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;");
  q.bindValue(QSL(":account_id"), account_id);

  if (q.exec()) {
    while (q.next()) {
      bool decoded;
      Message message = Message::fromSqlRecord(q.record(), &decoded);

      if (decoded) {
        messages.append(message);
      }
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return messages;
}

int DatabaseQueries::updateMessages(QSqlDatabase db,
                                    const QList<Message>& messages,
                                    const QString& feed_custom_id,
                                    int account_id,
                                    const QString& url,
                                    bool force_update,
                                    bool* any_message_changed,
                                    bool* ok) {
  if (messages.isEmpty()) {
    *any_message_changed = false;
    *ok = true;
    return 0;
  }

  bool use_transactions = qApp->settings()->value(GROUP(Database), SETTING(Database::UseTransactions)).toBool();
  int updated_messages = 0;

  // Prepare queries.
  QSqlQuery query_select_with_url(db);
  QSqlQuery query_select_with_custom_id(db);
  QSqlQuery query_select_with_id(db);
  QSqlQuery query_update(db);
  QSqlQuery query_insert(db);
  QSqlQuery query_begin_transaction(db);

  // Here we have query which will check for existence of the "same" message in given feed.
  // The two message are the "same" if:
  //   1) they belong to the SAME FEED AND,
  //   2) they have same URL AND,
  //   3) they have same AUTHOR AND,
  //   4) they have same TITLE.
  query_select_with_url.setForwardOnly(true);
  query_select_with_url.prepare("SELECT id, date_created, is_read, is_important, contents, feed FROM Messages "
                                "WHERE feed = :feed AND title = :title AND url = :url AND author = :author AND account_id = :account_id;");

  // When we have custom ID of the message, we can check directly for existence
  // of that particular message.
  query_select_with_custom_id.setForwardOnly(true);
  query_select_with_custom_id.prepare("SELECT id, date_created, is_read, is_important, contents, feed FROM Messages "
                                      "WHERE custom_id = :custom_id AND account_id = :account_id;");

  // In some case, messages are already stored in the DB and they all have primary DB ID.
  // This is particularly the case when user runs some message filter manually on existing messages
  // of some feed.
  query_select_with_id.setForwardOnly(true);
  query_select_with_id.prepare("SELECT date_created, is_read, is_important, contents, feed FROM Messages "
                               "WHERE id = :id AND account_id = :account_id;");

  // Used to insert new messages.
  query_insert.setForwardOnly(true);
  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, is_read, is_important, is_deleted, url, author, score, date_created, contents, enclosures, custom_id, custom_hash, account_id) "
                       "VALUES (:feed, :title, :is_read, :is_important, :is_deleted, :url, :author, :score, :date_created, :contents, :enclosures, :custom_id, :custom_hash, :account_id);");

  // Used to update existing messages.
  query_update.setForwardOnly(true);
  query_update.prepare("UPDATE Messages "
                       "SET title = :title, is_read = :is_read, is_important = :is_important, is_deleted = :is_deleted, url = :url, author = :author, score = :score, date_created = :date_created, contents = :contents, enclosures = :enclosures, feed = :feed "
                       "WHERE id = :id;");

  if (use_transactions && !query_begin_transaction.exec(qApp->database()->obtainBeginTransactionSql())) {
    qCriticalNN << LOGSEC_DB
                << "Transaction start for message downloader failed: '"
                << query_begin_transaction.lastError().text() << "'.";
    return updated_messages;
  }

  for (Message message : messages) {
    // Check if messages contain relative URLs and if they do, then replace them.
    if (message.m_url.startsWith(QL1S("//"))) {
      message.m_url = QString(URI_SCHEME_HTTP) + message.m_url.mid(2);
    }
    else if (message.m_url.startsWith(QL1S("/"))) {
      QString new_message_url = QUrl(url).toString(QUrl::UrlFormattingOption::RemoveUserInfo |
                                                   QUrl::UrlFormattingOption::RemovePath |
                                                   QUrl::UrlFormattingOption::RemoveQuery |
                                                   QUrl::UrlFormattingOption::RemoveFilename |
                                                   QUrl::UrlFormattingOption::StripTrailingSlash);

      new_message_url += message.m_url;
      message.m_url = new_message_url;
    }

    int id_existing_message = -1;
    qint64 date_existing_message = 0;
    bool is_read_existing_message = false;
    bool is_important_existing_message = false;
    QString contents_existing_message;
    QString feed_id_existing_message;

    if (message.m_id > 0) {
      // We recognize directly existing message.
      // NOTE: Particular for manual message filter execution.
      query_select_with_id.bindValue(QSL(":id"), message.m_id);
      query_select_with_id.bindValue(QSL(":account_id"), account_id);

      qDebugNN << LOGSEC_DB
               << "Checking if message with primary ID"
               << QUOTE_W_SPACE(message.m_id)
               << "is present in DB.";

      if (query_select_with_id.exec() && query_select_with_id.next()) {
        id_existing_message = message.m_id;
        date_existing_message = query_select_with_id.value(0).value<qint64>();
        is_read_existing_message = query_select_with_id.value(1).toBool();
        is_important_existing_message = query_select_with_id.value(2).toBool();
        contents_existing_message = query_select_with_id.value(3).toString();
        feed_id_existing_message = query_select_with_id.value(4).toString();

        qDebugNN << LOGSEC_DB
                 << "Message with these attributes is already present in DB and has DB ID '"
                 << id_existing_message
                 << "'.";
      }
      else if (query_select_with_id.lastError().isValid()) {
        qWarningNN << LOGSEC_DB
                   << "Failed to check for existing message in DB via primary ID: '"
                   << query_select_with_id.lastError().text()
                   << "'.";
      }

      query_select_with_id.finish();
    }
    else if (message.m_customId.isEmpty()) {
      // We need to recognize existing messages according URL & AUTHOR & TITLE.
      // NOTE: This particularly concerns messages from standard account.
      query_select_with_url.bindValue(QSL(":feed"), unnulifyString(feed_custom_id));
      query_select_with_url.bindValue(QSL(":title"), unnulifyString(message.m_title));
      query_select_with_url.bindValue(QSL(":url"), unnulifyString(message.m_url));
      query_select_with_url.bindValue(QSL(":author"), unnulifyString(message.m_author));
      query_select_with_url.bindValue(QSL(":account_id"), account_id);

      qDebugNN << LOGSEC_DB
               << "Checking if message with title '"
               << message.m_title
               << "', url '"
               << message.m_url
               << "' and author '"
               << message.m_author
               << "' is present in DB.";

      if (query_select_with_url.exec() && query_select_with_url.next()) {
        id_existing_message = query_select_with_url.value(0).toInt();
        date_existing_message = query_select_with_url.value(1).value<qint64>();
        is_read_existing_message = query_select_with_url.value(2).toBool();
        is_important_existing_message = query_select_with_url.value(3).toBool();
        contents_existing_message = query_select_with_url.value(4).toString();
        feed_id_existing_message = query_select_with_url.value(5).toString();

        qDebugNN << LOGSEC_DB
                 << "Message with these attributes is already present in DB and has DB ID '"
                 << id_existing_message
                 << "'.";
      }
      else if (query_select_with_url.lastError().isValid()) {
        qWarningNN << LOGSEC_DB
                   << "Failed to check for existing message in DB via URL: '"
                   << query_select_with_url.lastError().text()
                   << "'.";
      }

      query_select_with_url.finish();
    }
    else {
      // We can recognize existing messages via their custom ID.
      // NOTE: This concerns messages from custom accounts, like TT-RSS or Nextcloud News.
      query_select_with_custom_id.bindValue(QSL(":account_id"), account_id);
      query_select_with_custom_id.bindValue(QSL(":custom_id"), unnulifyString(message.m_customId));

      qDebugNN << LOGSEC_DB
               << "Checking if message with custom ID '"
               << message.m_customId
               << "' is present in DB.";

      if (query_select_with_custom_id.exec() && query_select_with_custom_id.next()) {
        id_existing_message = query_select_with_custom_id.value(0).toInt();
        date_existing_message = query_select_with_custom_id.value(1).value<qint64>();
        is_read_existing_message = query_select_with_custom_id.value(2).toBool();
        is_important_existing_message = query_select_with_custom_id.value(3).toBool();
        contents_existing_message = query_select_with_custom_id.value(4).toString();
        feed_id_existing_message = query_select_with_custom_id.value(5).toString();

        qDebugNN << LOGSEC_DB
                 << "Message with custom ID"
                 << QUOTE_W_SPACE(message.m_customId)
                 << "is already present in DB and has DB ID '"
                 << id_existing_message
                 << "'.";
      }
      else if (query_select_with_custom_id.lastError().isValid()) {
        qWarningNN << LOGSEC_DB
                   << "Failed to check for existing message in DB via ID: '"
                   << query_select_with_custom_id.lastError().text()
                   << "'.";
      }

      query_select_with_custom_id.finish();
    }

    // Now, check if this message is already in the DB.
    if (id_existing_message >= 0) {
      // Message is already in the DB.
      //
      // Now, we update it if at least one of next conditions is true:
      //   1) Message has custom ID AND (its date OR read status OR starred status are changed or message
      //      was moved from one feed to another - this can particularly happen in Gmail feeds).
      //
      //   2) Message has its date fetched from feed AND its date is different from date in DB and contents is changed.
      //
      //   3) Message update is force, we want to overwrite message as some arbitrary atribute was changed,
      //      this particularly happens when manual message filter execution happens.
      if (/* 1 */ (!message.m_customId.isEmpty() && (message.m_created.toMSecsSinceEpoch() != date_existing_message ||
                                                     message.m_isRead != is_read_existing_message ||
                                                     message.m_isImportant != is_important_existing_message ||
                                                     message.m_feedId != feed_id_existing_message ||
                                                     message.m_contents != contents_existing_message)) ||

          /* 2 */ (message.m_createdFromFeed && message.m_created.toMSecsSinceEpoch() != date_existing_message &&
                   message.m_contents != contents_existing_message) ||

          /* 3 */ force_update) {
        // Message exists, it is changed, update it.
        query_update.bindValue(QSL(":title"), unnulifyString(message.m_title));
        query_update.bindValue(QSL(":is_read"), int(message.m_isRead));
        query_update.bindValue(QSL(":is_important"), int(message.m_isImportant));
        query_update.bindValue(QSL(":is_deleted"), int(message.m_isDeleted));
        query_update.bindValue(QSL(":url"), unnulifyString(message.m_url));
        query_update.bindValue(QSL(":author"), unnulifyString(message.m_author));
        query_update.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
        query_update.bindValue(QSL(":contents"), unnulifyString(message.m_contents));
        query_update.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
        query_update.bindValue(QSL(":feed"), unnulifyString(feed_id_existing_message));
        query_update.bindValue(QSL(":score"), message.m_score);
        query_update.bindValue(QSL(":id"), id_existing_message);

        *any_message_changed = true;

        if (query_update.exec()) {
          qDebugNN << LOGSEC_DB
                   << "Updating message with title"
                   << QUOTE_W_SPACE(message.m_title)
                   << "URL"
                   << QUOTE_W_SPACE(message.m_url)
                   << "in DB.";

          if (!message.m_isRead) {
            updated_messages++;
          }
        }
        else if (query_update.lastError().isValid()) {
          qWarningNN << LOGSEC_DB
                     << "Failed to update message in DB: '" << query_update.lastError().text() << "'.";
        }

        query_update.finish();
      }
    }
    else {
      // Message with this URL is not fetched in this feed yet.
      query_insert.bindValue(QSL(":feed"), unnulifyString(feed_custom_id));
      query_insert.bindValue(QSL(":title"), unnulifyString(message.m_title));
      query_insert.bindValue(QSL(":is_read"), int(message.m_isRead));
      query_insert.bindValue(QSL(":is_important"), int(message.m_isImportant));
      query_insert.bindValue(QSL(":is_deleted"), int(message.m_isDeleted));
      query_insert.bindValue(QSL(":url"), unnulifyString( message.m_url));
      query_insert.bindValue(QSL(":author"), unnulifyString(message.m_author));
      query_insert.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
      query_insert.bindValue(QSL(":contents"), unnulifyString(message.m_contents));
      query_insert.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
      query_insert.bindValue(QSL(":custom_id"), unnulifyString(message.m_customId));
      query_insert.bindValue(QSL(":custom_hash"), unnulifyString(message.m_customHash));
      query_insert.bindValue(QSL(":score"), message.m_score);
      query_insert.bindValue(QSL(":account_id"), account_id);

      if (query_insert.exec() && query_insert.numRowsAffected() == 1) {
        updated_messages++;

        if (query_insert.lastInsertId().isValid()) {
          id_existing_message = query_insert.lastInsertId().toInt();
        }

        qDebugNN << LOGSEC_DB
                 << "Adding new message with title '"
                 << message.m_title
                 << "' url '"
                 << message.m_url
                 << "' to DB.";
      }
      else if (query_insert.lastError().isValid()) {
        qWarningNN << LOGSEC_DB
                   << "Failed to insert message to DB: '"
                   << query_insert.lastError().text()
                   << "' - message title is '"
                   << message.m_title
                   << "'.";
      }

      query_insert.finish();
    }

    // Update labels assigned to message.
    if (!message.m_assignedLabels.isEmpty()) {
      // NOTE: This message provides list of its labels from remote
      // source, which means they must replace local labels.
      if (!message.m_customId.isEmpty()) {
        setLabelsForMessage(db, message.m_assignedLabels, message);
      }
      else if (id_existing_message >= 0) {
        message.m_id = id_existing_message;
        setLabelsForMessage(db, message.m_assignedLabels, message);
      }
      else {
        qWarningNN << LOGSEC_DB
                   << "Cannot set labels for message"
                   << QUOTE_W_SPACE(message.m_title)
                   << "because we don't have ID or custom ID.";
      }
    }
  }

  // Now, fixup custom IDS for messages which initially did not have them,
  // just to keep the data consistent.
  if (db.exec("UPDATE Messages "
              "SET custom_id = id "
              "WHERE custom_id IS NULL OR custom_id = '';").lastError().isValid()) {
    qWarningNN << LOGSEC_DB
               << "Failed to set custom ID for all messages: '"
               << db.lastError().text()
               << "'.";
  }

  if (use_transactions && !db.commit()) {
    qCriticalNN << LOGSEC_DB
                << "Transaction commit for message downloader failed: '"
                << db.lastError().text()
                << "'.";
    db.rollback();

    if (ok != nullptr) {
      *ok = false;
      updated_messages = 0;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = true;
    }
  }

  return updated_messages;
}

bool DatabaseQueries::purgeMessagesFromBin(const QSqlDatabase& db, bool clear_only_read, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (clear_only_read) {
    q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE is_read = 1 AND is_deleted = 1 AND account_id = :account_id;"));
  }
  else {
    q.prepare(QSL("UPDATE Messages SET is_pdeleted = 1 WHERE is_deleted = 1 AND account_id = :account_id;"));
  }

  q.bindValue(QSL(":account_id"), account_id);
  return q.exec();
}

bool DatabaseQueries::deleteAccount(const QSqlDatabase& db, int account_id) {
  QSqlQuery query(db);

  query.setForwardOnly(true);
  QStringList queries;

  queries << QSL("DELETE FROM MessageFiltersInFeeds WHERE account_id = :account_id;")
          << QSL("DELETE FROM LabelsInMessages WHERE account_id = :account_id;")
          << QSL("DELETE FROM Messages WHERE account_id = :account_id;")
          << QSL("DELETE FROM Feeds WHERE account_id = :account_id;")
          << QSL("DELETE FROM Categories WHERE account_id = :account_id;")
          << QSL("DELETE FROM Labels WHERE account_id = :account_id;")
          << QSL("DELETE FROM Accounts WHERE id = :account_id;");

  for (const QString& q : queries) {
    query.prepare(q);
    query.bindValue(QSL(":account_id"), account_id);

    if (!query.exec()) {
      qCriticalNN << LOGSEC_DB
                  << "Removing of account from DB failed, this is critical: '"
                  << query.lastError().text()
                  << "'.";
      return false;
    }
    else {
      query.finish();
    }
  }

  return true;
}

bool DatabaseQueries::deleteAccountData(const QSqlDatabase& db, int account_id, bool delete_messages_too) {
  bool result = true;
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (delete_messages_too) {
    q.prepare(QSL("DELETE FROM Messages WHERE account_id = :account_id;"));
    q.bindValue(QSL(":account_id"), account_id);
    result &= q.exec();
  }

  q.prepare(QSL("DELETE FROM Feeds WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  result &= q.exec();

  q.prepare(QSL("DELETE FROM Categories WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  result &= q.exec();

  if (delete_messages_too) {
    // If we delete message, make sure to delete message/label assignments too.
    q.prepare(QSL("DELETE FROM LabelsInMessages WHERE account_id = :account_id;"));
    q.bindValue(QSL(":account_id"), account_id);
    result &= q.exec();
  }

  q.prepare(QSL("DELETE FROM Labels WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  result &= q.exec();

  return result;
}

bool DatabaseQueries::cleanLabelledMessages(const QSqlDatabase& db, bool clean_read_only, Label* label) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (clean_read_only) {
    q.prepare(QSL("UPDATE Messages SET is_deleted = :deleted "
                  "WHERE "
                  "    is_deleted = 0 AND "
                  "    is_pdeleted = 0 AND "
                  "    is_read = 1 AND "
                  "    account_id = :account_id AND "
                  "    EXISTS (SELECT * FROM LabelsInMessages WHERE LabelsInMessages.label = :label AND Messages.account_id = LabelsInMessages.account_id AND Messages.custom_id = LabelsInMessages.message);"));
  }
  else {
    q.prepare(QSL("UPDATE Messages SET is_deleted = :deleted "
                  "WHERE "
                  "    is_deleted = 0 AND "
                  "    is_pdeleted = 0 AND "
                  "    account_id = :account_id AND "
                  "    EXISTS (SELECT * FROM LabelsInMessages WHERE LabelsInMessages.label = :label AND Messages.account_id = LabelsInMessages.account_id AND Messages.custom_id = LabelsInMessages.message);"));
  }

  q.bindValue(QSL(":deleted"), 1);
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());
  q.bindValue(QSL(":label"), label->customId());

  if (!q.exec()) {
    qWarningNN << LOGSEC_DB
               << "Cleaning of labelled messages failed: '"
               << q.lastError().text()
               << "'.";
    return false;
  }
  else {
    return true;
  }
}

bool DatabaseQueries::cleanImportantMessages(const QSqlDatabase& db, bool clean_read_only, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (clean_read_only) {
    q.prepare(QSL("UPDATE Messages SET is_deleted = :deleted "
                  "WHERE is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 1 AND account_id = :account_id;"));
  }
  else {
    q.prepare(QSL("UPDATE Messages SET is_deleted = :deleted "
                  "WHERE is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;"));
  }

  q.bindValue(QSL(":deleted"), 1);
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    qWarningNN << LOGSEC_DB
               << "Cleaning of important messages failed: '"
               << q.lastError().text()
               << "'.";
    return false;
  }
  else {
    return true;
  }
}

bool DatabaseQueries::cleanFeeds(const QSqlDatabase& db, const QStringList& ids, bool clean_read_only, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);

  if (clean_read_only) {
    q.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                      "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND is_read = 1 AND account_id = :account_id;")
              .arg(ids.join(QSL(", "))));
  }
  else {
    q.prepare(QString("UPDATE Messages SET is_deleted = :deleted "
                      "WHERE feed IN (%1) AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;")
              .arg(ids.join(QSL(", "))));
  }

  q.bindValue(QSL(":deleted"), 1);
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    qWarningNN << LOGSEC_DB
               << "Cleaning of feeds failed: '"
               << q.lastError().text()
               << "'.";
    return false;
  }
  else {
    return true;
  }
}

bool DatabaseQueries::purgeLeftoverMessageFilterAssignments(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(
    QSL("DELETE FROM MessageFiltersInFeeds "
        "WHERE account_id = :account_id AND "
        "feed_custom_id NOT IN (SELECT custom_id FROM Feeds WHERE account_id = :account_id);"));
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    qWarningNN << LOGSEC_DB
               << "Removing of leftover message filter assignments failed: '"
               << q.lastError().text()
               << "'.";
    return false;
  }
  else {
    return true;
  }
}

bool DatabaseQueries::purgeLeftoverMessages(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Messages "
                "WHERE account_id = :account_id AND feed NOT IN (SELECT custom_id FROM Feeds WHERE account_id = :account_id);"));
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    qWarningNN << LOGSEC_DB
               << "Removing of leftover messages failed: '"
               << q.lastError().text()
               << "'.";
    return false;
  }
  else {
    return true;
  }
}

bool DatabaseQueries::purgeLeftoverLabelAssignments(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);
  bool succ = false;

  if (account_id <= 0) {
    succ = q.exec(QSL("DELETE FROM LabelsInMessages "
                      "WHERE NOT EXISTS (SELECT * FROM Messages WHERE Messages.account_id = LabelsInMessages.account_id AND Messages.custom_id = LabelsInMessages.message);"))
           &&
           q.exec(QSL("DELETE FROM LabelsInMessages "
                      "WHERE NOT EXISTS (SELECT * FROM Labels WHERE Labels.account_id = LabelsInMessages.account_id AND Labels.custom_id = LabelsInMessages.label);"));
  }
  else {
    q.prepare(QSL("DELETE FROM LabelsInMessages "
                  "WHERE account_id = :account_id AND "
                  "      (message NOT IN (SELECT custom_id FROM Messages WHERE account_id = :account_id) OR "
                  "       label NOT IN (SELECT custom_id FROM Labels WHERE account_id = :account_id));"));
    q.bindValue(QSL(":account_id"), account_id);
    succ = q.exec();
  }

  if (!succ) {
    auto xx = q.lastError().text();

    qWarningNN << LOGSEC_DB
               << "Removing of leftover label assignments failed: '"
               << q.lastError().text()
               << "'.";
  }

  return succ;
}

bool DatabaseQueries::purgeLabelsAndLabelAssignments(const QSqlDatabase& db, int account_id) {
  QSqlQuery q(db);

  q.prepare(QSL("DELETE FROM LabelsInMessages WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  auto succ = q.exec();

  q.prepare(QSL("DELETE FROM Labels WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  succ &= q.exec();

  return succ;
}

bool DatabaseQueries::storeAccountTree(const QSqlDatabase& db, RootItem* tree_root, int account_id) {
  QSqlQuery query_category(db);
  QSqlQuery query_feed(db);

  // Iterate all children.
  for (RootItem* child : tree_root->getSubTree()) {
    if (child->kind() == RootItem::Kind::Category) {
      createOverwriteCategory(db, child->toCategory(), account_id, child->parent()->id());
    }
    else if (child->kind() == RootItem::Kind::Feed) {
      createOverwriteFeed(db, child->toFeed(), account_id, child->parent()->id());
    }
    else if (child->kind() == RootItem::Kind::Labels) {
      // Add all labels.
      for (RootItem* lbl : child->childItems()) {
        Label* label = lbl->toLabel();

        if (!createLabel(db, label, account_id)) {
          return false;
        }
      }
    }
  }

  return true;
}

QStringList DatabaseQueries::customIdsOfMessagesFromAccount(const QSqlDatabase& db, int account_id, bool* ok) {
  QSqlQuery q(db);
  QStringList ids;

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT custom_id FROM Messages WHERE is_pdeleted = 0 AND account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (ok != nullptr) {
    *ok = q.exec();
  }
  else {
    q.exec();
  }

  while (q.next()) {
    ids.append(q.value(0).toString());
  }

  return ids;
}

QStringList DatabaseQueries::customIdsOfMessagesFromLabel(const QSqlDatabase& db, Label* label, bool* ok) {
  QSqlQuery q(db);
  QStringList ids;

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT custom_id FROM Messages "
                "WHERE "
                "    is_deleted = 0 AND "
                "    is_pdeleted = 0 AND "
                "    account_id = :account_id AND "
                "    EXISTS (SELECT * FROM LabelsInMessages WHERE LabelsInMessages.label = :label AND Messages.account_id = LabelsInMessages.account_id AND Messages.custom_id = LabelsInMessages.message);"));
  q.bindValue(QSL(":account_id"), label->getParentServiceRoot()->accountId());
  q.bindValue(QSL(":label"), label->customId());

  if (ok != nullptr) {
    *ok = q.exec();
  }
  else {
    q.exec();
  }

  while (q.next()) {
    ids.append(q.value(0).toString());
  }

  return ids;
}

QStringList DatabaseQueries::customIdsOfImportantMessages(const QSqlDatabase& db, int account_id, bool* ok) {
  QSqlQuery q(db);
  QStringList ids;

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT custom_id FROM Messages "
                "WHERE is_important = 1 AND is_deleted = 0 AND is_pdeleted = 0 AND account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (ok != nullptr) {
    *ok = q.exec();
  }
  else {
    q.exec();
  }

  while (q.next()) {
    ids.append(q.value(0).toString());
  }

  return ids;
}

QStringList DatabaseQueries::customIdsOfMessagesFromBin(const QSqlDatabase& db, int account_id, bool* ok) {
  QSqlQuery q(db);
  QStringList ids;

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 1 AND is_pdeleted = 0 AND account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);

  if (ok != nullptr) {
    *ok = q.exec();
  }
  else {
    q.exec();
  }

  while (q.next()) {
    ids.append(q.value(0).toString());
  }

  return ids;
}

QStringList DatabaseQueries::customIdsOfMessagesFromFeed(const QSqlDatabase& db, const QString& feed_custom_id, int account_id, bool* ok) {
  QSqlQuery q(db);
  QStringList ids;

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT custom_id FROM Messages WHERE is_deleted = 0 AND is_pdeleted = 0 AND feed = :feed AND account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":feed"), feed_custom_id);

  if (ok != nullptr) {
    *ok = q.exec();
  }
  else {
    q.exec();
  }

  while (q.next()) {
    ids.append(q.value(0).toString());
  }

  return ids;
}

void DatabaseQueries::createOverwriteCategory(const QSqlDatabase& db, Category* category, int account_id, int parent_id) {
  QSqlQuery q(db);

  if (category->id() <= 0) {
    // We need to insert category first.
    q.prepare(QSL("INSERT INTO "
                  "Categories (parent_id, title, date_created, account_id) "
                  "VALUES (0, 'new', 0, %1);").arg(QString::number(account_id)));

    if (!q.exec()) {
      throw ApplicationException(q.lastError().text());
    }
    else {
      category->setId(q.lastInsertId().toInt());
    }
  }

  q.prepare("UPDATE Categories "
            "SET parent_id = :parent_id, title = :title, description = :description, date_created = :date_created, "
            "    icon = :icon, account_id = :account_id, custom_id = :custom_id "
            "WHERE id = :id;");
  q.bindValue(QSL(":parent_id"), parent_id);
  q.bindValue(QSL(":title"), category->title());
  q.bindValue(QSL(":description"), category->description());
  q.bindValue(QSL(":date_created"), category->creationDate().toMSecsSinceEpoch());
  q.bindValue(QSL(":icon"), qApp->icons()->toByteArray(category->icon()));
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":custom_id"), category->customId());
  q.bindValue(QSL(":id"), category->id());

  if (!q.exec()) {
    throw ApplicationException(q.lastError().text());
  }
}

void DatabaseQueries::createOverwriteFeed(const QSqlDatabase& db, Feed* feed, int account_id, int parent_id) {
  QSqlQuery q(db);

  if (feed->id() <= 0) {
    // We need to insert feed first.
    q.prepare(QSL("INSERT INTO "
                  "Feeds (title, date_created, category, update_type, update_interval, account_id, custom_id) "
                  "VALUES ('new', 0, 0, 0, 1, %1, 'new');").arg(QString::number(account_id)));

    if (!q.exec()) {
      throw ApplicationException(q.lastError().text());
    }
    else {
      feed->setId(q.lastInsertId().toInt());

      if (feed->customId().isEmpty()) {
        feed->setCustomId(QString::number(feed->id()));
      }
    }
  }

  q.prepare("UPDATE Feeds "
            "SET title = :title, description = :description, date_created = :date_created, "
            "    icon = :icon, category = :category, source = :source, update_type = :update_type, "
            "    update_interval = :update_interval, account_id = :account_id, "
            "    custom_id = :custom_id, custom_data = :custom_data "
            "WHERE id = :id;");
  q.bindValue(QSL(":title"), feed->title());
  q.bindValue(QSL(":description"), feed->description());
  q.bindValue(QSL(":date_created"), feed->creationDate().toMSecsSinceEpoch());
  q.bindValue(QSL(":icon"), qApp->icons()->toByteArray(feed->icon()));
  q.bindValue(QSL(":category"), parent_id);
  q.bindValue(QSL(":source"), feed->source());
  q.bindValue(QSL(":update_type"), int(feed->autoUpdateType()));
  q.bindValue(QSL(":update_interval"), feed->autoUpdateInitialInterval());
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":custom_id"), feed->customId());
  q.bindValue(QSL(":id"), feed->id());

  auto custom_data = feed->customDatabaseData();
  QString serialized_custom_data = serializeCustomData(custom_data);

  q.bindValue(QSL(":custom_data"), serialized_custom_data);

  if (!q.exec()) {
    auto xx = q.lastError().text();

    throw ApplicationException(q.lastError().text());
  }
}

void DatabaseQueries::createOverwriteAccount(const QSqlDatabase& db, ServiceRoot* account) {
  QSqlQuery q(db);

  if (account->accountId() <= 0) {
    // We need to insert account first.
    q.prepare(QSL("INSERT INTO Accounts (type) VALUES (:type);"));
    q.bindValue(QSL(":type"), account->code());

    if (!q.exec()) {
      throw ApplicationException(q.lastError().text());
    }
    else {
      //account->setId(q.lastInsertId().toInt());
      account->setAccountId(q.lastInsertId().toInt());
    }
  }

  // Now we construct the SQL update query.
  auto proxy = account->networkProxy();

  q.prepare(QSL("UPDATE Accounts "
                "SET proxy_type = :proxy_type, proxy_host = :proxy_host, proxy_port = :proxy_port, "
                "    proxy_username = :proxy_username, proxy_password = :proxy_password, "
                "    custom_data = :custom_data "
                "WHERE id = :id"));
  q.bindValue(QSL(":proxy_type"), proxy.type());
  q.bindValue(QSL(":proxy_host"), proxy.hostName());
  q.bindValue(QSL(":proxy_port"), proxy.port());
  q.bindValue(QSL(":proxy_username"), proxy.user());
  q.bindValue(QSL(":proxy_password"), TextFactory::encrypt(proxy.password()));
  q.bindValue(QSL(":id"), account->accountId());

  auto custom_data = account->customDatabaseData();
  QString serialized_custom_data = serializeCustomData(custom_data);

  q.bindValue(QSL(":custom_data"), serialized_custom_data);

  if (!q.exec()) {
    throw ApplicationException(q.lastError().text());
  }
}

bool DatabaseQueries::deleteFeed(const QSqlDatabase& db, int feed_custom_id, int account_id) {
  QSqlQuery q(db);

  q.prepare(QSL("DELETE FROM Messages WHERE feed = :feed AND account_id = :account_id;"));
  q.bindValue(QSL(":feed"), feed_custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  if (!q.exec()) {
    return false;
  }

  // Remove feed itself.
  q.prepare(QSL("DELETE FROM Feeds WHERE custom_id = :feed AND account_id = :account_id;"));
  q.bindValue(QSL(":feed"), feed_custom_id);
  q.bindValue(QSL(":account_id"), account_id);

  return q.exec() &&
         purgeLeftoverMessageFilterAssignments(db, account_id) &&
         purgeLeftoverLabelAssignments(db, account_id);
}

bool DatabaseQueries::deleteCategory(const QSqlDatabase& db, int id) {
  QSqlQuery q(db);

  // Remove this category from database.
  q.setForwardOnly(true);
  q.prepare(QSL("DELETE FROM Categories WHERE id = :category;"));
  q.bindValue(QSL(":category"), id);
  return q.exec();
}

MessageFilter* DatabaseQueries::addMessageFilter(const QSqlDatabase& db, const QString& title,
                                                 const QString& script) {
  if (!db.driver()->hasFeature(QSqlDriver::DriverFeature::LastInsertId)) {
    throw ApplicationException(QObject::tr("Cannot insert message filter, because current database cannot return last inserted row ID."));
  }

  QSqlQuery q(db);

  q.prepare("INSERT INTO MessageFilters (name, script) VALUES(:name, :script);");

  q.bindValue(QSL(":name"), title);
  q.bindValue(QSL(":script"), script);
  q.setForwardOnly(true);

  if (q.exec()) {
    auto* fltr = new MessageFilter(q.lastInsertId().toInt());

    fltr->setName(title);
    fltr->setScript(script);

    return fltr;
  }
  else {
    throw ApplicationException(q.lastError().text());
  }
}

void DatabaseQueries::removeMessageFilter(const QSqlDatabase& db, int filter_id, bool* ok) {
  QSqlQuery q(db);

  q.prepare("DELETE FROM MessageFilters WHERE id = :id;");

  q.bindValue(QSL(":id"), filter_id);
  q.setForwardOnly(true);

  if (q.exec()) {
    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }
}

void DatabaseQueries::removeMessageFilterAssignments(const QSqlDatabase& db, int filter_id, bool* ok) {
  QSqlQuery q(db);

  q.prepare("DELETE FROM MessageFiltersInFeeds WHERE filter = :filter;");

  q.bindValue(QSL(":filter"), filter_id);
  q.setForwardOnly(true);

  if (q.exec()) {
    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }
}

QList<MessageFilter*> DatabaseQueries::getMessageFilters(const QSqlDatabase& db, bool* ok) {
  QSqlQuery q(db);
  QList<MessageFilter*> filters;

  q.setForwardOnly(true);
  q.prepare(QSL("SELECT * FROM MessageFilters;"));

  if (q.exec()) {
    while (q.next()) {
      auto rec = q.record();
      auto* filter = new MessageFilter(rec.value(0).toInt());

      filter->setName(rec.value(1).toString());
      filter->setScript(rec.value(2).toString());

      filters.append(filter);
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return filters;
}

QMultiMap<QString, int> DatabaseQueries::messageFiltersInFeeds(const QSqlDatabase& db, int account_id, bool* ok) {
  QSqlQuery q(db);
  QMultiMap<QString, int> filters_in_feeds;

  q.prepare("SELECT filter, feed_custom_id FROM MessageFiltersInFeeds WHERE account_id = :account_id;");

  q.bindValue(QSL(":account_id"), account_id);
  q.setForwardOnly(true);

  if (q.exec()) {
    while (q.next()) {
      auto rec = q.record();

      filters_in_feeds.insert(rec.value(1).toString(), rec.value(0).toInt());
    }

    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }

  return filters_in_feeds;
}

void DatabaseQueries::assignMessageFilterToFeed(const QSqlDatabase& db, const QString& feed_custom_id,
                                                int filter_id, int account_id, bool* ok) {
  QSqlQuery q(db);

  q.prepare("INSERT INTO MessageFiltersInFeeds (filter, feed_custom_id, account_id) "
            "VALUES(:filter, :feed_custom_id, :account_id);");

  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed_custom_id"), feed_custom_id);
  q.bindValue(QSL(":account_id"), account_id);
  q.setForwardOnly(true);

  if (q.exec()) {
    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }
}

void DatabaseQueries::updateMessageFilter(const QSqlDatabase& db, MessageFilter* filter, bool* ok) {
  QSqlQuery q(db);

  q.prepare("UPDATE MessageFilters SET name = :name, script = :script WHERE id = :id;");

  q.bindValue(QSL(":name"), filter->name());
  q.bindValue(QSL(":script"), filter->script());
  q.bindValue(QSL(":id"), filter->id());
  q.setForwardOnly(true);

  if (q.exec()) {
    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }
}

void DatabaseQueries::removeMessageFilterFromFeed(const QSqlDatabase& db, const QString& feed_custom_id,
                                                  int filter_id, int account_id, bool* ok) {
  QSqlQuery q(db);

  q.prepare("DELETE FROM MessageFiltersInFeeds "
            "WHERE filter = :filter AND feed_custom_id = :feed_custom_id AND account_id = :account_id;");

  q.bindValue(QSL(":filter"), filter_id);
  q.bindValue(QSL(":feed_custom_id"), feed_custom_id);
  q.bindValue(QSL(":account_id"), account_id);
  q.setForwardOnly(true);

  if (q.exec()) {
    if (ok != nullptr) {
      *ok = true;
    }
  }
  else {
    if (ok != nullptr) {
      *ok = false;
    }
  }
}

QStringList DatabaseQueries::getAllRecipients(const QSqlDatabase& db, int account_id) {
  QSqlQuery query(db);
  QStringList rec;

  query.prepare(QSL("SELECT DISTINCT author "
                    "FROM Messages "
                    "WHERE account_id = :account_id AND author IS NOT NULL AND author != '' "
                    "ORDER BY lower(author) ASC;"));
  query.bindValue(QSL(":account_id"), account_id);

  if (query.exec()) {
    while (query.next()) {
      rec.append(query.value(0).toString());
    }
  }
  else {
    qWarningNN << LOGSEC_GMAIL << "Query for all recipients failed: '" << query.lastError().text() << "'.";
  }

  return rec;
}

bool DatabaseQueries::storeNewOauthTokens(const QSqlDatabase& db,
                                          const QString& refresh_token, int account_id) {
  QSqlQuery query(db);

  query.prepare(QSL("SELECT custom_data FROM Accounts WHERE id = :id;"));
  query.bindValue(QSL(":id"), account_id);

  if (!query.exec() || !query.next()) {
    qWarningNN << LOGSEC_OAUTH
               << "Cannot fetch custom data column for storing of OAuth tokens, because of error:"
               << QUOTE_W_SPACE_DOT(query.lastError().text());
    return false;
  }

  QVariantHash custom_data = deserializeCustomData(query.value(0).toString());

  custom_data["refresh_token"] = refresh_token;

  query.clear();
  query.prepare(QSL("UPDATE Accounts SET custom_data = :custom_data WHERE id = :id;"));
  query.bindValue(QSL(":custom_data"), serializeCustomData(custom_data));
  query.bindValue(QSL(":id"), account_id);

  if (!query.exec()) {
    qWarningNN << LOGSEC_OAUTH
               << "Cannot store OAuth tokens, because of error:"
               << QUOTE_W_SPACE_DOT(query.lastError().text());
    return false;
  }
  else {
    return true;
  }
}

QString DatabaseQueries::unnulifyString(const QString& str) {
  return str.isNull() ? "" : str;
}
