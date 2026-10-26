// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formstandardfeeddetails.h"

#include "src/gui/standardfeeddetails.h"
#include "src/gui/standardfeedexpdetails.h"
#include "src/gui/standardfeednetworkdetails.h"
#include "src/standardfeed.h"
#include "src/standardserviceroot.h"

#include <exception>
#include <librssguard/database/databasefactory.h>
#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/gui/dialogs/formprogressworker.h>
#include <librssguard/gui/reusable/networkproxydetails.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/network-web/webfactory.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/serviceroot.h>

#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QNetworkCookie>
#include <QStringList>

FormStandardFeedDetails::FormStandardFeedDetails(ServiceRoot* service_root,
                                                 RootItem* parent_to_select,
                                                 const QString& url,
                                                 QWidget* parent)
  : FormFeedDetails(service_root, parent), m_standardFeedDetails(new StandardFeedDetails(this)),
    m_standardFeedExpDetails(new StandardFeedExpDetails(this)), m_networkDetails(new StandardFeedNetworkDetails(this)),
    m_parentToSelect(parent_to_select), m_urlToProcess(url) {
  insertScrollableCustomTab(m_standardFeedDetails, tr("General"), 0);
  insertScrollableCustomTab(m_networkDetails, tr("Network"));
  insertScrollableCustomTab(m_standardFeedExpDetails, tr("Experimental"));
  activateTab(0);

  m_standardFeedDetails->setNetworkDetails(m_networkDetails);
  initializeIconFetching();

  connect(m_standardFeedDetails->m_ui.m_btnFetchMetadata,
          &QPushButton::clicked,
          this,
          &FormStandardFeedDetails::guessFeed);
  connect(m_standardFeedDetails->m_actionFetchIcon, &QAction::triggered, this, &FormStandardFeedDetails::guessIconOnly);
  connect(m_standardFeedDetails->m_ui.m_txtTitle->lineEdit(),
          &QLineEdit::textChanged,
          this,
          &FormStandardFeedDetails::onTitleChanged);

  onTitleChanged(m_standardFeedDetails->m_ui.m_txtTitle->lineEdit()->text());
}

void FormStandardFeedDetails::guessFeed() {
  m_standardFeedDetails->guessFeed(m_standardFeedDetails->sourceType(),
                                   m_standardFeedDetails->m_ui.m_txtSource->textEdit()->toPlainText(),
                                   m_standardFeedDetails->m_ui.m_txtPostProcessScript->textEdit()->toPlainText(),
                                   qobject_cast<StandardServiceRoot*>(m_serviceRoot),
                                   m_networkDetails->m_ui.m_wdgAuthentication->authenticationType(),
                                   m_networkDetails->m_ui.m_wdgAuthentication->username(),
                                   m_networkDetails->m_ui.m_wdgAuthentication->password(),
                                   StandardFeed::httpHeadersToList(m_networkDetails->httpHeaders()),
                                   m_networkDetails->m_ui.m_wdgNetworkProxy->useAccountProxy()
                                     ? m_serviceRoot->networkProxy()
                                     : m_networkDetails->m_ui.m_wdgNetworkProxy->proxy(),
                                   m_networkDetails->http2Status(),
                                   m_networkDetails->m_ui.m_cbIgnoreCookies->isChecked());
}

