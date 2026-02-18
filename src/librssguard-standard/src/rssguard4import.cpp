// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/rssguard4import.h"

#include "src/definitions.h"
#include "src/standardcategory.h"
#include "src/standardfeed.h"
#include "src/standardserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/database/sqlquery.h>
#include <librssguard/exceptions/sqlexception.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/gui/dialogs/formprogressworker.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/miscellaneous/thread.h>
#include <librssguard/services/abstract/labelsnode.h>

#include <QSqlError>
#include <QStack>
#include <QtConcurrentMap>

RssGuard4Import::RssGuard4Import(StandardServiceRoot* account, QObject* parent) : QObject(parent), m_account(account) {}

void RssGuard4Import::import() {
  m_dbFile = FileDialog::openFileName(qApp->mainFormWidget(),
                                      tr("Select RSS Guard 4.x database file"),
                                      qApp->homeFolder(),
                                      QSL("database.db"),
                                      tr("RSS Guard 4.x DB files (%1)").arg(QSL("*.db")),
                                      nullptr,
                                      GENERAL_REMEMBERED_PATH);

  if (m_dbFile.isEmpty() || !QFile::exists(m_dbFile)) {
    m_dbFile.clear();
    return;
  }

  QSqlDatabase rssguard4_db = dbConnection(m_dbFile, QSL("rssguard4"));

  checkIfRssGuard4(rssguard4_db);

  RootItem* feed_tree = extractFeedsAndCategories(rssguard4_db);
  QList<Label*> labels = extractLabels(rssguard4_db);
  QList<StandardFeed*> imported_feeds = importTree(feed_tree);

  importLabels(labels);

  QMap<QString, Label*> hashed_labels = hashLabels(labels);

  FormProgressWorker d(qApp->mainFormWidget());

  d.doWork<StandardFeed*>(
    tr("Import data from QuiteRSS"),
    true,
    imported_feeds,
    [&](StandardFeed* fd) {
      importArticles(fd, hashed_labels);
    },
    [](int progress) {
      return tr("Imported articles from %1 feeds...").arg(progress);
    });

  m_account->updateCounts();
  m_account->itemChanged(m_account->getSubTree<RootItem>());

  delete feed_tree;

  closeDbConnection(rssguard4_db);
}

void RssGuard4Import::importArticles(StandardFeed* feed, const QMap<QString, Label*>& lbls) {
  QSqlDatabase quiterss_db = dbConnection(m_dbFile, QSL("quiterss_%1").arg(getThreadID()));
  QList<Message> msgs;

  // Load articles and migrate them to RSS Guard.
  SqlQuery q(quiterss_db);
  int quiterss_id = feed->property("quiterss_id").toInt();

  q.prepare(QSL("SELECT guid, description, title, published, author_name, link_href, read, starred, label "
                "FROM news "
                "WHERE feedId = :feed_id;"));
  q.bindValue(QSL(":feed_id"), quiterss_id);
  q.exec();

  while (q.next()) {
    try {
      auto msg = convertArticle(q);
      QStringList label_ids = q.value(8).toString().split(QL1C(','), SPLIT_BEHAVIOR::SkipEmptyParts);

      for (const QString& label_id : std::as_const(label_ids)) {
        auto* target_lbl = lbls.value(label_id);

        if (target_lbl != nullptr) {
          msg.m_assignedLabelsByFilter.append(target_lbl);
        }
      }

      msg.sanitize(feed, false);
      msgs.append(msg);
    }
    catch (const ApplicationException& ex) {
      qWarningNN << LOGSEC_STANDARD << "Article was not converted:" << QUOTE_W_SPACE_DOT(ex.message());
    }
  }

  qDebugNN << LOGSEC_STANDARD << "Collected" << NONQUOTE_W_SPACE(msgs.size()) << "articles for QuiteRSS import for feed"
           << NONQUOTE_W_SPACE_DOT(feed->title());

  if (msgs.isEmpty()) {
    return;
  }

  try {
    DatabaseQueries::updateMessages(msgs, feed, false, true);
  }
  catch (const ApplicationException& ex) {
    qWarningNN << LOGSEC_STANDARD << "Article import from quiterss failed:" << QUOTE_W_SPACE_DOT(ex.message());
  }
}

