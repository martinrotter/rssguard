// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardserviceroot.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/standard/gui/formstandardcategorydetails.h"
#include "services/standard/gui/formstandardfeeddetails.h"
#include "services/standard/gui/formstandardimportexport.h"
#include "services/standard/standardcategory.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardfeedsimportexportmodel.h"
#include "services/standard/standardserviceentrypoint.h"

#include <QAction>
#include <QClipboard>
#include <QSqlTableModel>
#include <QStack>

StandardServiceRoot::StandardServiceRoot(RootItem* parent)
  : ServiceRoot(parent) {
  setTitle(qApp->system()->loggedInUser() + QSL(" (RSS/RDF/ATOM)"));
  setIcon(StandardServiceEntryPoint().icon());
  setDescription(tr("This is obligatory service account for standard RSS/RDF/ATOM feeds."));
}

StandardServiceRoot::~StandardServiceRoot() {
  qDeleteAll(m_feedContextMenu);
}

void StandardServiceRoot::start(bool freshly_activated) {
  loadFromDatabase();

  if (freshly_activated && getSubTree(RootItem::Kind::Feed).isEmpty()) {
    // In other words, if there are no feeds or categories added.
    if (MessageBox::show(qApp->mainFormWidget(), QMessageBox::Question, QObject::tr("Load initial set of feeds"),
                         tr("This new account does not include any feeds. You can now add default set of feeds."),
                         tr("Do you want to load initial set of feeds?"),
                         QString(), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
      QString target_opml_file = APP_INITIAL_FEEDS_PATH + QDir::separator() + FEED_INITIAL_OPML_PATTERN;
      QString current_locale = qApp->localization()->loadedLanguage();
      QString file_to_load;

      if (QFile::exists(target_opml_file.arg(current_locale))) {
        file_to_load = target_opml_file.arg(current_locale);
      }
      else if (QFile::exists(target_opml_file.arg(DEFAULT_LOCALE))) {
        file_to_load = target_opml_file.arg(DEFAULT_LOCALE);
      }

      FeedsImportExportModel model;
      QString output_msg;

      try {
        model.importAsOPML20(IOFactory::readFile(file_to_load), false);
        model.checkAllItems();

        if (mergeImportExportModel(&model, this, output_msg)) {
          requestItemExpand(getSubTree(), true);
        }
      }
      catch (ApplicationException& ex) {
        MessageBox::show(qApp->mainFormWidget(), QMessageBox::Critical, tr("Error when loading initial feeds"), ex.message());
      }
    }
  }

  checkArgumentsForFeedAdding();
}

void StandardServiceRoot::stop() {
  qDebugNN << LOGSEC_CORE << "Stopping StandardServiceRoot instance.";
}

QString StandardServiceRoot::code() const {
  return StandardServiceEntryPoint().code();
}

bool StandardServiceRoot::canBeEdited() const {
  return false;
}

bool StandardServiceRoot::canBeDeleted() const {
  return true;
}

bool StandardServiceRoot::deleteViaGui() {
  return ServiceRoot::deleteViaGui();
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
    qApp->showGuiMessage(tr("Cannot add item"),
                         tr("Cannot add feed because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainFormWidget(), true);

    // Thus, cannot delete and quit the method.
    return;
  }

  QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(this,
                                                                                   qApp->mainFormWidget()));

  form_pointer->addEditFeed(nullptr, selected_item, url);
  qApp->feedUpdateLock()->unlock();
}

Qt::ItemFlags StandardServiceRoot::additionalFlags() const {
  return Qt::ItemIsDropEnabled;
}

void StandardServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<StandardCategory>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<StandardFeed>(database, qApp->feedReader()->messageFilters(), accountId());
  auto labels = DatabaseQueries::getLabels(database, accountId());

  performInitialAssembly(categories, feeds, labels);
}

void StandardServiceRoot::checkArgumentsForFeedAdding() {
  for (const QString& arg : qApp->arguments().mid(1)) {
    checkArgumentForFeedAdding(arg);
  }
}

QString StandardServiceRoot::processFeedUrl(const QString& feed_url) {
  if (feed_url.startsWith(QL1S(URI_SCHEME_FEED_SHORT))) {
    QString without_feed_prefix = feed_url.mid(5);

    if (without_feed_prefix.startsWith(QL1S("https:")) || without_feed_prefix.startsWith(QL1S("http:"))) {
      return without_feed_prefix;
    }
    else {
      return feed_url;
    }
  }
  else {
    return feed_url;
  }
}