void FormStandardFeedDetails::guessIconOnly() {
  struct IconJob {
      FeedIconRequest m_request;
      QString m_title;
      QImage m_icon;
      QString m_error;
  };

  QList<QSharedPointer<IconJob>> jobs;
  const auto selected_feeds = feeds<StandardFeed>();
  for (StandardFeed* feed : selected_feeds) {
    auto job = QSharedPointer<IconJob>::create();
    job->m_request = iconRequest(feed);
    job->m_title = feed->title();
    jobs.append(job);
  }
  if (jobs.isEmpty()) {
    return;
  }

  FormProgressWorker progress(this);
  auto* account = qobject_cast<StandardServiceRoot*>(m_serviceRoot);
  const int result = progress.doWork<QSharedPointer<IconJob>>(
    tr("Fetching feed icons"),
    true,
    jobs,
    [account](QSharedPointer<IconJob> job) {
      try {
        job->m_icon = FeedParser::fetchFeedIconForSource(job->m_request, account);
        if (job->m_icon.isNull()) {
          job->m_error = tr("No usable icon found.");
        }
      }
      catch (const ApplicationException& ex) {
        job->m_error = ex.message();
      }
      catch (const std::exception& ex) {
        job->m_error = QString::fromUtf8(ex.what());
      }
      catch (...) {
        job->m_error = tr("Icon fetching failed.");
      }
    },
    [count = jobs.size()](int processed) {
      return tr("Processed %1 of %2 feeds.").arg(processed).arg(count);
    });

  if (result != QDialog::DialogCode::Accepted) {
    // Keep the previous selection, including any earlier successful fetch.
    return;
  }

  QHash<int, QIcon> icons;
  QStringList errors;
  for (int i = 0; i < jobs.size(); ++i) {
    const auto& job = jobs.at(i);
    if (!job->m_icon.isNull()) {
      icons.insert(i, QIcon(QPixmap::fromImage(job->m_icon)));
    }
    else {
      errors.append(tr("%1: %2").arg(job->m_title, job->m_error));
    }
  }

  if (m_isBatchEdit) {
    m_iconSelection.selectIndividual(icons);
    m_standardFeedDetails->m_ui.m_btnIcon->setIcon(qApp->icons()->fromTheme(QSL("view-list-icons")), false);
    m_standardFeedDetails->m_ui.m_btnIcon->setToolTip(tr("Individual icons are ready for %1 feeds.").arg(icons.size()));
  }
  else if (icons.contains(0)) {
    m_standardFeedDetails->m_ui.m_btnIcon->setIcon(icons.value(0));
  }

  const QString summary = tr("Fetched %1 of %2 icons. Existing icons will be kept for unsuccessful feeds.")
                            .arg(icons.size())
                            .arg(jobs.size());
  m_standardFeedDetails->m_ui.m_lblFetchMetadata->setText(errors.isEmpty() ? WidgetWithStatus::StatusType::Ok
                                                                           : WidgetWithStatus::StatusType::Warning,
                                                          errors.isEmpty()
                                                            ? summary
                                                            : summary + QSL("\n\n") + errors.join(QL1C('\n')));
}

