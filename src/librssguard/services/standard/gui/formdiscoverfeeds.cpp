// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/formdiscoverfeeds.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/definitions.h"
#include "services/standard/standardfeed.h"

#include "services/standard/parsers/atomparser.h"
#include "services/standard/parsers/jsonparser.h"
#include "services/standard/parsers/rdfparser.h"
#include "services/standard/parsers/rssparser.h"
#include "services/standard/parsers/sitemapparser.h"

#include <QtConcurrentMap>

FormDiscoverFeeds::FormDiscoverFeeds(ServiceRoot* service_root,
                                     RootItem* parent_to_select,
                                     const QString& url,
                                     QWidget* parent)
  : QDialog(parent), m_serviceRoot(service_root), m_discoveredModel(new DiscoveredFeedsModel(this)) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("application-rss+xml")));

  m_parsers = {new AtomParser({}), new RssParser({}), new RdfParser({}), new JsonParser({}), new SitemapParser({})};

  m_btnGoAdvanced = m_ui.m_buttonBox->addButton(tr("Close && &advanced mode"), QDialogButtonBox::ButtonRole::NoRole);
  m_btnImportSelectedFeeds =
    m_ui.m_buttonBox->addButton(tr("Import selected feeds"), QDialogButtonBox::ButtonRole::ActionRole);

  m_btnGoAdvanced
    ->setToolTip(tr("Close this dialog and display dialog for adding individual feeds with advanced options."));

  m_btnGoAdvanced->setIcon(qApp->icons()->fromTheme(QSL("system-upgrade")));
  m_btnImportSelectedFeeds->setIcon(qApp->icons()->fromTheme(QSL("document-import")));
  m_ui.m_btnDiscover->setIcon(qApp->icons()->fromTheme(QSL("system-search")));

  connect(m_ui.m_txtUrl->lineEdit(), &QLineEdit::textChanged, this, &FormDiscoverFeeds::onUrlChanged);
  connect(m_btnImportSelectedFeeds, &QPushButton::clicked, this, &FormDiscoverFeeds::importSelectedFeeds);
  connect(m_btnGoAdvanced, &QPushButton::clicked, this, &FormDiscoverFeeds::userWantsAdvanced);
  connect(m_ui.m_btnDiscover, &QPushButton::clicked, this, &FormDiscoverFeeds::discoverFeeds);

  connect(&m_watcherLookup, &QFutureWatcher<QList<StandardFeed*>>::progressValueChanged, this, [=](int prog) {
    m_ui.m_pbDiscovery->setValue(prog);
    qDebugNN << "progress";
  });

  connect(&m_watcherLookup, &QFutureWatcher<QList<StandardFeed*>>::finished, this, [=]() {
    auto res = m_watcherLookup.future().result();

    loadDiscoveredFeeds(res);
  });

  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  m_ui.m_tvFeeds->setModel(m_discoveredModel);

  m_ui.m_tvFeeds->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  m_ui.m_tvFeeds->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);

  m_ui.m_pbDiscovery->setVisible(false);
  m_ui.m_txtUrl->lineEdit()->setText(url);

  if (url.isEmpty()) {
    emit m_ui.m_txtUrl->lineEdit()->textChanged(url);
  }

  m_ui.m_txtUrl->lineEdit()->selectAll();
  m_ui.m_txtUrl->setFocus();

  if (parent_to_select != nullptr) {
    if (parent_to_select->kind() == RootItem::Kind::Category) {
      m_ui.m_cmbParentCategory
        ->setCurrentIndex(m_ui.m_cmbParentCategory->findData(QVariant::fromValue((void*)parent_to_select)));
    }
    else if (parent_to_select->kind() == RootItem::Kind::Feed) {
      int target_item = m_ui.m_cmbParentCategory->findData(QVariant::fromValue((void*)parent_to_select->parent()));

      if (target_item >= 0) {
        m_ui.m_cmbParentCategory->setCurrentIndex(target_item);
      }
    }
    else {
      m_ui.m_cmbParentCategory->setCurrentIndex(0);
    }
  }
}

FormDiscoverFeeds::~FormDiscoverFeeds() {
  qDeleteAll(m_parsers);
}

