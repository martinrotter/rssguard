// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formstandardfeeddetails.h"

#include "src/gui/standardfeeddetails.h"
#include "src/gui/standardfeedexpdetails.h"
#include "src/gui/standardfeednetworkdetails.h"
#include "src/standardfeed.h"
#include "src/standardserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/gui/reusable/networkproxydetails.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/network-web/webfactory.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/serviceroot.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QNetworkCookie>

FormStandardFeedDetails::FormStandardFeedDetails(ServiceRoot* service_root,
                                                 RootItem* parent_to_select,
                                                 const QString& url,
                                                 QWidget* parent)
  : FormFeedDetails(service_root, parent), m_standardFeedDetails(new StandardFeedDetails(this)),
    m_standardFeedExpDetails(new StandardFeedExpDetails(this)), m_networkDetails(new StandardFeedNetworkDetails(this)),
    m_parentToSelect(parent_to_select), m_urlToProcess(url) {
  insertCustomTab(m_standardFeedDetails, tr("General"), 0);
  insertCustomTab(m_networkDetails, tr("Network"));
  insertCustomTab(m_standardFeedExpDetails, tr("Experimental"));
  activateTab(0);

  m_standardFeedDetails->setNetworkDetails(m_networkDetails);

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
                                   m_networkDetails->http2Status());
}

void FormStandardFeedDetails::guessIconOnly() {
  m_standardFeedDetails->guessIconOnly(m_standardFeedDetails->sourceType(),
                                       m_standardFeedDetails->m_ui.m_txtSource->textEdit()->toPlainText(),
                                       m_standardFeedDetails->m_ui.m_txtPostProcessScript->textEdit()->toPlainText(),
                                       qobject_cast<StandardServiceRoot*>(m_serviceRoot),
                                       m_networkDetails->m_ui.m_wdgAuthentication->authenticationType(),
                                       m_networkDetails->m_ui.m_wdgAuthentication->username(),
                                       m_networkDetails->m_ui.m_wdgAuthentication->password(),
                                       StandardFeed::httpHeadersToList(m_networkDetails->httpHeaders()),
                                       m_networkDetails->m_ui.m_wdgNetworkProxy->useAccountProxy()
                                         ? m_serviceRoot->networkProxy()
                                         : m_networkDetails->m_ui.m_wdgNetworkProxy->proxy());
}

void FormStandardFeedDetails::onTitleChanged(const QString& title) {
  buttonBox()->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(!title.simplified().isEmpty());
}

void FormStandardFeedDetails::apply() {
  FormFeedDetails::apply();

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  RootItem* parent = m_standardFeedDetails->m_ui.m_cmbParentCategory->currentData().value<RootItem*>();
  StandardFeed::Type type =
    static_cast<StandardFeed::Type>(m_standardFeedDetails->m_ui.m_cmbType
                                      ->itemData(m_standardFeedDetails->m_ui.m_cmbType->currentIndex())
                                      .toInt());

  QList<StandardFeed*> fds = feeds<StandardFeed>();

  for (StandardFeed* std_feed : fds) {
    // Setup data for the feed.
    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbTitle)) {
      std_feed->setTitle(m_standardFeedDetails->m_ui.m_txtTitle->lineEdit()->text().simplified());
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbDescription)) {
      std_feed->setDescription(m_standardFeedDetails->m_ui.m_txtDescription->lineEdit()->text());
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbIcon)) {
      std_feed->setIcon(m_standardFeedDetails->m_ui.m_btnIcon->icon());
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

    if (isChangeAllowed(m_networkDetails->m_ui.m_mcbEnableHttp2)) {
      std_feed->setHttp2Status(m_networkDetails->http2Status());
    }

    if (isChangeAllowed(m_standardFeedExpDetails->m_ui.m_mcbFetchComments)) {
      std_feed->setFetchCommentsEnabled(m_standardFeedExpDetails->m_ui.m_cbFetchComments->isChecked());
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

    try {
      DatabaseQueries::createOverwriteFeed(database, std_feed, m_serviceRoot->accountId(), new_parent_id);
    }
    catch (const ApplicationException& ex) {
      qFatal("Cannot save feed: '%s'.", qPrintable(ex.message()));
    }

    if (isChangeAllowed(m_standardFeedDetails->m_ui.m_mcbParentCategory)) {
      m_serviceRoot->requestItemReassignment(std_feed, parent);
    }
  }

  m_serviceRoot->itemChanged(feeds<RootItem>());
}

void FormStandardFeedDetails::loadFeedData() {
  FormFeedDetails::loadFeedData();

  if (m_isBatchEdit) {
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

    m_standardFeedExpDetails->m_ui.m_mcbDontUseRawXml
      ->addActionWidget(m_standardFeedExpDetails->m_ui.m_cbDontUseRawXml);
    m_standardFeedExpDetails->m_ui.m_mcbFetchComments
      ->addActionWidget(m_standardFeedExpDetails->m_ui.m_cbFetchComments);

    m_networkDetails->m_ui.m_mcbNetworkProxy->addActionWidget(m_networkDetails->m_ui.m_wdgNetworkProxy);

    m_networkDetails->m_ui.m_wdgAuthentication->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthType"))
      ->addActionWidget(m_networkDetails->m_ui.m_wdgAuthentication->findChild<QComboBox*>(QSL("m_cbAuthType")));
    m_networkDetails->m_ui.m_wdgAuthentication->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthentication"))
      ->addActionWidget(m_networkDetails->m_ui.m_wdgAuthentication->findChild<QGroupBox*>(QSL("m_gbAuthentication")));

    m_networkDetails->m_ui.m_mcbHttpHeaders->addActionWidget(m_networkDetails->m_ui.m_txtHttpHeaders);

    m_networkDetails->m_ui.m_mcbEnableHttp2->addActionWidget(m_networkDetails->m_ui.m_lblEnableHttp2);
    m_networkDetails->m_ui.m_mcbEnableHttp2->addActionWidget(m_networkDetails->m_ui.m_cmbEnableHttp2);
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

  m_networkDetails->loadHttpHeaders(std_feed->httpHeaders());

  if (m_creatingNew) {
    // auto processed_url = qApp->web()->processFeedUriScheme(m_urlToProcess);

    m_standardFeedDetails->prepareForNewFeed(m_serviceRoot, m_parentToSelect, m_urlToProcess);
  }
  else {
    m_standardFeedDetails->setExistingFeed(m_serviceRoot, std_feed);
    m_standardFeedExpDetails->m_ui.m_cbDontUseRawXml->setChecked(std_feed->dontUseRawXmlSaving());
    m_standardFeedExpDetails->m_ui.m_cbFetchComments->setChecked(std_feed->fetchCommentsEnabled());
    m_networkDetails->setHttp2Status(std_feed->http2Status());
  }
}
