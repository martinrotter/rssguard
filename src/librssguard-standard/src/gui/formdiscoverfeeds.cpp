// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formdiscoverfeeds.h"

#include "src/definitions.h"
#include "src/gui/formstandardfeeddetails.h"
#include "src/parsers/atomparser.h"
#include "src/parsers/icalparser.h"
#include "src/parsers/jsonparser.h"
#include "src/parsers/rdfparser.h"
#include "src/parsers/rssparser.h"
#include "src/parsers/sitemapparser.h"
#include "src/standardfeed.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/gui/guiutilities.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/serviceroot.h>

#include <QtConcurrentMap>

FormDiscoverFeeds::FormDiscoverFeeds(ServiceRoot* service_root,
                                     RootItem* parent_to_select,
                                     const QString& url,
                                     QWidget* parent)
  : QDialog(parent), m_serviceRoot(service_root), m_discoveredModel(new DiscoveredFeedsModel(this)) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("application-rss+xml")));

  m_parsers = {new AtomParser({}),
               new RssParser({}),
               new RdfParser({}),
               new IcalParser({}),
               new JsonParser({}),
               new SitemapParser({})};

  m_btnGoAdvanced = m_ui.m_buttonBox->addButton(tr("Switch to advanced &mode"), QDialogButtonBox::ButtonRole::NoRole);
  m_btnGoAdvanced
    ->setToolTip(tr("Close this dialog and display dialog for adding individual feeds with advanced options."));

  setTabOrder({m_ui.m_txtUrl->lineEdit(),
               m_ui.m_btnDiscover,
               m_ui.m_cbDiscoverRecursive,
               m_ui.m_cmbParentCategory,
               m_ui.m_btnSelecAll,
               m_ui.m_btnSelectNone,
               m_ui.m_tvFeeds,
               m_ui.m_btnAddIndividually,
               m_ui.m_btnImportSelected,
               m_btnGoAdvanced,
               m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Cancel)});

  m_ui.m_btnSelecAll->setIcon(qApp->icons()->fromTheme(QSL("dialog-yes"), QSL("edit-select-all")));
  m_ui.m_btnSelectNone->setIcon(qApp->icons()->fromTheme(QSL("dialog-no"), QSL("edit-select-none")));
  m_ui.m_btnAddIndividually->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_btnGoAdvanced->setIcon(qApp->icons()->fromTheme(QSL("system-upgrade")));
  m_ui.m_btnImportSelected->setIcon(qApp->icons()->fromTheme(QSL("document-import")));
  m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Close)
    ->setIcon(qApp->icons()->fromTheme(QSL("window-close")));
  m_ui.m_btnDiscover->setIcon(qApp->icons()->fromTheme(QSL("system-search")));

  connect(m_ui.m_txtUrl->lineEdit(), &QLineEdit::textChanged, this, &FormDiscoverFeeds::onUrlChanged);
  connect(m_ui.m_btnImportSelected, &QPushButton::clicked, this, &FormDiscoverFeeds::importSelectedFeeds);
  connect(m_ui.m_btnSelecAll, &QPushButton::clicked, m_discoveredModel, &DiscoveredFeedsModel::checkAllItems);
  connect(m_ui.m_btnSelectNone, &QPushButton::clicked, m_discoveredModel, &DiscoveredFeedsModel::uncheckAllItems);
  connect(m_ui.m_btnAddIndividually, &QPushButton::clicked, this, &FormDiscoverFeeds::addSingleFeed);
  connect(m_btnGoAdvanced, &QPushButton::clicked, this, &FormDiscoverFeeds::userWantsAdvanced);
  connect(m_ui.m_btnDiscover, &QPushButton::clicked, this, &FormDiscoverFeeds::discoverFeeds);
  connect(&m_watcherLookup,
          &QFutureWatcher<QList<StandardFeed*>>::progressValueChanged,
          this,
          &FormDiscoverFeeds::onDiscoveryProgress);
  connect(&m_watcherLookup,
          &QFutureWatcher<QList<StandardFeed*>>::finished,
          this,
          &FormDiscoverFeeds::onDiscoveryFinished);

  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  m_ui.m_tvFeeds->setModel(m_discoveredModel);

  m_ui.m_tvFeeds->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  m_ui.m_tvFeeds->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);

  connect(m_ui.m_tvFeeds->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &FormDiscoverFeeds::onFeedSelectionChanged);

  m_ui.m_pbDiscovery->setVisible(false);

  if (QUrl(url).isValid()) {
    m_ui.m_txtUrl->lineEdit()->setText(url);
  }

  if (url.isEmpty()) {
    emit m_ui.m_txtUrl->lineEdit()->textChanged(url);
  }

  m_ui.m_txtUrl->lineEdit()->selectAll();
  m_ui.m_txtUrl->setFocus();

  if (parent_to_select != nullptr) {
    if (parent_to_select->kind() == RootItem::Kind::Category) {
      m_ui.m_cmbParentCategory
        ->setCurrentIndex(m_ui.m_cmbParentCategory->findData(QVariant::fromValue(parent_to_select)));
    }
    else if (parent_to_select->kind() == RootItem::Kind::Feed) {
      int target_item = m_ui.m_cmbParentCategory->findData(QVariant::fromValue(parent_to_select->parent()));

      if (target_item >= 0) {
        m_ui.m_cmbParentCategory->setCurrentIndex(target_item);
      }
    }
    else {
      m_ui.m_cmbParentCategory->setCurrentIndex(0);
    }
  }
}