void FormDiscoverFeeds::discoverFeeds() {
  QString url = m_ui.m_txtUrl->lineEdit()->text();

  std::function<QList<StandardFeed*>(const FeedParser*)> func = [=](const FeedParser* parser) -> QList<StandardFeed*> {
    return parser->discoverFeeds(m_serviceRoot, url);
  };

  std::function<QList<StandardFeed*>(QList<StandardFeed*>&, const QList<StandardFeed*>&)> reducer =
    [=](QList<StandardFeed*>& res, const QList<StandardFeed*>& interm) -> QList<StandardFeed*> {
    res.append(interm);
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
  m_ui.m_cmbParentCategory->addItem(root_item->fullIcon(), root_item->title(), QVariant::fromValue((void*)root_item));

  for (Category* category : categories) {
    m_ui.m_cmbParentCategory->addItem(category->fullIcon(), category->title(), QVariant::fromValue((void*)category));
  }
}

void FormDiscoverFeeds::addSingleFeed(StandardFeed* feed) {
  /*
  QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(this,
                                                                                   selected_item,
                                                                                   feed->source(),
                                                                                   qApp->mainFormWidget()));

  form_pointer->addEditFeed<StandardFeed>();
  */
}

void FormDiscoverFeeds::importSelectedFeeds() {}

void FormDiscoverFeeds::userWantsAdvanced() {
  setResult(ADVANCED_FEED_ADD_DIALOG_CODE);
  close();
}

void FormDiscoverFeeds::loadDiscoveredFeeds(const QList<StandardFeed*>& feeds) {
  m_ui.m_pbDiscovery->setVisible(false);
  m_discoveredModel->setDiscoveredFeeds(feeds);

  qDebugNN << "finish";
}

DiscoveredFeedsModel::DiscoveredFeedsModel(QObject* parent) : QAbstractListModel(parent) {}

int DiscoveredFeedsModel::rowCount(const QModelIndex& parent) const {
  return m_discoveredFeeds.size();
}

int DiscoveredFeedsModel::columnCount(const QModelIndex& parent) const {
  return 2;
}

QVariant DiscoveredFeedsModel::data(const QModelIndex& index, int role) const {
  switch (role) {
    case Qt::ItemDataRole::DisplayRole: {
      if (index.column() == 0) {
        return m_discoveredFeeds.at(index.row()).m_feed->title();
      }
      else {
        return StandardFeed::typeToString(m_discoveredFeeds.at(index.row()).m_feed->type());
      }
    }

    case Qt::ItemDataRole::CheckStateRole: {
      if (index.column() == 0) {
        return m_discoveredFeeds.at(index.row()).m_isChecked ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
      }
      else {
        return {};
      }

      break;
    }

    case Qt::ItemDataRole::DecorationRole: {
      if (index.column() == 0) {
        return m_discoveredFeeds.at(index.row()).m_feed->fullIcon();
      }
    }

    default:
      return {};
  }
}

QList<DiscoveredFeedsModel::FeedItem> DiscoveredFeedsModel::discoveredFeeds() const {
  return m_discoveredFeeds;
}

void DiscoveredFeedsModel::setDiscoveredFeeds(const QList<StandardFeed*>& feeds) {
  auto std_feeds = boolinq::from(feeds)
                     .select([](StandardFeed* fd) {
                       return FeedItem{false, fd};
                     })
                     .toStdList();

  m_discoveredFeeds = FROM_STD_LIST(QList<FeedItem>, std_feeds);

  emit layoutAboutToBeChanged();
  emit layoutChanged();
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

Qt::ItemFlags DiscoveredFeedsModel::flags(const QModelIndex& index) const {
  return index.column() == 0 ? Qt::ItemFlag::ItemIsUserCheckable | QAbstractListModel::flags(index)
                             : QAbstractListModel::flags(index);
}

bool DiscoveredFeedsModel::setData(const QModelIndex& index, const QVariant& value, int role) {
  if (role == Qt::ItemDataRole::CheckStateRole && index.column() == 0) {
    m_discoveredFeeds[index.row()].m_isChecked = value.value<Qt::CheckState>() == Qt::CheckState::Checked;
    return true;
  }

  return QAbstractListModel::setData(index, value, role);
}