FeedIconRequest FormStandardFeedDetails::iconRequest(StandardFeed* feed) const {
  const auto& general = m_standardFeedDetails->m_ui;
  const auto& network = m_networkDetails->m_ui;
  const auto* auth = network.m_wdgAuthentication;
  FeedIconRequest request;
  request.m_sourceType =
    isChangeAllowed(general.m_mcbSourceType) ? m_standardFeedDetails->sourceType() : feed->sourceType();
  request.m_source =
    isChangeAllowed(general.m_mcbSource) ? general.m_txtSource->textEdit()->toPlainText() : feed->source();
  request.m_postProcessScript = isChangeAllowed(general.m_mcbPostProcessScript)
                                  ? general.m_txtPostProcessScript->textEdit()->toPlainText()
                                  : feed->postProcessScript();
  request.m_protection = isChangeAllowed(auth->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthType")))
                           ? auth->authenticationType()
                           : feed->protection();
  const bool credentials = isChangeAllowed(auth->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthentication")));
  request.m_username = credentials ? auth->username() : feed->username();
  request.m_password = credentials ? auth->password() : feed->password();
  request.m_headers =
    StandardFeed::httpHeadersToList(isChangeAllowed(network.m_mcbHttpHeaders) ? m_networkDetails->httpHeaders()
                                                                              : feed->httpHeaders());
  if (isChangeAllowed(network.m_mcbNetworkProxy)) {
    request.m_proxy =
      network.m_wdgNetworkProxy->useAccountProxy() ? m_serviceRoot->networkProxy() : network.m_wdgNetworkProxy->proxy();
  }
  else {
    request.m_proxy = feed->useAccountProxy() ? m_serviceRoot->networkProxy() : feed->networkProxy();
  }
  request.m_http2Status =
    isChangeAllowed(network.m_mcbEnableHttp2) ? m_networkDetails->http2Status() : feed->http2Status();
  request.m_ignoreCookies =
    isChangeAllowed(network.m_mcbIgnoreCookies) ? network.m_cbIgnoreCookies->isChecked() : feed->ignoreCookies();
  request.m_timeout =
    isChangeAllowed(network.m_mcbUpdateTimeout) ? network.m_spinUpdateTimeout->value() : feed->updateTimeout();
  if (request.m_timeout <= 0) {
    request.m_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  }
  return request;
}

void FormStandardFeedDetails::initializeIconFetching() {
  const auto& general = m_standardFeedDetails->m_ui;
  const auto& network = m_networkDetails->m_ui;
  connect(general.m_btnIcon, &ColorIconToolButton::iconChanged, this, [this]() {
    const bool was_individual = m_iconSelection.isIndividual();
    m_iconSelection.selectShared();
    m_standardFeedDetails->m_ui.m_btnIcon->setToolTip(tr("Select icon for your feed."));
    if (was_individual) {
      m_standardFeedDetails->m_ui.m_lblFetchMetadata
        ->setText(WidgetWithStatus::StatusType::Information,
                  tr("The selected icon will be applied to all selected feeds."));
    }
  });

  for (QPlainTextEdit* text :
       {general.m_txtSource->textEdit(), general.m_txtPostProcessScript->textEdit(), network.m_txtHttpHeaders}) {
    connect(text, &QPlainTextEdit::textChanged, this, &FormStandardFeedDetails::invalidateFetchedIcons);
  }
  for (QComboBox* combo : {general.m_cmbSourceType,
                           network.m_cmbEnableHttp2,
                           network.m_wdgAuthentication->findChild<QComboBox*>(QSL("m_cbAuthType"))}) {
    connect(combo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &FormStandardFeedDetails::invalidateFetchedIcons);
  }
  for (QLineEdit* edit : network.m_wdgAuthentication->findChildren<QLineEdit*>()) {
    connect(edit, &QLineEdit::textChanged, this, &FormStandardFeedDetails::invalidateFetchedIcons);
  }
  connect(network.m_wdgNetworkProxy,
          &NetworkProxyDetails::changed,
          this,
          &FormStandardFeedDetails::invalidateFetchedIcons);
  connect(network.m_spinUpdateTimeout,
          qOverload<int>(&QSpinBox::valueChanged),
          this,
          &FormStandardFeedDetails::invalidateFetchedIcons);
  connect(network.m_cbIgnoreCookies, &QCheckBox::toggled, this, &FormStandardFeedDetails::invalidateFetchedIcons);
  for (MultiFeedEditCheckBox* checkbox :
       {general.m_mcbSourceType,
        general.m_mcbSource,
        general.m_mcbPostProcessScript,
        network.m_mcbHttpHeaders,
        network.m_mcbNetworkProxy,
        network.m_mcbEnableHttp2,
        network.m_mcbIgnoreCookies,
        network.m_mcbUpdateTimeout,
        network.m_wdgAuthentication->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthType")),
        network.m_wdgAuthentication->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthentication"))}) {
    connect(checkbox, &QCheckBox::toggled, this, &FormStandardFeedDetails::invalidateFetchedIcons);
  }
}

void FormStandardFeedDetails::invalidateFetchedIcons() {
  if (m_iconSelection.isIndividual()) {
    m_iconSelection.invalidate();
    m_standardFeedDetails->m_ui.m_btnIcon->setToolTip(tr("Fetch icons again after changing feed settings."));
    m_standardFeedDetails->m_ui.m_lblFetchMetadata
      ->setText(WidgetWithStatus::StatusType::Warning,
                tr("Feed settings changed. Fetch icons again to apply individual icons."));
  }
}

void FormStandardFeedDetails::onTitleChanged(const QString& title) {
  buttonBox()->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(!title.simplified().isEmpty());
}

void FormStandardFeedDetails::apply() {
  FormFeedDetails::apply();

  RootItem* parent = m_standardFeedDetails->m_ui.m_cmbParentCategory->currentData().value<RootItem*>();
  StandardFeed::Type type =
    static_cast<StandardFeed::Type>(m_standardFeedDetails->m_ui.m_cmbType
                                      ->itemData(m_standardFeedDetails->m_ui.m_cmbType->currentIndex())
                                      .toInt());

  QList<StandardFeed*> fds = feeds<StandardFeed>();

  int feed_index = 0;
  for (StandardFeed* std_feed : std::as_const(fds)) {
    // Setup data for the feed.
    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbTitle)) {
      std_feed->setTitle(m_standardFeedDetails->m_ui.m_txtTitle->lineEdit()->text().simplified());
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbDescription)) {
      std_feed->setDescription(m_standardFeedDetails->m_ui.m_txtDescription->lineEdit()->text());
    }

    const auto icon = m_iconSelection.iconForFeed(feed_index++,
                                                  m_standardFeedDetails->m_ui.m_btnIcon->icon(),
                                                  isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbIcon));
    if (icon.has_value()) {
      std_feed->setIcon(icon.value());
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbSource)) {
      std_feed->setSource(m_standardFeedDetails->m_ui.m_txtSource->textEdit()->toPlainText());
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbSourceType)) {
      std_feed->setSourceType(m_standardFeedDetails->sourceType());
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbType)) {
      std_feed->setType(type);
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbEncoding)) {
      std_feed->setEncoding(m_standardFeedDetails->m_ui.m_cmbEncoding->currentText());
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbPostProcessScript)) {
      std_feed->setPostProcessScript(m_standardFeedDetails->m_ui.m_txtPostProcessScript->textEdit()->toPlainText());
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_mcbNetworkProxy)) {
      std_feed->setNetworkProxy(m_networkDetails->m_ui.m_wdgNetworkProxy->proxy());
      std_feed->setUseAccountProxy(m_networkDetails->m_ui.m_wdgNetworkProxy->useAccountProxy());
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_mcbNetworkProxyExtraDomains)) {
      std_feed
        ->setProxyExtraDomains(m_networkDetails->m_ui.m_txtNetworkProxyExtraDomains->toPlainText().split(QL1C('\n')));
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_wdgAuthentication
                          ->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthType")))) {
      std_feed->setProtection(m_networkDetails->m_ui.m_wdgAuthentication->authenticationType());
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_wdgAuthentication
                          ->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthentication")))) {
      std_feed->setUsername(m_networkDetails->m_ui.m_wdgAuthentication->username());
      std_feed->setPassword(m_networkDetails->m_ui.m_wdgAuthentication->password());
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_mcbHttpHeaders)) {
      std_feed->setHttpHeaders(m_networkDetails->httpHeaders());
    }

    if (isChangeAllowed(m_standardFeedExpDetails->m_ui.m_mcbDontUseRawXml)) {
      std_feed->setDontUseRawXmlSaving(m_standardFeedExpDetails->m_ui.m_cbDontUseRawXml->isChecked());
    }

    if (isChangeAllowed(m_standardFeedExpDetails->m_ui.m_mcbPublishedInsteadOfUpdated)) {
      std_feed
        ->setPublishedInsteadOfUpdatedTime(static_cast<
                                           StandardFeed::ArticleDateTimeBehavior>(m_standardFeedExpDetails->m_ui
                                                                                    .m_cmbPublishedInsteadOfUpdated
                                                                                    ->currentData()
                                                                                    .toInt()));
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_mcbEnableHttp2)) {
      std_feed->setHttp2Status(m_networkDetails->http2Status());
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_mcbUpdateTimeout)) {
      std_feed->setUpdateTimeout(m_networkDetails->m_ui.m_spinUpdateTimeout->value());
    }

    if (isChangeAllowed(m_networkDetails->m_ui.m_mcbIgnoreCookies)) {
      std_feed->setIgnoreCookies(m_networkDetails->m_ui.m_cbIgnoreCookies->isChecked());
    }

    if (isChangeAllowed(m_standardFeedExpDetails->m_ui.m_mcbFetchComments)) {
      std_feed->setFetchCommentsEnabled(m_standardFeedExpDetails->m_ui.m_cbFetchComments->isChecked());
    }

    if (isChangeAllowed(m_standardFeedExpDetails->m_ui.m_mcbReportAsBrokenIfEmpty)) {
      std_feed->setReportAsBrokenIfEmpty(m_standardFeedExpDetails->m_ui.m_cbReportAsBrokenIfEmpty->isChecked());
    }

    if (isChangeAllowed(m_standardFeedExpDetails->m_ui.m_mcbFetchFullArticles)) {
      std_feed->setFetchFullArticles(m_standardFeedExpDetails->m_ui.m_cbFetchFullArticles->isChecked());
      std_feed->setFetchFullArticlesInPlainText(m_standardFeedExpDetails->m_ui.m_cbFetchFullArticlesPlainTextOnly
                                                  ->isChecked());
    }

    std_feed->setCreationDate(QDateTime::currentDateTime());
    std_feed->setLastEtag({});

    int new_parent_id;

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbParentCategory)) {
      new_parent_id = parent->id();
    }
    else {
      new_parent_id = std_feed->parent()->id();
    }

    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      DatabaseQueries::createOverwriteFeed(db, std_feed, m_serviceRoot->accountId(), new_parent_id);
    });

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbParentCategory)) {
      m_serviceRoot->requestItemReassignment(std_feed, parent);
    }
  }

  m_serviceRoot->itemChanged(feeds<RootItem>());
}