void FormDiscoverFeeds::onDiscoveryProgress(int progress) {
  m_ui.m_pbDiscovery->setValue(progress);
}

void FormDiscoverFeeds::onDiscoveryFinished() {
  try {
    auto res = m_watcherLookup.future().result();

    loadDiscoveredFeeds(res);
  }
  catch (const ApplicationException& ex) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot discover feeds"),
                          tr("Error: %1").arg(ex.message()),
                          QSystemTrayIcon::MessageIcon::Critical});
  }

  setEnabled(true);
}

StandardFeed* FormDiscoverFeeds::selectedFeed() const {
  RootItem* it = m_discoveredModel->itemForIndex(m_ui.m_tvFeeds->currentIndex());

  return qobject_cast<StandardFeed*>(it);
}

RootItem* FormDiscoverFeeds::targetParent() const {
  return m_ui.m_cmbParentCategory->currentData().value<RootItem*>();
}

FormDiscoverFeeds::~FormDiscoverFeeds() {
  qDeleteAll(m_parsers);

  m_discoveredModel->setRootItem(nullptr);
}

QList<StandardFeed*> FormDiscoverFeeds::discoverFeedsWithParser(const FeedParser* parser,
                                                                const QString& url,
                                                                bool greedy) {
  auto feeds = parser->discoverFeeds(m_serviceRoot, QUrl::fromUserInput(url), greedy);
  QPixmap icon;
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  if (NetworkFactory::downloadIcon({{url, false}}, timeout, icon, {}, m_serviceRoot->networkProxy()) ==
      QNetworkReply::NetworkError::NoError) {
    for (Feed* feed : feeds) {
      feed->setIcon(icon);
    }
  }

  return feeds;
}

void FormDiscoverFeeds::discoverFeeds() {
  QString url = m_ui.m_txtUrl->lineEdit()->text();
  bool greedy_discover = m_ui.m_cbDiscoverRecursive->isChecked();

  std::function<QList<StandardFeed*>(const FeedParser*)> func = [=](const FeedParser* parser) -> QList<StandardFeed*> {
    return discoverFeedsWithParser(parser, url, greedy_discover);
  };

  std::function<QList<StandardFeed*>(QList<StandardFeed*>&, const QList<StandardFeed*>&)> reducer =
    [=](QList<StandardFeed*>& res, const QList<StandardFeed*>& interm) -> QList<StandardFeed*> {
    for (StandardFeed* new_fd : interm) {
      if (!std::any_of(res.cbegin(), res.cend(), [=](const StandardFeed* fd) {
            return fd->source() == new_fd->source();
          })) {
        res.append(new_fd);
      }
    }

    return res;
  };

#if QT_VERSION_MAJOR == 5
  QFuture<QList<StandardFeed*>> fut = QtConcurrent::mappedReduced<QList<StandardFeed*>>(m_parsers, func, reducer);
#else
  QFuture<QList<StandardFeed*>> fut =
    QtConcurrent::mappedReduced<QList<StandardFeed*>>(qApp->workHorsePool(), m_parsers, func, reducer);
#endif

  m_watcherLookup.setFuture(fut);

  m_ui.m_pbDiscovery->setMaximum(m_parsers.size());
  m_ui.m_pbDiscovery->setValue(0);
  m_ui.m_pbDiscovery->setVisible(true);
  setEnabled(false);
}

void FormDiscoverFeeds::onUrlChanged(const QString& new_url) {
  if (QUrl(new_url).isValid()) {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Ok, tr("URL is valid."));
  }
  else {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Error, tr("URL is NOT valid."));
  }
}

