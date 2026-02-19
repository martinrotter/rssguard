// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/rssguard4import.h"

#include "src/definitions.h"
#include "src/standardcategory.h"
#include "src/standardfeed.h"
#include "src/standardserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/database/sqlquery.h>
#include <librssguard/exceptions/sqlexception.h>
#include <librssguard/filtering/messagefilter.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/gui/dialogs/formprogressworker.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/miscellaneous/thread.h>
#include <librssguard/services/abstract/labelsnode.h>
#include <librssguard/services/abstract/searchsnode.h>

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
  QList<MessageFilter*> filters = extractFilters(rssguard4_db);
  QList<Search*> probes = extractProbes(rssguard4_db);

  QList<StandardFeed*> imported_feeds = importTree(feed_tree);

  importLabels(labels);
  importFilters(filters);
  importProbes(probes);

  QMap<QString, Label*> hashed_labels = hashLabels(labels);
  FormProgressWorker d(qApp->mainFormWidget());

  d.doWork<StandardFeed*>(
    tr("Import data from RSS Guard 4.x"),
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

  qDeleteAll(filters);
  delete feed_tree;

  closeDbConnection(rssguard4_db);
}

void RssGuard4Import::importArticles(StandardFeed* feed, const QMap<QString, Label*>& lbls) {
  QSqlDatabase rssguard4_db = dbConnection(m_dbFile, QSL("rssguard4_%1").arg(getThreadID()));
  QList<Message> msgs;

  // Load articles and migrate them to RSS Guard.
  SqlQuery q(rssguard4_db);
  int db_id = feed->property("db_id").toInt();
  int account_id = feed->property("account_id").toInt();

  q.prepare(QSL("SELECT "
                "  Messages.is_read, "
                "  Messages.is_important, "
                "  Messages.is_deleted, "
                "  Messages.is_pdeleted, "
                "  Messages.title, "
                "  Messages.url, "
                "  Messages.author, "
                "  Messages.date_created, "
                "  Messages.contents, "
                "  Messages.enclosures, "
                "  Messages.score, "
                "  Messages.labels "
                "FROM Messages "
                "WHERE Messages.account_id = :account_id AND Messages.feed = :feed_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  q.bindValue(QSL(":feed_id"), QString::number(db_id));
  q.exec();

  while (q.next()) {
    try {
      auto msg = convertArticle(q);
      QStringList label_ids = q.value(11).toString().split(QL1C('.'), SPLIT_BEHAVIOR::SkipEmptyParts);

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
    // lbl->setId(NO_PARENT_CATEGORY);

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

void RssGuard4Import::importFilters(const QList<MessageFilter*>& filters) {
  for (MessageFilter* lbl : filters) {
    qApp->feedReader()->addMessageFilter(lbl->name(), lbl->script());
  }
}

void RssGuard4Import::importProbes(const QList<Search*>& probes) {
  if (probes.isEmpty()) {
    return;
  }

  for (Search* prob : probes) {
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      DatabaseQueries::createProbe(db, prob, m_account->accountId());
    });

    m_account->requestItemReassignment(prob, m_account->probesNode());
  }

  m_account->requestItemExpand({m_account->probesNode()}, true);
}

Message RssGuard4Import::convertArticle(const SqlQuery& rec) const {
  Message msg;

  msg.m_createdFromFeed = true;
  msg.m_created = TextFactory::parseDateTime(rec.value(7).value<qint64>());
  msg.m_author = rec.value(6).toString();
  msg.m_url = rec.value(5).toString();
  msg.m_title = rec.value(4).toString();
  msg.m_contents = rec.value(8).toString();
  msg.m_isImportant = rec.value(1).toBool();
  msg.m_isRead = rec.value(0).toBool();
  msg.m_isDeleted = rec.value(2).toBool();
  msg.m_isPdeleted = rec.value(3).toBool();
  msg.m_enclosures = Enclosures::decodeEnclosuresFromString(rec.value(9).toString());
  msg.m_score = rec.value(10).toDouble();

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
      source_item->setId(NO_PARENT_CATEGORY);

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

        // NOTE: Copy IDs.
        new_feed->setProperty("db_id", source_feed->property("db_id"));
        new_feed->setProperty("account_id", source_feed->property("account_id"));

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
  RootItem::Assignment cats, fds;

  // Categories.
  q.exec(QSL("SELECT"
             "  Categories.account_id, "
             "  Categories.id, "
             "  Categories.parent_id, "
             "  Categories.title, "
             "  Categories.description, "
             "  Categories.icon "
             "FROM Categories "
             "JOIN Accounts ac ON ac.id = Categories.account_id AND ac.type = 'std-rss' "
             "ORDER BY parent_id ASC;"));

  while (q.next()) {
    int account_id = q.value(QSL("account_id")).toInt();
    int id = q.value(QSL("id")).toInt();
    int pid = q.value(QSL("parent_id")).toInt();
    QString title = q.value(QSL("title")).toString();
    QString description = q.value(QSL("title")).toString();
    QByteArray image = q.value(QSL("icon")).toByteArray();
    StandardCategory* cat = new StandardCategory();

    cat->setId(id);
    cat->setProperty("db_id", id);
    cat->setProperty("account_id", account_id);
    cat->setTitle(title);
    cat->setDescription(description);
    cat->setIcon(IconFactory::fromByteArray(image));

    cats.append({pid, cat});
  }

  root->assembleCategories(cats);

  // Feeds.
  q.exec(QSL("SELECT"
             "  Feeds.account_id, "
             "  Feeds.id, "
             "  Feeds.category, "
             "  Feeds.custom_data, "
             "  Feeds.title, "
             "  Feeds.description, "
             "  Feeds.icon, "
             "  Feeds.source "
             "FROM Feeds "
             "JOIN Accounts ac ON ac.id = Feeds.account_id AND ac.type = 'std-rss' "
             "ORDER BY Feeds.ordr ASC;"));

  while (q.next()) {
    int account_id = q.value(QSL("account_id")).toInt();
    int id = q.value(QSL("id")).toInt();
    int pid = q.value(QSL("category")).toInt();
    QString title = q.value(QSL("title")).toString();
    QString source = q.value(QSL("source")).toString();
    QString description = q.value(QSL("title")).toString();
    QByteArray image = q.value(QSL("icon")).toByteArray();
    QString custom_data = q.value(QSL("custom_data")).toString();
    StandardFeed* fd = new StandardFeed();

    fd->setId(id);
    fd->setProperty("db_id", id);
    fd->setProperty("account_id", account_id);
    fd->setTitle(title);
    fd->setDescription(description);
    fd->setSource(source);
    fd->setIcon(IconFactory::fromByteArray(image));
    fd->setCustomDatabaseData(DatabaseQueries::deserializeCustomData(custom_data));

    fds.append({pid, fd});
  }

  root->assembleFeeds(fds);

  return root;
}

QList<Label*> RssGuard4Import::extractLabels(const QSqlDatabase& db) const {
  QList<Label*> lbls;
  SqlQuery q(db);

  q.exec(QSL("SELECT Labels.id, Labels.name, Labels.color, Labels.account_id "
             "FROM Labels "
             "JOIN Accounts ac ON ac.id = Labels.account_id AND ac.type = 'std-rss';"));

  while (q.next()) {
    QString id = q.value(0).toString();
    QString name = q.value(1).toString();
    QString hex_color = q.value(2).toString();
    int account_id = q.value(3).toInt();

    if (!id.isEmpty() && !name.isEmpty()) {
      Label* lbl = new Label(name, IconFactory::fromColor(QColor(hex_color)), nullptr);

      lbl->setProperty("db_id", id);
      lbl->setProperty("account_id", account_id);
      lbl->setCustomId(id);
      lbls.append(lbl);
    }
  }

  return lbls;
}

QList<MessageFilter*> RssGuard4Import::extractFilters(const QSqlDatabase& db) const {
  QList<MessageFilter*> lbls;
  SqlQuery q(db);

  q.exec(QSL("SELECT name, script "
             "FROM MessageFilters;"));

  while (q.next()) {
    auto* fltr = new MessageFilter();
    fltr->setEnabled(true);
    fltr->setName(q.value(0).toString());
    fltr->setScript(q.value(1).toString());

    lbls.append(fltr);
  }

  return lbls;
}

QList<Search*> RssGuard4Import::extractProbes(const QSqlDatabase& db) const {
  QList<Search*> lbls;
  SqlQuery q(db);

  q.exec(QSL("SELECT "
             "  Probes.name, "
             "  Probes.color, "
             "  Probes.fltr "
             "FROM Probes "
             "JOIN Accounts ac ON ac.id = Probes.account_id AND ac.type = 'std-rss';"));

  while (q.next()) {
    QString name = q.value(0).toString();
    QString hex_color = q.value(1).toString();
    QString fltr = q.value(2).toString();

    lbls.append(new Search(name, Search::Type::Regex, fltr, IconFactory::fromColor(hex_color)));
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

  QStringList tables =
    QStringList{QSL("Categories"), QSL("Feeds"), QSL("Labels"), QSL("MessageFilters"), QSL("Messages"), QSL("Probes")};

  while (q.next()) {
    tables.removeOne(q.value(0).toString());
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
