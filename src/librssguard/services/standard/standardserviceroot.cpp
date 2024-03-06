// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardserviceroot.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedfetchexception.h"
#include "exceptions/scriptexception.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "network-web/networkfactory.h"

#include "services/abstract/gui/formcategorydetails.h"
#include "services/standard/definitions.h"
#include "services/standard/gui/formdiscoverfeeds.h"
#include "services/standard/gui/formeditstandardaccount.h"
#include "services/standard/gui/formstandardfeeddetails.h"
#include "services/standard/gui/formstandardimportexport.h"
#include "services/standard/parsers/atomparser.h"
#include "services/standard/parsers/jsonparser.h"
#include "services/standard/parsers/rdfparser.h"
#include "services/standard/parsers/rssparser.h"
#include "services/standard/parsers/sitemapparser.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardfeedsimportexportmodel.h"
#include "services/standard/standardserviceentrypoint.h"

#if defined(NO_LITE)
#include "gui/webviewers/webengine/webengineviewer.h"
#include "network-web/webengine/webenginepage.h"
#endif

#if defined(ENABLE_COMPRESSED_SITEMAP)
#include "3rd-party/qcompressor/qcompressor.h"
#endif

#include <QAction>
#include <QSqlTableModel>
#include <QStack>
#include <QTextCodec>

StandardServiceRoot::StandardServiceRoot(RootItem* parent) : ServiceRoot(parent) {
  setIcon(StandardServiceEntryPoint().icon());
  setDescription(tr("This is the obligatory service account for standard RSS/RDF/ATOM feeds."));
}

StandardServiceRoot::~StandardServiceRoot() {
  qDeleteAll(m_feedContextMenu);
}

void StandardServiceRoot::onDatabaseCleanup() {
  for (Feed* fd : getSubTreeFeeds()) {
    qobject_cast<StandardFeed*>(fd)->setLastEtag({});
  }
}

void StandardServiceRoot::start(bool freshly_activated) {
  DatabaseQueries::loadRootFromDatabase<StandardCategory, StandardFeed>(this);

  if (freshly_activated && getSubTreeFeeds().isEmpty()) {
    // In other words, if there are no feeds or categories added.
    if (MsgBox::show(qApp->mainFormWidget(),
                     QMessageBox::Question,
                     QObject::tr("Load initial set of feeds"),
                     tr("This new account does not include any feeds. You can now add default set of feeds."),
                     tr("Do you want to load initial set of feeds?"),
                     QString(),
                     QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      QString target_opml_file = APP_INITIAL_FEEDS_PATH + QDir::separator() + FEED_INITIAL_OPML_PATTERN;
      QString current_locale = qApp->localization()->loadedLanguage();
      QString file_to_load;

      if (QFile::exists(target_opml_file.arg(current_locale))) {
        file_to_load = target_opml_file.arg(current_locale);
      }
      else if (QFile::exists(target_opml_file.arg(QSL(DEFAULT_LOCALE)))) {
        file_to_load = target_opml_file.arg(QSL(DEFAULT_LOCALE));
      }

      FeedsImportExportModel model(this);
      QString output_msg;

      try {
        model.importAsOPML20(IOFactory::readFile(file_to_load), false, false, false);
        model.checkAllItems();

        if (mergeImportExportModel(&model, this, output_msg)) {
          requestItemExpand(getSubTree(), true);
        }
      }
      catch (ApplicationException& ex) {
        MsgBox::show(qApp->mainFormWidget(),
                     QMessageBox::Critical,
                     tr("Error when loading initial feeds"),
                     ex.message());
      }
    }
    else {
      requestItemExpand({this}, true);
    }
  }
}

void StandardServiceRoot::stop() {
  qDebugNN << LOGSEC_CORE << "Stopping StandardServiceRoot instance.";
}

QString StandardServiceRoot::code() const {
  return StandardServiceEntryPoint().code();
}

bool StandardServiceRoot::canBeEdited() const {
  return true;
}

FormAccountDetails* StandardServiceRoot::accountSetupDialog() const {
  return new FormEditStandardAccount(qApp->mainFormWidget());
}

void StandardServiceRoot::editItems(const QList<RootItem*>& items) {
  auto std_feeds = boolinq::from(items)
                     .select([](RootItem* it) {
                       return qobject_cast<Feed*>(it);
                     })
                     .where([](Feed* fd) {
                       return fd != nullptr;
                     })
                     .toStdList();

  if (!std_feeds.empty()) {
    QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(this,
                                                                                     nullptr,
                                                                                     {},
                                                                                     qApp->mainFormWidget()));

    form_pointer->addEditFeed<StandardFeed>(FROM_STD_LIST(QList<Feed*>, std_feeds));
    return;
  }

  if (items.first()->kind() == RootItem::Kind::ServiceRoot) {
    QScopedPointer<FormEditStandardAccount> p(qobject_cast<FormEditStandardAccount*>(accountSetupDialog()));

    p->addEditAccount(this);
    return;
  }

  ServiceRoot::editItems(items);
}