void FormDiscoverFeeds::loadCategories(const QList<Category*>& categories, RootItem* root_item) {
  m_ui.m_cmbParentCategory->addItem(root_item->fullIcon(), root_item->title(), QVariant::fromValue(root_item));

  for (Category* category : categories) {
    m_ui.m_cmbParentCategory->addItem(category->fullIcon(), category->title(), QVariant::fromValue(category));
  }
}

void FormDiscoverFeeds::addSingleFeed() {
  auto* fd = selectedFeed();

  if (fd == nullptr) {
    return;
  }

  auto idx = m_ui.m_tvFeeds->currentIndex();

  QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(m_serviceRoot,
                                                                                   targetParent(),
                                                                                   fd->source(),
                                                                                   qApp->mainFormWidget()));

  if (!form_pointer->addEditFeed<StandardFeed>().isEmpty()) {
    // Feed was added, remove from list.
    if (m_discoveredModel->removeItem(idx) != nullptr) {
      // Feed was guessed by the dialog, we do not need this object.
      fd->deleteLater();
    }
  }
}

void FormDiscoverFeeds::importSelectedFeeds() {
  for (RootItem* it : m_discoveredModel->checkedItems()) {
    Feed* std_feed = it->toFeed();
    RootItem* parent = targetParent();
    QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

    try {
      DatabaseQueries::createOverwriteFeed(database, std_feed, m_serviceRoot->accountId(), parent->id());

      m_discoveredModel->removeItem(std_feed);
      m_serviceRoot->requestItemReassignment(std_feed, parent);
      m_serviceRoot->itemChanged({std_feed});
    }
    catch (const ApplicationException& ex) {
      qFatal("Cannot save feed: '%s'.", qPrintable(ex.message()));
    }
  }
}

void FormDiscoverFeeds::onFeedSelectionChanged() {
  m_ui.m_btnAddIndividually->setEnabled(selectedFeed() != nullptr);
}

void FormDiscoverFeeds::userWantsAdvanced() {
  setResult(ADVANCED_FEED_ADD_DIALOG_CODE);
  close();
}

void FormDiscoverFeeds::loadDiscoveredFeeds(const QList<StandardFeed*>& feeds) {
  RootItem* root = new RootItem();

  for (Feed* feed : feeds) {
    if (feed->title().isEmpty()) {
      feed->setTitle(tr("No title"));
    }

    root->appendChild(feed);
  }

  m_ui.m_pbDiscovery->setVisible(false);
  m_discoveredModel->setRootItem(root);
}

DiscoveredFeedsModel::DiscoveredFeedsModel(QObject* parent) : AccountCheckModel(parent) {}

int DiscoveredFeedsModel::columnCount(const QModelIndex& parent) const {
  return 2;
}

QVariant DiscoveredFeedsModel::data(const QModelIndex& index, int role) const {
  if (role == Qt::ItemDataRole::DisplayRole && index.column() == 1) {
    StandardFeed* fd = qobject_cast<StandardFeed*>(itemForIndex(index));

    if (fd != nullptr) {
      return StandardFeed::typeToString(fd->type());
    }
  }

  return AccountCheckModel::data(index, role);
}

QVariant DiscoveredFeedsModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Orientation::Vertical) {
    return {};
  }

  static QStringList headers = {tr("Title"), tr("Type")};

  if (role == Qt::ItemDataRole::DisplayRole) {
    return headers.at(section);
  }

  return {};
}

void FormDiscoverFeeds::closeEvent(QCloseEvent* event) {
  try {
    // Wait for discovery to finish.
    if (m_watcherLookup.isRunning()) {
      m_watcherLookup.result();
    }
  }
  catch (...) {
  }

  // Clear all remaining items.
  m_discoveredModel->setRootItem(nullptr);

  QDialog::closeEvent(event);
}

RootItem* DiscoveredFeedsModel::removeItem(RootItem* it) {
  auto idx = indexForItem(it);

  if (!idx.isValid() || it == nullptr || it == m_rootItem || it->parent() == nullptr) {
    return nullptr;
  }

  beginRemoveRows(idx.parent(), idx.row(), idx.row());
  it->parent()->removeChild(it);
  removeCheckState(it);
  endRemoveRows();

  return it;
}

RootItem* DiscoveredFeedsModel::removeItem(const QModelIndex& idx) {
  RootItem* it = itemForIndex(idx);

  if (it == nullptr || it == m_rootItem || it->parent() == nullptr) {
    return nullptr;
  }

  beginRemoveRows(idx.parent(), idx.row(), idx.row());
  it->parent()->removeChild(it);
  endRemoveRows();

  return it;
}