void RssGuard4Import::importLabels(const QList<Label*>& labels) {
  if (labels.isEmpty()) {
    return;
  }

  for (Label* lbl : labels) {
    try {
      qApp->database()->worker()->write([&](const QSqlDatabase& db) {
        DatabaseQueries::createLabel(db, lbl, m_account->accountId());
      });

      m_account->requestItemReassignment(lbl, m_account->labelsNode());
    }
    catch (const SqlException& ex) {
      qCriticalNN << LOGSEC_STANDARD << "Failed to import quiterss label:" << QUOTE_W_SPACE_DOT(ex.message());
    }
  }

  m_account->requestItemExpand({m_account->labelsNode()}, true);
}

Message RssGuard4Import::convertArticle(const SqlQuery& rec) const {
  Message msg;
  QString dt_format = QSL("yyyy-MM-ddTHH:mm:ss");

  msg.m_created = QDateTime::currentDateTime();
  msg.m_customId = rec.value(QSL("guid")).toString();
  msg.m_author = rec.value(QSL("author_name")).toString();
  msg.m_url = rec.value(QSL("link_href")).toString();
  msg.m_title = rec.value(QSL("title")).toString();
  msg.m_contents = rec.value(QSL("description")).toString();
  msg.m_isImportant = rec.value(QSL("starred")).toBool();
  msg.m_isRead = rec.value(QSL("read")).toBool();
  msg.m_created = TextFactory::parseDateTime(rec.value(QSL("published")).toString(), &dt_format);
  msg.m_createdFromFeed = !msg.m_created.isNull();

  if (!msg.m_createdFromFeed) {
    msg.m_created = QDateTime::currentDateTimeUtc();
  }

  if (msg.m_title.trimmed().isEmpty()) {
    if (msg.m_url.trimmed().isEmpty()) {
      throw ApplicationException(tr("skipping article, it has no title and no URL"));
    }
    else {
      msg.m_title = msg.m_url;
    }
  }

  return msg;
}

QMap<QString, Label*> RssGuard4Import::hashLabels(const QList<Label*>& labels) const {
  QMap<QString, Label*> map;

  for (Label* lbl : labels) {
    map.insert(lbl->customId(), lbl);
  }

  return map;
}

QList<StandardFeed*> RssGuard4Import::importTree(RootItem* root) const {
  QList<StandardFeed*> feeds;
  QStack<RootItem*> original_parents;
  QStack<RootItem*> new_parents;
  RootItem* target_root_node = m_account;

  original_parents.push(target_root_node);
  new_parents.push(root);

  // Iterate all new items we would like to merge into current model.
  while (!new_parents.isEmpty()) {
    RootItem* target_parent = original_parents.pop();
    RootItem* source_parent = new_parents.pop();
    auto sour_chi = source_parent->childItems();

    for (RootItem* source_item : std::as_const(sour_chi)) {
      if (source_item->kind() == RootItem::Kind::Category) {
        auto* source_category = qobject_cast<StandardCategory*>(source_item);
        auto* new_category = new StandardCategory(*source_category);
        QString new_category_title = new_category->title();

        // Add category to model.
        new_category->clearChildren();

        try {
          qApp->database()->worker()->write([&](const QSqlDatabase& db) {
            DatabaseQueries::createOverwriteCategory(db, new_category, m_account->accountId(), target_parent->id());
          });

          m_account->requestItemReassignment(new_category, target_parent);

          original_parents.push(new_category);
          new_parents.push(source_category);
        }
        catch (ApplicationException& ex) {
          // Add category failed, but this can mean that the same category (with same title)
          // already exists. If such a category exists in current parent, then find it and
          // add descendants to it.
          RootItem* existing_category = nullptr;
          auto tar_chi = target_parent->childItems();

          for (RootItem* child : std::as_const(tar_chi)) {
            if (child->kind() == RootItem::Kind::Category && child->title() == new_category_title) {
              existing_category = child;
            }
          }

          if (existing_category != nullptr) {
            original_parents.push(existing_category);
            new_parents.push(source_category);
          }
          else {
            qCriticalNN << LOGSEC_STANDARD << "Cannot import category:" << QUOTE_W_SPACE_DOT(ex.message());
          }
        }
      }
      else if (source_item->kind() == RootItem::Kind::Feed) {
        auto* source_feed = qobject_cast<StandardFeed*>(source_item);
        const auto* feed_with_same_url = target_root_node->getItemFromSubTree([source_feed](const RootItem* it) {
          return it->kind() == RootItem::Kind::Feed &&
                 it->toFeed()->source().toLower() == source_feed->source().toLower();
        });

        if (feed_with_same_url != nullptr) {
          continue;
        }

        auto* new_feed = new StandardFeed(*source_feed);

        // NOTE: Copy QuiteRSS DB ID.
        new_feed->setProperty("quiterss_id", source_feed->property("quiterss_id"));

        try {
          qApp->database()->worker()->write([&](const QSqlDatabase& db) {
            DatabaseQueries::createOverwriteFeed(db, new_feed, m_account->accountId(), target_parent->id());
          });

          m_account->requestItemReassignment(new_feed, target_parent);
          feeds.append(new_feed);
        }
        catch (const ApplicationException& ex) {
          qCriticalNN << LOGSEC_STANDARD << "Cannot import feed:" << QUOTE_W_SPACE_DOT(ex.message());
        }
      }
    }
  }

  return feeds;
}

