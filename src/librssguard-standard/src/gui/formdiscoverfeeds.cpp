// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formdiscoverfeeds.h"

#include "src/definitions.h"
#include "src/gui/formstandardfeeddetails.h"
#include "src/parsers/atomparser.h"
#include "src/parsers/gemlogparser.h"
#include "src/parsers/icalparser.h"
#include "src/parsers/jsonparser.h"
#include "src/parsers/rdfparser.h"
#include "src/parsers/rssparser.h"
#include "src/parsers/sitemapparser.h"
#include "src/standardfeed.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/gui/guiutilities.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/iofactory.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/network-web/webfactory.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/serviceroot.h>

#include <QFileInfo>
#include <QHash>
#include <QMetaObject>
#include <QtConcurrentMap>

FormDiscoverFeeds::FormDiscoverFeeds(ServiceRoot* service_root,
                                     RootItem* parent_to_select,
                                     const QString& url,
                                     QWidget* parent)
  : QDialog(parent), m_serviceRoot(service_root), m_discoveredModel(new DiscoveredFeedsModel(this)),
    m_deepDiscovery(false) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("application-rss+xml")));

  m_parsers = {new AtomParser({}),
               new RssParser({}),
               new RdfParser({}),
               new IcalParser({}),
               new GemlogParser({}),
               new JsonParser({}),
               new SitemapParser({})};

  m_btnGoAdvanced = m_ui.m_buttonBox->addButton(tr("Switch to advanced &mode"), QDialogButtonBox::ButtonRole::NoRole);
  m_btnGoAdvanced
    ->setToolTip(tr("Close this dialog and display dialog for adding individual feeds with advanced options."));

  setTabOrder(m_ui.m_txtUrl->textEdit(), m_ui.m_btnDiscover);
  setTabOrder(m_ui.m_btnDiscover, m_ui.m_cbDiscoverRecursive);
  setTabOrder(m_ui.m_cbDiscoverRecursive, m_ui.m_cmbParentCategory);
  setTabOrder(m_ui.m_cmbParentCategory, m_ui.m_btnSelecAll);
  setTabOrder(m_ui.m_btnSelecAll, m_ui.m_btnSelectNone);
  setTabOrder(m_ui.m_btnSelectNone, m_ui.m_tvFeeds);
  setTabOrder(m_ui.m_tvFeeds, m_ui.m_btnAddIndividually);
  setTabOrder(m_ui.m_btnAddIndividually, m_ui.m_btnImportSelected);
  setTabOrder(m_ui.m_btnImportSelected, m_btnGoAdvanced);
  setTabOrder(m_btnGoAdvanced, m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Cancel));

  m_ui.m_btnSelecAll->setIcon(qApp->icons()->fromTheme(QSL("dialog-yes"), QSL("edit-select-all")));
  m_ui.m_btnSelectNone->setIcon(qApp->icons()->fromTheme(QSL("dialog-no"), QSL("edit-select-none")));
  m_ui.m_btnAddIndividually->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_btnGoAdvanced->setIcon(qApp->icons()->fromTheme(QSL("system-upgrade")));
  m_ui.m_btnImportSelected->setIcon(qApp->icons()->fromTheme(QSL("document-import")));
  m_ui.m_buttonBox->button(QDialogButtonBox::StandardButton::Close)
    ->setIcon(qApp->icons()->fromTheme(QSL("window-close")));
  m_ui.m_btnDiscover->setIcon(qApp->icons()->fromTheme(QSL("system-search")));

  connect(m_ui.m_txtUrl->textEdit(), &QPlainTextEdit::textChanged, this, &FormDiscoverFeeds::onUrlChanged);
  connect(m_ui.m_btnImportSelected, &QPushButton::clicked, this, &FormDiscoverFeeds::importSelectedFeeds);
  connect(m_ui.m_btnSelecAll, &QPushButton::clicked, m_discoveredModel, &DiscoveredFeedsModel::checkAllItems);
  connect(m_ui.m_btnSelectNone, &QPushButton::clicked, m_discoveredModel, &DiscoveredFeedsModel::uncheckAllItems);
  connect(m_ui.m_btnAddIndividually, &QPushButton::clicked, this, &FormDiscoverFeeds::addSingleFeed);
  connect(m_btnGoAdvanced, &QPushButton::clicked, this, &FormDiscoverFeeds::userWantsAdvanced);
  connect(m_ui.m_btnDiscover, &QPushButton::clicked, this, &FormDiscoverFeeds::discoverFeeds);
  connect(&m_watcherDocuments,
          &QFutureWatcher<DiscoverDocumentsResult>::finished,
          this,
          &FormDiscoverFeeds::onDocumentsFinished);
  connect(&m_watcherDocuments,
          &QFutureWatcher<DiscoverDocumentsResult>::progressValueChanged,
          this,
          &FormDiscoverFeeds::onDiscoveryProgress);
  connect(&m_watcherLinkedDocuments,
          &QFutureWatcher<DiscoverLinkedDocumentResult>::finished,
          this,
          &FormDiscoverFeeds::onLinkedDocumentsFinished);
  connect(&m_watcherLinkedDocuments,
          &QFutureWatcher<DiscoverLinkedDocumentResult>::progressValueChanged,
          this,
          &FormDiscoverFeeds::onDiscoveryProgress);
  connect(&m_watcherLookup,
          &QFutureWatcher<QList<StandardFeed*>>::progressValueChanged,
          this,
          &FormDiscoverFeeds::onDiscoveryProgress);
  connect(&m_watcherLookup,
          &QFutureWatcher<QList<StandardFeed*>>::finished,
          this,
          &FormDiscoverFeeds::onDiscoveryFinished);

  loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  m_ui.m_txtUrl->textEdit()->setPlaceholderText(tr("Enter feed URLs, one URL per line"));
  m_ui.m_tvFeeds->setModel(m_discoveredModel);
  m_ui.m_tvFeeds->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  m_ui.m_tvFeeds->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);

  connect(m_ui.m_tvFeeds->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &FormDiscoverFeeds::onFeedSelectionChanged);

  m_ui.m_pbDiscovery->setVisible(false);

  if (QUrl(url).isValid()) {
    m_ui.m_txtUrl->textEdit()->setPlainText(url);
  }

  if (url.isEmpty()) {
    emit m_ui.m_txtUrl->textEdit()->textChanged();
  }

  m_ui.m_txtUrl->textEdit()->selectAll();
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
                                                                const QUrl& url,
                                                                bool deep_discovery,
                                                                const QList<DocumentWithUrl>& documents) {
  auto feeds = parser->discoverFeeds(m_serviceRoot, url, deep_discovery, documents);
  QPixmap icon;
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  if (!feeds.isEmpty() &&
      NetworkFactory::downloadIcon({{url.toString(), false}}, timeout, icon, {}, m_serviceRoot->networkProxy()) ==
        QNetworkReply::NetworkError::NoError) {
    for (Feed* feed : std::as_const(feeds)) {
      feed->setIcon(icon);
    }
  }

  return feeds;
}