bool StandardServiceRoot::supportsFeedAdding() const {
  return true;
}

bool StandardServiceRoot::supportsCategoryAdding() const {
  return true;
}

void StandardServiceRoot::addNewFeed(RootItem* selected_item, const QString& url) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot add item"),
                          tr("Cannot add feed because another critical operation is ongoing."),
                          QSystemTrayIcon::MessageIcon::Warning});

    return;
  }

  QScopedPointer<FormDiscoverFeeds> form_discover(new FormDiscoverFeeds(this,
                                                                        selected_item,
                                                                        url,
                                                                        qApp->mainFormWidget()));

  if (form_discover->exec() == ADVANCED_FEED_ADD_DIALOG_CODE) {
    QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(this,
                                                                                     selected_item,
                                                                                     url,
                                                                                     qApp->mainFormWidget()));

    form_pointer->addEditFeed<StandardFeed>();
  }

  qApp->feedUpdateLock()->unlock();
}

Qt::ItemFlags StandardServiceRoot::additionalFlags() const {
  return Qt::ItemFlag::ItemIsDropEnabled;
}

QList<Message> StandardServiceRoot::obtainNewMessages(Feed* feed,
                                                      const QHash<ServiceRoot::BagOfMessages, QStringList>&
                                                        stated_messages,
                                                      const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(stated_messages)
  Q_UNUSED(tagged_messages)

  StandardFeed* f = static_cast<StandardFeed*>(feed);
  QByteArray feed_contents;
  QString formatted_feed_contents;
  int download_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  if (f->sourceType() == StandardFeed::SourceType::Url) {
    qDebugNN << LOGSEC_CORE << "Downloading URL" << QUOTE_W_SPACE(feed->source()) << "to obtain feed data.";

    QList<QPair<QByteArray, QByteArray>> headers;

    headers << NetworkFactory::generateBasicAuthHeader(f->protection(), f->username(), f->password());

    if (!f->lastEtag().isEmpty()) {
      headers.append({QSL("If-None-Match").toLocal8Bit(), f->lastEtag().toLocal8Bit()});

      qDebugNN << "Using ETag value:" << QUOTE_W_SPACE_DOT(f->lastEtag());
    }

    auto network_result = NetworkFactory::performNetworkOperation(feed->source(),
                                                                  download_timeout,
                                                                  {},
                                                                  feed_contents,
                                                                  QNetworkAccessManager::Operation::GetOperation,
                                                                  headers,
                                                                  false,
                                                                  {},
                                                                  {},
                                                                  networkProxy());

    if (network_result.m_networkError != QNetworkReply::NetworkError::NoError) {
      qWarningNN << LOGSEC_CORE << "Error" << QUOTE_W_SPACE(network_result.m_networkError)
                 << "during fetching of new messages for feed" << QUOTE_W_SPACE_DOT(feed->source());
      throw FeedFetchException(Feed::Status::NetworkError,
                               NetworkFactory::networkErrorText(network_result.m_networkError));
    }
    else {
      f->setLastEtag(network_result.m_headers.value(QSL("etag")));
    }
  }
  else if (f->sourceType() == StandardFeed::SourceType::EmbeddedBrowser) {
#if defined(NO_LITE)
    WebEnginePage* page = new WebEnginePage();
    WebEngineViewer* viewer = nullptr;

    QMetaObject::invokeMethod(
      qApp,
      [&] {
        // NOTE: Must be create on main thread.
        viewer = new WebEngineViewer();
      },
      Qt::ConnectionType::BlockingQueuedConnection);

    viewer->moveToThread(qApp->thread());
    page->moveToThread(qApp->thread());

    viewer->setPage(page);
    viewer->setAttribute(Qt::WA_DontShowOnScreen);

    QMetaObject::invokeMethod(viewer, "show", Qt::ConnectionType::BlockingQueuedConnection);

    QString html;
    QMetaObject::invokeMethod(page,
                              "pageHtml",
                              Qt::ConnectionType::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString, html),
                              Q_ARG(QString, f->source()));

    feed_contents = html.toUtf8();

    page->deleteLater();
    viewer->deleteLater();
#else
    throw ApplicationException(tr("this source type cannot be used on 'lite' %1 build").arg(QSL(APP_NAME)));
#endif
  }
  else if (f->sourceType() == StandardFeed::SourceType::LocalFile) {
    feed_contents = IOFactory::readFile(feed->source());
  }
  else {
    qDebugNN << LOGSEC_CORE << "Running custom script" << QUOTE_W_SPACE(feed->source()) << "to obtain feed data.";

    // Use script to generate feed file.
    try {
      feed_contents = StandardFeed::generateFeedFileWithScript(feed->source(), download_timeout);
    }
    catch (const ScriptException& ex) {
      qCriticalNN << LOGSEC_CORE << "Custom script for generating feed file failed:" << QUOTE_W_SPACE_DOT(ex.message());

      throw FeedFetchException(Feed::Status::OtherError, ex.message());
    }
  }

  // Sitemap parser supports gzip-encoded data too.
  // We need to decode it here before encoding
  // stuff kicks in.
  if (SitemapParser::isGzip(feed_contents)) {
#if defined(ENABLE_COMPRESSED_SITEMAP)
    qWarningNN << LOGSEC_CORE << "Decompressing gzipped feed data.";

    QByteArray uncompressed_feed_contents;

    if (!QCompressor::gzipDecompress(feed_contents, uncompressed_feed_contents)) {
      throw ApplicationException("gzip decompression failed");
    }

    feed_contents = uncompressed_feed_contents;
#else
    qWarningNN << LOGSEC_CORE << "This feed is gzipped.";
#endif
  }

  if (!f->postProcessScript().simplified().isEmpty()) {
    qDebugNN << LOGSEC_CORE << "We will process feed data with post-process script"
             << QUOTE_W_SPACE_DOT(f->postProcessScript());

    try {
      feed_contents =
        StandardFeed::postProcessFeedFileWithScript(f->postProcessScript(), feed_contents, download_timeout);
    }
    catch (const ScriptException& ex) {
      qCriticalNN << LOGSEC_CORE << "Post-processing script for feed file failed:" << QUOTE_W_SPACE_DOT(ex.message());

      throw FeedFetchException(Feed::Status::OtherError, ex.message());
    }
  }

  // Encode obtained data for further parsing.
  QTextCodec* codec = QTextCodec::codecForName(f->encoding().toLocal8Bit());

  if (codec == nullptr) {
    // No suitable codec for this encoding was found.
    // Use UTF-8.
    formatted_feed_contents = QString::fromUtf8(feed_contents);
  }
  else {
    formatted_feed_contents = codec->toUnicode(feed_contents);
  }

  // Feed data are downloaded and encoded.
  // Parse data and obtain messages.
  QList<Message> messages;

  switch (f->type()) {
    case StandardFeed::Type::Rss0X:
    case StandardFeed::Type::Rss2X:
      messages = RssParser(formatted_feed_contents).messages();
      break;

    case StandardFeed::Type::Rdf:
      messages = RdfParser(formatted_feed_contents).messages();
      break;

    case StandardFeed::Type::Atom10:
      messages = AtomParser(formatted_feed_contents).messages();
      break;

    case StandardFeed::Type::Json:
      messages = JsonParser(formatted_feed_contents).messages();
      break;

    case StandardFeed::Type::Sitemap:
      messages = SitemapParser(formatted_feed_contents).messages();

    default:
      break;
  }

  for (Message& mess : messages) {
    mess.m_feedId = feed->customId();
  }

  return messages;
}