void StandardServiceRoot::checkArgumentForFeedAdding(const QString& argument) {
  if (argument.startsWith(QL1S(URI_SCHEME_FEED_SHORT))) {
    addNewFeed(nullptr, processFeedUrl(argument));
  }
}

QList<QAction*> StandardServiceRoot::getContextMenuForFeed(StandardFeed* feed) {
  if (m_feedContextMenu.isEmpty()) {
    // Initialize.
    auto* action_metadata = new QAction(qApp->icons()->fromTheme(QSL("emblem-downloads")),
                                        tr("Fetch metadata"),
                                        this);

    m_feedContextMenu.append(action_metadata);

    connect(action_metadata, &QAction::triggered, this, [this]() {
      m_feedForMetadata->fetchMetadataForItself();
    });
  }

  m_feedForMetadata = feed;

  return m_feedContextMenu;
}

bool StandardServiceRoot::mergeImportExportModel(FeedsImportExportModel* model, RootItem* target_root_node, QString& output_message) {
  QStack<RootItem*> original_parents;

  original_parents.push(target_root_node);
  QStack<RootItem*> new_parents;

  new_parents.push(model->sourceModel()->rootItem());
  bool some_feed_category_error = false;

  // Iterate all new items we would like to merge into current model.
  while (!new_parents.isEmpty()) {
    RootItem* target_parent = original_parents.pop();
    RootItem* source_parent = new_parents.pop();

    for (RootItem* source_item : source_parent->childItems()) {
      if (!model->sourceModel()->isItemChecked(source_item)) {
        // We can skip this item, because it is not checked and should not be imported.
        // NOTE: All descendants are thus skipped too.
        continue;
      }

      if (source_item->kind() == RootItem::Kind::Category) {
        auto* source_category = dynamic_cast<StandardCategory*>(source_item);
        auto* new_category = new StandardCategory(*source_category);
        QString new_category_title = new_category->title();

        // Add category to model.
        new_category->clearChildren();

        if (new_category->addItself(target_parent)) {
          requestItemReassignment(new_category, target_parent);

          // Process all children of this category.
          original_parents.push(new_category);
          new_parents.push(source_category);
        }
        else {
          delete new_category;

          // Add category failed, but this can mean that the same category (with same title)
          // already exists. If such a category exists in current parent, then find it and
          // add descendants to it.
          RootItem* existing_category = nullptr;

          for (RootItem* child : target_parent->childItems()) {
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
          }
        }
      }
      else if (source_item->kind() == RootItem::Kind::Feed) {
        auto* source_feed = dynamic_cast<StandardFeed*>(source_item);
        auto* new_feed = new StandardFeed(*source_feed);

        // Append this feed and end this iteration.
        if (new_feed->addItself(target_parent)) {
          requestItemReassignment(new_feed, target_parent);
        }
        else {
          delete new_feed;
          some_feed_category_error = true;
        }
      }
    }
  }

  if (some_feed_category_error) {
    output_message = tr("Import successful, but some feeds/categories were not imported due to error.");
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
    qApp->showGuiMessage(tr("Cannot add category"),
                         tr("Cannot add category because another critical operation is ongoing."),
                         QSystemTrayIcon::Warning, qApp->mainFormWidget(), true);

    // Thus, cannot delete and quit the method.
    return;
  }

  QScopedPointer<FormStandardCategoryDetails> form_pointer(new FormStandardCategoryDetails(this,
                                                                                           qApp->mainFormWidget()));

  form_pointer->addEditCategory(nullptr, selected_item);
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

    auto* action_export_feeds = new QAction(qApp->icons()->fromTheme("document-export"), tr("Export feeds"), this);
    auto* action_import_feeds = new QAction(qApp->icons()->fromTheme("document-import"), tr("Import feeds"), this);

    connect(action_export_feeds, &QAction::triggered, this, &StandardServiceRoot::exportFeeds);
    connect(action_import_feeds, &QAction::triggered, this, &StandardServiceRoot::importFeeds);

    m_serviceMenu.append(action_export_feeds);
    m_serviceMenu.append(action_import_feeds);
  }

  return m_serviceMenu;
}