void FormDiscoverFeeds::discoverFeeds() {
  QStringList urls =
    m_ui.m_txtUrl->textEdit()->toPlainText().split(QRegularExpression(QSL("[\r\n]")), SPLIT_BEHAVIOR::SkipEmptyParts);

  for (QString& u : urls) {
    u = u.trimmed();
  }

  urls.removeAll({});
  urls.removeDuplicates();

  QList<DiscoverDocumentsTask> tasks;

  for (const QString& url : std::as_const(urls)) {
    const QUrl input_url = QUrl::fromUserInput(url);

    if (input_url.isValid()) {
      tasks.append({input_url, m_ui.m_cbDiscoverRecursive->isChecked()});
    }
  }

  if (tasks.isEmpty()) {
    return;
  }

  m_deepDiscovery = m_ui.m_cbDiscoverRecursive->isChecked();

  std::function<DiscoverDocumentsResult(const DiscoverDocumentsTask&)> func =
    [=](const DiscoverDocumentsTask& task) -> DiscoverDocumentsResult {
    return fetchDocumentsForUrl(task);
  };

#if QT_VERSION_MAJOR == 5
  QFuture<DiscoverDocumentsResult> fut = QtConcurrent::mapped(tasks, func);
#else
  QFuture<DiscoverDocumentsResult> fut = QtConcurrent::mapped(qApp->workHorsePool(), tasks, func);
#endif

  m_ui.m_pbDiscovery->setMaximum(tasks.size());
  m_ui.m_pbDiscovery->setValue(0);
  m_ui.m_pbDiscovery->setVisible(true);

  m_watcherDocuments.setFuture(fut);

  setEnabled(false);
}