QList<QAction*> StandardServiceRoot::getContextMenuForFeed(StandardFeed* feed) {
  if (m_feedContextMenu.isEmpty()) {
    // Initialize.
    auto* action_metadata =
      new QAction(qApp->icons()->fromTheme(QSL("download"), QSL("emblem-downloads")), tr("Fetch metadata"), this);

    m_feedContextMenu.append(action_metadata);

    connect(action_metadata, &QAction::triggered, this, [this]() {
      m_feedForMetadata->fetchMetadataForItself();
    });
  }

  m_feedForMetadata = feed;

  return m_feedContextMenu;
}

QVariantHash StandardServiceRoot::customDatabaseData() const {
  QVariantHash data = ServiceRoot::customDatabaseData();

  data[QSL("title")] = title();
  data[QSL("icon")] = IconFactory::toByteArray(icon());

  return data;
}

void StandardServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  ServiceRoot::setCustomDatabaseData(data);

  setTitle(data.value(QSL("title"), defaultTitle()).toString());

  QByteArray icon_data = data.value(QSL("icon")).toByteArray();

  if (!icon_data.isEmpty()) {
    setIcon(IconFactory::fromByteArray(icon_data));
  }
}

QString StandardServiceRoot::defaultTitle() {
  return qApp->system()->loggedInUser() + QSL(" (RSS/ATOM/JSON)");
}