void FormStandardFeedDetails::loadFeedData() {
  FormFeedDetails::loadFeedData();

  if (m_isBatchEdit) {
    m_standardFeedDetails->m_actionFetchIcon->setText(tr("Fetch icons for each feed"));
    // We hook batch selectors.
    m_standardFeedDetails->m_ui.m_mcbDescription->addActionWidget(m_standardFeedDetails->m_ui.m_txtDescription);
    m_standardFeedDetails->m_ui.m_mcbIcon->addActionWidget(m_standardFeedDetails->m_ui.m_btnIcon);
    m_standardFeedDetails->m_ui.m_mcbParentCategory->addActionWidget(m_standardFeedDetails->m_ui.m_cmbParentCategory);
    m_standardFeedDetails->m_ui.m_mcbPostProcessScript
      ->addActionWidget(m_standardFeedDetails->m_ui.m_txtPostProcessScript);
    m_standardFeedDetails->m_ui.m_mcbSourceType->addActionWidget(m_standardFeedDetails->m_ui.m_cmbSourceType);
    m_standardFeedDetails->m_ui.m_mcbSource->addActionWidget(m_standardFeedDetails->m_ui.m_txtSource);
    m_standardFeedDetails->m_ui.m_mcbTitle->addActionWidget(m_standardFeedDetails->m_ui.m_txtTitle);
    m_standardFeedDetails->m_ui.m_mcbType->addActionWidget(m_standardFeedDetails->m_ui.m_cmbType);
    m_standardFeedDetails->m_ui.m_mcbEncoding->addActionWidget(m_standardFeedDetails->m_ui.m_cmbEncoding);

    m_standardFeedDetails->m_ui.m_btnFetchMetadata->setEnabled(false);

    m_standardFeedExpDetails->m_ui.m_mcbPublishedInsteadOfUpdated
      ->addActionWidget(m_standardFeedExpDetails->m_ui.m_cmbPublishedInsteadOfUpdated);
    m_standardFeedExpDetails->m_ui.m_mcbDontUseRawXml
      ->addActionWidget(m_standardFeedExpDetails->m_ui.m_cbDontUseRawXml);
    m_standardFeedExpDetails->m_ui.m_mcbFetchComments
      ->addActionWidget(m_standardFeedExpDetails->m_ui.m_cbFetchComments);
    m_standardFeedExpDetails->m_ui.m_mcbReportAsBrokenIfEmpty
      ->addActionWidget(m_standardFeedExpDetails->m_ui.m_cbReportAsBrokenIfEmpty);

    m_standardFeedExpDetails->m_ui.m_mcbFetchFullArticles
      ->addActionWidget(m_standardFeedExpDetails->m_ui.m_wdgFetchFullArticles);

    m_networkDetails->m_ui.m_mcbNetworkProxy->addActionWidget(m_networkDetails->m_ui.m_wdgNetworkProxy);

    m_networkDetails->m_ui.m_wdgAuthentication->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthType"))
      ->addActionWidget(m_networkDetails->m_ui.m_wdgAuthentication->findChild<QComboBox*>(QSL("m_cbAuthType")));
    m_networkDetails->m_ui.m_wdgAuthentication->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthentication"))
      ->addActionWidget(m_networkDetails->m_ui.m_wdgAuthentication->findChild<QGroupBox*>(QSL("m_gbAuthentication")));

    m_networkDetails->m_ui.m_mcbHttpHeaders->addActionWidget(m_networkDetails->m_ui.m_txtHttpHeaders);
    m_networkDetails->m_ui.m_mcbUpdateTimeout->addActionWidget(m_networkDetails->m_ui.m_lblUpdateTimeout);
    m_networkDetails->m_ui.m_mcbUpdateTimeout->addActionWidget(m_networkDetails->m_ui.m_spinUpdateTimeout);
    m_networkDetails->m_ui.m_mcbEnableHttp2->addActionWidget(m_networkDetails->m_ui.m_lblEnableHttp2);
    m_networkDetails->m_ui.m_mcbEnableHttp2->addActionWidget(m_networkDetails->m_ui.m_cmbEnableHttp2);
    m_networkDetails->m_ui.m_mcbIgnoreCookies->addActionWidget(m_networkDetails->m_ui.m_cbIgnoreCookies);
    m_networkDetails->m_ui.m_mcbNetworkProxyExtraDomains
      ->addActionWidget(m_networkDetails->m_ui.m_txtNetworkProxyExtraDomains);
  }
  else {
    // We hide batch selectors.
    for (auto* cb : findChildren<MultiFeedEditCheckBox*>()) {
      cb->hide();
    }
  }

  auto* std_feed = feed<StandardFeed>();

  // Load categories.
  m_standardFeedDetails->loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  m_networkDetails->m_ui.m_wdgAuthentication->setAuthenticationType(std_feed->protection());
  m_networkDetails->m_ui.m_wdgAuthentication->setUsername(std_feed->username());
  m_networkDetails->m_ui.m_wdgAuthentication->setPassword(std_feed->password());
  m_networkDetails->m_ui.m_wdgNetworkProxy->setProxy(std_feed->networkProxy(), std_feed->useAccountProxy());
  m_networkDetails->m_ui.m_txtNetworkProxyExtraDomains->setPlainText(std_feed->proxyExtraDomains().join(QL1C('\n')));

  m_networkDetails->loadHttpHeaders(std_feed->httpHeaders());

  if (m_creatingNew) {
    m_standardFeedDetails->prepareForNewFeed(m_serviceRoot, m_parentToSelect, m_urlToProcess);
    m_standardFeedExpDetails->m_ui.m_cmbPublishedInsteadOfUpdated
      ->setCurrentIndex(m_standardFeedExpDetails->m_ui.m_cmbPublishedInsteadOfUpdated
                          ->findData(int(StandardFeed::ArticleDateTimeBehavior::Published)));
    m_standardFeedExpDetails->m_ui.m_cbFetchFullArticles->setChecked(false);
  }
  else {
    m_standardFeedDetails->setExistingFeed(m_serviceRoot, std_feed);

    m_standardFeedExpDetails->m_ui.m_cmbPublishedInsteadOfUpdated
      ->setCurrentIndex(m_standardFeedExpDetails->m_ui.m_cmbPublishedInsteadOfUpdated
                          ->findData(int(std_feed->publishedInsteadOfUpdatedTime())));
    m_standardFeedExpDetails->m_ui.m_cbDontUseRawXml->setChecked(std_feed->dontUseRawXmlSaving());
    m_standardFeedExpDetails->m_ui.m_cbFetchComments->setChecked(std_feed->fetchCommentsEnabled());
    m_standardFeedExpDetails->m_ui.m_cbReportAsBrokenIfEmpty->setChecked(std_feed->reportAsBrokenIfEmpty());
    m_standardFeedExpDetails->m_ui.m_cbFetchFullArticles->setChecked(std_feed->fetchFullArticles());
    m_standardFeedExpDetails->m_ui.m_cbFetchFullArticlesPlainTextOnly
      ->setChecked(std_feed->fetchFullArticlesInPlainText());

    m_networkDetails->setHttp2Status(std_feed->http2Status());
    m_networkDetails->m_ui.m_spinUpdateTimeout->setValue(std_feed->updateTimeout());
    m_networkDetails->m_ui.m_cbIgnoreCookies->setChecked(std_feed->ignoreCookies());
  }
}