FormDiscoverFeeds::DiscoverDocumentsResult FormDiscoverFeeds::fetchDocumentsForUrl(const DiscoverDocumentsTask& task) {
  QList<DocumentWithUrl> documents;
  QList<QUrl> linked_urls;
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  const QList<QPair<QByteArray, QByteArray>> headers = {{HTTP_HEADERS_ACCEPT, QByteArray("*/*")}};

  auto extractLinkedUrls = [&](const QList<DocumentWithUrl>& prepared_documents) {
    QStringList known_document_urls;

    for (const DocumentWithUrl& document : std::as_const(prepared_documents)) {
      QUrl document_url = document.m_documentUrl;

      document_url.setFragment({});
      known_document_urls << document_url.toString();
    }

    QStringList queued_document_urls = known_document_urls;

    for (const DocumentWithUrl& document : prepared_documents) {
      QStringList hyperlinks = qApp->web()->extractAllHyperlinks(document.m_documentUrl, document.m_documentData);

      hyperlinks.removeDuplicates();

      for (const QString& hyperlink : std::as_const(hyperlinks)) {
        QUrl hyperlink_url(hyperlink);

        hyperlink_url.setFragment({});

        const QString hyperlink_string = hyperlink_url.toString();

        if (!hyperlink_url.isValid() || hyperlink_string.isEmpty() || queued_document_urls.contains(hyperlink_string)) {
          continue;
        }

        if (hyperlink_url.isLocalFile() || hyperlink_url.scheme() == QSL("http") ||
            hyperlink_url.scheme() == QSL("https")) {
          linked_urls.append(hyperlink_url);
          queued_document_urls << hyperlink_string;
        }
      }
    }
  };

  const QUrl input_url = task.m_url;

  if (!input_url.isValid()) {
    return {task.m_url, documents};
  }

  if (!input_url.isLocalFile()) {
    QByteArray data;
    NetworkResult res = NetworkFactory::performNetworkOperation(input_url.toString(),
                                                                timeout,
                                                                {},
                                                                data,
                                                                QNetworkAccessManager::Operation::GetOperation,
                                                                headers,
                                                                {},
                                                                {},
                                                                {},
                                                                m_serviceRoot->networkProxy());

    if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
      documents.append({data, res.m_url.isValid() ? res.m_url : input_url});
    }
    else {
      qDebugNN << LOGSEC_STANDARD << "Base document download failed for" << QUOTE_W_SPACE(task.m_url.toString())
               << "with error:" << QUOTE_W_SPACE_DOT(NetworkFactory::networkErrorText(res.m_networkError));
    }
  }
  else {
    const QString file_path = input_url.toLocalFile();

    if (QFileInfo(file_path).isReadable()) {
      try {
        documents.append({IOFactory::readFile(file_path), input_url});
      }
      catch (const ApplicationException& ex) {
        qDebugNN << LOGSEC_STANDARD << "Base local document cannot be read:" << QUOTE_W_SPACE(file_path)
                 << NONQUOTE_W_SPACE_DOT(ex.message());
      }
    }
    else {
      qDebugNN << LOGSEC_STANDARD << "Base local document is not readable:" << QUOTE_W_SPACE_DOT(file_path);
    }
  }

  if (task.m_deepDiscovery) {
    extractLinkedUrls(documents);
  }

  return {task.m_url, documents, linked_urls};
}