bool StandardServiceRoot::mergeImportExportModel(FeedsImportExportModel* model,
                                                 RootItem* target_root_node,
                                                 QString& output_message) {
  QStack<RootItem*> original_parents;

  original_parents.push(target_root_node);
  QStack<RootItem*> new_parents;

  new_parents.push(model->sourceModel()->rootItem());
  bool some_feed_category_error = false;

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  // Iterate all new items we would like to merge into current model.
  while (!new_parents.isEmpty()) {
    RootItem* target_parent = original_parents.pop();
    RootItem* source_parent = new_parents.pop();
    auto sour_chi = source_parent->childItems();

    for (RootItem* source_item : std::as_const(sour_chi)) {
      if (!model->sourceModel()->isItemChecked(source_item)) {
        // We can skip this item, because it is not checked and should not be imported.
        // NOTE: All descendants are thus skipped too.
        continue;
      }

      if (source_item->kind() == RootItem::Kind::Category) {
        auto* source_category = qobject_cast<StandardCategory*>(source_item);
        auto* new_category = new StandardCategory(*source_category);
        QString new_category_title = new_category->title();

        // Add category to model.
        new_category->clearChildren();

        try {
          DatabaseQueries::createOverwriteCategory(database,
                                                   new_category,
                                                   target_root_node->getParentServiceRoot()->accountId(),
                                                   target_parent->id());
          requestItemReassignment(new_category, target_parent);

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
            some_feed_category_error = true;

            qCriticalNN << LOGSEC_CORE << "Cannot import category:" << QUOTE_W_SPACE_DOT(ex.message());
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

        try {
          DatabaseQueries::createOverwriteFeed(database,
                                               new_feed,
                                               target_root_node->getParentServiceRoot()->accountId(),
                                               target_parent->id());
          requestItemReassignment(new_feed, target_parent);
        }
        catch (const ApplicationException& ex) {
          qCriticalNN << LOGSEC_CORE << "Cannot import feed:" << QUOTE_W_SPACE_DOT(ex.message());
          some_feed_category_error = true;
        }
      }
    }
  }

  if (some_feed_category_error) {
    output_message = tr("Some feeds/categories were not imported due to error, check debug log for more details.");
  }
  else {
    output_message = tr("Import was completely successful.");
  }

  return !some_feed_category_error;
}

void StandardServiceRoot::addNewCategory(RootItem* selected_item) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot add category"),
                          tr("Cannot add category because another critical operation is ongoing."),
                          QSystemTrayIcon::MessageIcon::Warning});

    // Thus, cannot delete and quit the method.
    return;
  }

  QScopedPointer<FormCategoryDetails> form_pointer(new FormCategoryDetails(this,
                                                                           selected_item,
                                                                           qApp->mainFormWidget()));

  form_pointer->addEditCategory<StandardCategory>();
  qApp->feedUpdateLock()->unlock();
}

void StandardServiceRoot::importFeeds() {
  QScopedPointer<FormStandardImportExport> form(new FormStandardImportExport(this, qApp->mainFormWidget()));

  form.data()->setMode(FeedsImportExportModel::Mode::Import);
  form.data()->exec();
}

void StandardServiceRoot::exportFeeds() {
  QScopedPointer<FormStandardImportExport> form(new FormStandardImportExport(this, qApp->mainFormWidget()));

  form.data()->setMode(FeedsImportExportModel::Mode::Export);
  form.data()->exec();
}

QList<QAction*> StandardServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    ServiceRoot::serviceMenu();

    auto* action_export_feeds = new QAction(qApp->icons()->fromTheme(QSL("document-export")), tr("Export feeds"), this);
    auto* action_import_feeds = new QAction(qApp->icons()->fromTheme(QSL("document-import")), tr("Import feeds"), this);

    connect(action_export_feeds, &QAction::triggered, this, &StandardServiceRoot::exportFeeds);
    connect(action_import_feeds, &QAction::triggered, this, &StandardServiceRoot::importFeeds);

    m_serviceMenu.append(action_export_feeds);
    m_serviceMenu.append(action_import_feeds);
  }

  return m_serviceMenu;
}
