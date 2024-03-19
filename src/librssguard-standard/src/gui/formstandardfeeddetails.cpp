// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formstandardfeeddetails.h"

#include "src/gui/standardfeeddetails.h"
#include "src/standardfeed.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/network-web/webfactory.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/gui/authenticationdetails.h>
#include <librssguard/services/abstract/serviceroot.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QNetworkCookie>

FormStandardFeedDetails::FormStandardFeedDetails(ServiceRoot* service_root,
                                                 RootItem* parent_to_select,
                                                 const QString& url,
                                                 QWidget* parent)
  : FormFeedDetails(service_root, parent), m_standardFeedDetails(new StandardFeedDetails(this)),
    m_authDetails(new AuthenticationDetails(false, this)), m_parentToSelect(parent_to_select), m_urlToProcess(url) {
  insertCustomTab(m_standardFeedDetails, tr("General"), 0);
  insertCustomTab(m_authDetails, tr("Network"), 2);
  activateTab(0);

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
                                   m_authDetails->authenticationType(),
                                   m_authDetails->username(),
                                   m_authDetails->password(),
                                   m_serviceRoot->networkProxy());
}

void FormStandardFeedDetails::guessIconOnly() {
  m_standardFeedDetails->guessIconOnly(m_standardFeedDetails->sourceType(),
                                       m_standardFeedDetails->m_ui.m_txtSource->textEdit()->toPlainText(),
                                       m_standardFeedDetails->m_ui.m_txtPostProcessScript->textEdit()->toPlainText(),
                                       m_authDetails->authenticationType(),
                                       m_authDetails->username(),
                                       m_authDetails->password(),
                                       m_serviceRoot->networkProxy());
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

    if (isChangeAllowed(m_authDetails->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthType")))) {
      std_feed->setProtection(m_authDetails->authenticationType());
    }

    if (isChangeAllowed(m_authDetails->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthentication")))) {
      std_feed->setUsername(m_authDetails->username());
      std_feed->setPassword(m_authDetails->password());
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

    m_authDetails->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthType"))
      ->addActionWidget(m_authDetails->findChild<QComboBox*>(QSL("m_cbAuthType")));
    m_authDetails->findChild<MultiFeedEditCheckBox*>(QSL("m_mcbAuthentication"))
      ->addActionWidget(m_authDetails->findChild<QGroupBox*>(QSL("m_gbAuthentication")));

    m_standardFeedDetails->m_ui.m_btnFetchMetadata->setEnabled(false);
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

  m_authDetails->setAuthenticationType(std_feed->protection());
  m_authDetails->setUsername(std_feed->username());
  m_authDetails->setPassword(std_feed->password());

  if (m_creatingNew) {
    // auto processed_url = qApp->web()->processFeedUriScheme(m_urlToProcess);

    m_standardFeedDetails->prepareForNewFeed(m_parentToSelect, m_urlToProcess);
  }
  else {
    m_standardFeedDetails->setExistingFeed(std_feed);
  }
}