FormDiscoverFeeds::DiscoverLinkedDocumentResult FormDiscoverFeeds::fetchLinkedDocument(const DiscoverLinkedDocumentTask&
                                                                                         task) {
  const QUrl input_url = task.m_url;

  if (!input_url.isValid()) {
    return {task.m_masterUrl, {}, false};
  }

  if (input_url.isLocalFile()) {
    const QString file_path = input_url.toLocalFile();

    if (QFileInfo(file_path).isReadable()) {
      try {
        return {task.m_masterUrl, {IOFactory::readFile(file_path), input_url}, true};
      }
      catch (const ApplicationException& ex) {
        qDebugNN << LOGSEC_STANDARD << "Linked local document cannot be read:" << QUOTE_W_SPACE(file_path)
                 << NONQUOTE_W_SPACE_DOT(ex.message());
      }
    }
    else {
      qDebugNN << LOGSEC_STANDARD << "Linked local document is not readable:" << QUOTE_W_SPACE_DOT(file_path);
    }

    return {task.m_masterUrl, {}, false};
  }

  if (input_url.scheme() != QSL("http") && input_url.scheme() != QSL("https")) {
    return {task.m_masterUrl, {}, false};
  }

  QByteArray data;
  const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  const QList<QPair<QByteArray, QByteArray>> headers = {{HTTP_HEADERS_ACCEPT, QByteArray("*/*")}};
  const QString input_url_string = input_url.toString();
  NetworkResult res = NetworkFactory::performNetworkOperation(input_url_string,
                                                              timeout,
                                                              {},
                                                              data,
                                                              QNetworkAccessManager::Operation::GetOperation,
                                                              headers,
                                                              {},
                                                              {},
                                                              {},
                                                              m_serviceRoot->networkProxy());

  if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
    QUrl document_url = res.m_url.isValid() ? res.m_url : input_url;

    document_url.setFragment({});

    return {task.m_masterUrl, {data, document_url}, true};
  }

  qDebugNN << LOGSEC_STANDARD << "Linked document download failed for" << QUOTE_W_SPACE(input_url_string)
           << "with error:" << QUOTE_W_SPACE_DOT(NetworkFactory::networkErrorText(res.m_networkError));

  return {task.m_masterUrl, {}, false};
}

void FormDiscoverFeeds::onDocumentsFinished() {
  QList<DiscoverLinkedDocumentTask> linked_document_tasks;

  m_documentsByUrl.clear();

  try {
    const auto future = m_watcherDocuments.future();

    for (int i = 0; i < future.resultCount(); ++i) {
      const DiscoverDocumentsResult result = future.resultAt(i);

      m_documentsByUrl.insert(result.m_url, result.m_documents);

      for (const QUrl& linked_url : result.m_linkedUrls) {
        linked_document_tasks.append({result.m_url, linked_url});
      }
    }
  }
  catch (const ApplicationException& ex) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot discover feeds"),
                          tr("Error: %1").arg(ex.message()),
                          QSystemTrayIcon::MessageIcon::Critical});

    setEnabled(true);
    return;
  }

  if (linked_document_tasks.isEmpty()) {
    startDiscoveringFeeds(m_documentsByUrl);
  }
  else {
    startDiscoveringLinkedDocuments(linked_document_tasks);
  }
}

void FormDiscoverFeeds::startDiscoveringLinkedDocuments(const QList<DiscoverLinkedDocumentTask>& tasks) {
  std::function<DiscoverLinkedDocumentResult(const DiscoverLinkedDocumentTask&)> func =
    [=](const DiscoverLinkedDocumentTask& task) -> DiscoverLinkedDocumentResult {
    return fetchLinkedDocument(task);
  };

#if QT_VERSION_MAJOR == 5
  QFuture<DiscoverLinkedDocumentResult> fut = QtConcurrent::mapped(tasks, func);
#else
  QFuture<DiscoverLinkedDocumentResult> fut = QtConcurrent::mapped(qApp->workHorsePool(), tasks, func);
#endif

  m_ui.m_pbDiscovery->setMaximum(tasks.size());
  m_ui.m_pbDiscovery->setValue(0);
  m_ui.m_pbDiscovery->setVisible(true);

  m_watcherLinkedDocuments.setFuture(fut);
}