RootItem* RssGuard4Import::extractFeedsAndCategories(const QSqlDatabase& db) const {
  RootItem* root = new RootItem();
  SqlQuery q(db);
  /*
    q.exec(QSL("SELECT"
               "  id, "
               "  parent_id, "
               "  title, "
               "  description, "
               "  icon"
               "FROM Categories "
               "ORDER parent_id ASC;"));

    while (q.next()) {
      int id = q.value(QSL("id")).toInt();
      int pid = q.value(QSL("parent_id")).toInt();
      QString title = q.value(QSL("title")).toString();
      QString description = q.value(QSL("title")).toString();
      QString image = q.value(QSL("image")).toString();
      StandardCategory* cat = new StandardCategory(root);

      new_item = cat;

      new_item->setProperty("quiterss_id", id);
      new_item->setTitle(title);
      new_item->setDescription(description);
    }
    */

  return root;
}

QList<Label*> RssGuard4Import::extractLabels(const QSqlDatabase& db) const {
  QList<Label*> lbls;
  SqlQuery q(db);

  q.exec(QSL("SELECT id, name FROM labels;"));

  while (q.next()) {
    QString id = q.value(0).toString();
    QString name = q.value(1).toString();

    if (!id.isEmpty() && !name.isEmpty()) {
      Label* lbl = new Label(name, IconFactory::generateIcon(TextFactory::generateColorFromText(name)), nullptr);

      lbl->setCustomId(id);
      lbls.append(lbl);
    }
  }

  return lbls;
}

QIcon RssGuard4Import::decodeBase64Icon(const QString& base64) const {
  if (base64.isEmpty()) {
    return QIcon();
  }
  else {
    auto data = QByteArray::fromBase64(base64.toLocal8Bit());
    QPixmap px;

    if (!px.loadFromData(data)) {
      qWarningNN << LOGSEC_STANDARD << "Failed to convert QuiteRSS image.";
    }

    return QIcon(px);
  }
}

void RssGuard4Import::checkIfRssGuard4(const QSqlDatabase& db) const {
  SqlQuery q(db);

  q.exec(QSL("SELECT name FROM sqlite_master WHERE type='table';"));

  QStringList tables = QStringList{QSL("feeds"),
                                   QSL("news"),
                                   QSL("feeds_ex"),
                                   QSL("news_ex"),
                                   QSL("filters"),
                                   QSL("filterconditions"),
                                   QSL("filteractions"),
                                   QSL("filters_ex"),
                                   QSL("labels"),
                                   QSL("passwords"),
                                   QSL("info")};

  while (q.next()) {
    tables.removeOne(q.value(0).toString().toLower());
  }

  if (!tables.isEmpty()) {
    throw ApplicationException(tr("missing RSS Guard 4.x tables %1").arg(tables.join(QSL(", "))));
  }

  q.exec(QSL("SELECT inf_value FROM Information WHERE inf_key = 'schema_version';"));

  if (!q.next() || q.value(0).toString() != QSL("10")) {
    throw ApplicationException(tr("metadata version 10 was expected, your DB file comes from too old RSS Guard 4.x"));
  }
}

QSqlDatabase RssGuard4Import::dbConnection(const QString& db_file, const QString& connection_name) const {
  QSqlDatabase db;

  if (QSqlDatabase::contains(connection_name)) {
    db = QSqlDatabase::database(connection_name);
  }
  else {
    db = QSqlDatabase::addDatabase(QSL(APP_DB_SQLITE_DRIVER), connection_name);
    db.setDatabaseName(db_file);
  }

  if (!db.isOpen() && !db.open()) {
    throw ApplicationException(db.lastError().text());
  }

  return db;
}

void RssGuard4Import::closeDbConnection(QSqlDatabase& db) const {
  db.close();
  QSqlDatabase::removeDatabase(db.connectionName());
}
