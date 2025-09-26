// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/quiterssimport.h"

#include "src/definitions.h"
#include "src/standardcategory.h"
#include "src/standardfeed.h"
#include "src/standardserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/thread.h>

#include <QProgressDialog>
#include <QSqlError>
#include <QStack>

QuiteRssImport::QuiteRssImport(StandardServiceRoot* account, QObject* parent) : QObject(parent), m_account(account) {}

void QuiteRssImport::import() {
  m_dbFile = FileDialog::openFileName(qApp->mainFormWidget(),
                                      tr("Select QuiteRSS database file"),
                                      qApp->homeFolder(),
                                      tr("QuiteRSS DB files (%1)").arg(QSL("*.db")),
                                      nullptr,
                                      GENERAL_REMEMBERED_PATH);

  if (m_dbFile.isEmpty() || !QFile::exists(m_dbFile)) {
    m_dbFile.clear();
    return;
  }

  QSqlDatabase quiterss_db = dbConnection(m_dbFile, QSL("quiterss"));
  QSqlDatabase rssguard_db = qApp->database()->driver()->connection(metaObject()->className());

  checkIfQuiteRss(quiterss_db);

  RootItem* feed_tree = extractFeedsAndCategories(quiterss_db);
  QList<StandardFeed*> imported_feeds = importTree(rssguard_db, feed_tree);

  QProgressDialog d;

  d.setMaximum(imported_feeds.size());
  d.setMinimum(0);
  d.show();

  int i = 0;

  for (StandardFeed* feed : imported_feeds) {
    importArticles(feed);
    d.setValue(i++);
    qApp->processEvents();
  }

  m_account->updateCounts(true);
  m_account->itemChanged(m_account->getSubTree<RootItem>());

  delete feed_tree;

  closeDbConnection(quiterss_db);
}

void QuiteRssImport::importArticles(StandardFeed* feed) {
  QSqlDatabase quiterss_db = dbConnection(m_dbFile, QSL("quiterss_%1").arg(getThreadID()));
  QSqlDatabase rssguard_db = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  QList<Message> msgs;

  // Load articles and migrate them to RSS Guard.
  QSqlQuery q(quiterss_db);
  int quiterss_id = feed->property("quiterss_id").toInt();

  q.prepare(QSL("SELECT guid, description, title, published, author_name, link_href, read, starred "
                "FROM news "
                "WHERE feedId = :feed_id AND deleted = 0;"));
  q.bindValue(QSL(":feed_id"), quiterss_id);
  q.exec();

  while (q.next()) {
    auto msg = convertArticle(q.record());

    msg.sanitize(feed, false);
    msgs.append(msg);
  }

  qDebugNN << LOGSEC_STANDARD << "Collected" << NONQUOTE_W_SPACE(msgs.size()) << "articles for QuiteRSS import for feed"
           << NONQUOTE_W_SPACE_DOT(feed->title());

  if (msgs.isEmpty()) {
    return;
  }

  DatabaseQueries::updateMessages(rssguard_db, msgs, feed->toFeed(), false, true, nullptr);
}

Message QuiteRssImport::convertArticle(const QSqlRecord& rec) const {
  Message msg;

  msg.m_created = QDateTime::currentDateTime();
  msg.m_customId = rec.value(QSL("guid")).toString();
  msg.m_author = rec.value(QSL("author_name")).toString();
  msg.m_url = rec.value(QSL("link_href")).toString();
  msg.m_title = rec.value(QSL("title")).toString();
  msg.m_contents = rec.value(QSL("description")).toString();
  msg.m_isImportant = rec.value(QSL("starred")).toBool();
  msg.m_isRead = rec.value(QSL("read")).toBool();

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

QList<StandardFeed*> QuiteRssImport::importTree(QSqlDatabase& db, RootItem* root) const {
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
          DatabaseQueries::createOverwriteCategory(db, new_category, m_account->accountId(), target_parent->id());
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
          DatabaseQueries::createOverwriteFeed(db, new_feed, m_account->accountId(), target_parent->id());
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

RootItem* QuiteRssImport::extractFeedsAndCategories(QSqlDatabase& db) const {
  RootItem* root = new RootItem();
  QMap<int, RootItem*> roots;        // Map of all items (feeds, categories) with their original DB ID.
  QMultiMap<int, RootItem*> parents; // Map which assigns all items to PARENT DB ID.

  roots.insert(0, root);

  QSqlQuery q(db);

  if (!q.exec(QSL("SELECT id, "
                  "  text, "
                  "  title, "
                  "  description, "
                  "  xmlUrl, "
                  "  image, "
                  "  parentId "
                  "FROM feeds "
                  "ORDER BY xmlUrl ASC, parentId ASC;"))) {
    throw ApplicationException(q.lastError().text());
  }

  while (q.next()) {
    int id = q.value(QSL("id")).toInt();
    int pid = q.value(QSL("parentId")).toInt();
    QString xml_url = q.value(QSL("xmlUrl")).toString();
    QString title = q.value(QSL("title")).toString();

    if (title.isEmpty()) {
      title = q.value(QSL("text")).toString();

      if (title.isEmpty()) {
        title = tr("Unnamed item");
      }
    }

    QString description = q.value(QSL("description")).toString();
    QString image = q.value(QSL("image")).toString();

    RootItem* new_item = nullptr;

    if (xml_url.isEmpty()) {
      // We have category.
      StandardCategory* cat = new StandardCategory(root);

      new_item = cat;
    }
    else {
      // We have feed.
      StandardFeed* fd = new StandardFeed(root);

      fd->setSource(xml_url);
      fd->setType(xml_url.contains(QSL("atom")) ? StandardFeed::Type::Atom10 : StandardFeed::Type::Rss2X);

      new_item = fd;
    }

    new_item->setProperty("quiterss_id", id);
    new_item->setTitle(title);
    new_item->setDescription(description);
    new_item->setIcon(decodeBase64Icon(image));

    roots.insert(id, new_item);
    parents.insert(pid, new_item);
  }

  for (int pid : parents.uniqueKeys()) {
    RootItem* parent = roots.value(pid);
    QList<RootItem*> childs = parents.values(pid);

    if (parent == nullptr) {
      parent = root;
    }

    for (RootItem* child : childs) {
      parent->appendChild(child);
    }
  }

  return root;
}

QIcon QuiteRssImport::decodeBase64Icon(const QString& base64) const {
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

void QuiteRssImport::checkIfQuiteRss(QSqlDatabase& db) const {
  QSqlQuery q(db);

  if (!q.exec(QSL("SELECT name FROM sqlite_master WHERE type='table';"))) {
    throw ApplicationException(q.lastError().text());
  }

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
    throw ApplicationException(tr("missing QuiteRSS tables %1").arg(tables.join(QSL(", "))));
  }

  if (!q.exec(QSL("SELECT value FROM info WHERE name = \"version\";")) || !q.next() ||
      q.value(0).toString() != QSL("17")) {
    throw ApplicationException(tr("metadata version 17 was expected"));
  }
}

QSqlDatabase QuiteRssImport::dbConnection(const QString& db_file, const QString& connection_name) const {
  QSqlDatabase db = QSqlDatabase::addDatabase(QSL(APP_DB_SQLITE_DRIVER), connection_name);

  db.setDatabaseName(db_file);

  if (!db.open()) {
    throw ApplicationException(db.lastError().text());
  }

  return db;
}

void QuiteRssImport::closeDbConnection(QSqlDatabase& db) const {
  db.close();
  QSqlDatabase::removeDatabase(db.connectionName());
}