void FormDiscoverFeeds::onLinkedDocumentsFinished() {
  try {
    const auto future = m_watcherLinkedDocuments.future();
    QHash<QUrl, QStringList> known_document_urls;

    for (auto it = m_documentsByUrl.cbegin(); it != m_documentsByUrl.cend(); ++it) {
      QStringList urls;

      for (const DocumentWithUrl& document : it.value()) {
        QUrl document_url = document.m_documentUrl;

        document_url.setFragment({});
        urls << document_url.toString();
      }

      known_document_urls.insert(it.key(), urls);
    }

    for (int i = 0; i < future.resultCount(); ++i) {
      const DiscoverLinkedDocumentResult result = future.resultAt(i);

      if (!result.m_success) {
        continue;
      }

      QUrl document_url = result.m_document.m_documentUrl;

      document_url.setFragment({});

      QStringList& known_urls = known_document_urls[result.m_masterUrl];
      const QString document_url_string = document_url.toString();

      if (!known_urls.contains(document_url_string)) {
        m_documentsByUrl[result.m_masterUrl].append(result.m_document);
        known_urls << document_url_string;
      }
    }
  }
  catch (const ApplicationException& ex) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot discover feeds"),
                          tr("Error: %1").arg(ex.message()),
                          QSystemTrayIcon::MessageIcon::Critical});

    setEnabled(true);
    return;
  }

  startDiscoveringFeeds(m_documentsByUrl);
}

void FormDiscoverFeeds::startDiscoveringFeeds(const QHash<QUrl, QList<DocumentWithUrl>>& documents_by_url) {
  QList<DiscoverTask> tasks;

  for (const FeedParser* parser : std::as_const(m_parsers)) {
    for (auto it = documents_by_url.cbegin(); it != documents_by_url.cend(); ++it) {
      const QUrl& url = it.key();

      if (url.isValid()) {
        tasks.append({parser, url, it.value()});
      }
    }
  }

  if (tasks.isEmpty()) {
    m_ui.m_pbDiscovery->setVisible(false);
    setEnabled(true);
    return;
  }

  std::function<QList<StandardFeed*>(const DiscoverTask&)> func =
    [=](const DiscoverTask& task) -> QList<StandardFeed*> {
    return discoverFeedsWithParser(task.m_parser, task.m_url, m_deepDiscovery, task.m_documents);
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
  QFuture<QList<StandardFeed*>> fut = QtConcurrent::mappedReduced<QList<StandardFeed*>>(tasks, func, reducer);
#else
  QFuture<QList<StandardFeed*>> fut =
    QtConcurrent::mappedReduced<QList<StandardFeed*>>(qApp->workHorsePool(), tasks, func, reducer);
#endif

  m_ui.m_pbDiscovery->setMaximum(tasks.size());
  m_ui.m_pbDiscovery->setValue(0);
  m_ui.m_pbDiscovery->setVisible(true);

  m_watcherLookup.setFuture(fut);
}

void FormDiscoverFeeds::onUrlChanged() {
  QStringList urls =
    m_ui.m_txtUrl->textEdit()->toPlainText().split(QRegularExpression(QSL("[\r\n]")), SPLIT_BEHAVIOR::SkipEmptyParts);

  urls.removeDuplicates();

  bool all_valid = true;

  for (const QString& url : std::as_const(urls)) {
    if (!QUrl(url.trimmed()).isValid()) {
      all_valid = false;
      break;
    }
  }

  if (!urls.isEmpty() && all_valid) {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Ok, tr("All URLs are valid."));
  }
  else {
    m_ui.m_txtUrl->setStatus(WidgetWithStatus::StatusType::Error, tr("One or more URLs are invalid."));
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

    try {
      qApp->database()->worker()->write([&](const QSqlDatabase& db) {
        DatabaseQueries::createOverwriteFeed(db, std_feed, m_serviceRoot->accountId(), parent->id());
      });

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
    if (m_watcherDocuments.isRunning()) {
      m_watcherDocuments.waitForFinished();
    }

    if (m_watcherLinkedDocuments.isRunning()) {
      m_watcherLinkedDocuments.waitForFinished();
    }

    if (m_watcherLookup.isRunning()) {
      m_watcherLookup.waitForFinished();
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
