// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/formstandardfeeddetails.h"

#include "database/databasequeries.h"
#include "exceptions/applicationexception.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/cookiejar.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/gui/authenticationdetails.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/definitions.h"
#include "services/standard/gui/standardfeeddetails.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardserviceroot.h"

#include <QFileDialog>
#include <QNetworkCookie>
#include <QTextCodec>

FormStandardFeedDetails::FormStandardFeedDetails(ServiceRoot* service_root, RootItem* parent_to_select,
                                                 const QString& url, QWidget* parent)
  : FormFeedDetails(service_root, parent), m_standardFeedDetails(new StandardFeedDetails(this)),
  m_authDetails(new AuthenticationDetails(this)), m_parentToSelect(parent_to_select), m_urlToProcess(url) {
  insertCustomTab(m_standardFeedDetails, tr("General"), 0);
  insertCustomTab(m_authDetails, tr("Network"), 2);
  activateTab(0);

  connect(m_standardFeedDetails->m_ui.m_btnFetchMetadata, &QPushButton::clicked, this, &FormStandardFeedDetails::guessFeed);
  connect(m_standardFeedDetails->m_actionFetchIcon, &QAction::triggered, this, &FormStandardFeedDetails::guessIconOnly);
  connect(m_standardFeedDetails->m_ui.m_txtTitle->lineEdit(), &QLineEdit::textChanged,
          this, &FormStandardFeedDetails::onTitleChanged);

  onTitleChanged(m_standardFeedDetails->m_ui.m_txtTitle->lineEdit()->text());
}

void FormStandardFeedDetails::guessFeed() {
  m_standardFeedDetails->guessFeed(m_standardFeedDetails->sourceType(),
                                   m_standardFeedDetails->m_ui.m_txtSource->textEdit()->toPlainText(),
                                   m_standardFeedDetails->m_ui.m_txtPostProcessScript->textEdit()->toPlainText(),
                                   m_authDetails->m_txtUsername->lineEdit()->text(),
                                   m_authDetails->m_txtPassword->lineEdit()->text(),
                                   m_serviceRoot->networkProxy());
}

void FormStandardFeedDetails::guessIconOnly() {
  m_standardFeedDetails->guessIconOnly(m_standardFeedDetails->sourceType(),
                                       m_standardFeedDetails->m_ui.m_txtSource->textEdit()->toPlainText(),
                                       m_standardFeedDetails->m_ui.m_txtPostProcessScript->textEdit()->toPlainText(),
                                       m_authDetails->m_txtUsername->lineEdit()->text(),
                                       m_authDetails->m_txtPassword->lineEdit()->text(),
                                       m_serviceRoot->networkProxy());
}

void FormStandardFeedDetails::onTitleChanged(const QString& title) {
  m_ui->m_buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(!title.simplified().isEmpty());
}

void FormStandardFeedDetails::apply() {
  FormFeedDetails::apply();

  auto* std_feed = feed<StandardFeed>();
  RootItem* parent =
    static_cast<RootItem*>(m_standardFeedDetails->m_ui.m_cmbParentCategory->itemData(
                             m_standardFeedDetails->m_ui.m_cmbParentCategory->currentIndex()).value<void*>());

  StandardFeed::Type type =
    static_cast<StandardFeed::Type>(m_standardFeedDetails->m_ui.m_cmbType->itemData(m_standardFeedDetails->m_ui.m_cmbType->currentIndex()).value<int>());

  // Setup data for new_feed.
  std_feed->setTitle(m_standardFeedDetails->m_ui.m_txtTitle->lineEdit()->text().simplified());
  std_feed->setCreationDate(QDateTime::currentDateTime());
  std_feed->setDescription(m_standardFeedDetails->m_ui.m_txtDescription->lineEdit()->text());
  std_feed->setIcon(m_standardFeedDetails->m_ui.m_btnIcon->icon());

  std_feed->setSource(m_standardFeedDetails->m_ui.m_txtSource->textEdit()->toPlainText());

  std_feed->setEncoding(m_standardFeedDetails->m_ui.m_cmbEncoding->currentText());
  std_feed->setType(type);
  std_feed->setSourceType(m_standardFeedDetails->sourceType());
  std_feed->setPostProcessScript(m_standardFeedDetails->m_ui.m_txtPostProcessScript->textEdit()->toPlainText());
  std_feed->setPasswordProtected(m_authDetails->m_gbAuthentication->isChecked());
  std_feed->setUsername(m_authDetails->m_txtUsername->lineEdit()->text());
  std_feed->setPassword(m_authDetails->m_txtPassword->lineEdit()->text());

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::createOverwriteFeed(database, std_feed, m_serviceRoot->accountId(), parent->id());
  }
  catch (const ApplicationException& ex) {
    qFatal("Cannot save feed: '%s'.", qPrintable(ex.message()));
  }

  m_serviceRoot->requestItemReassignment(m_feed, parent);
  m_serviceRoot->itemChanged({ m_feed });
}

void FormStandardFeedDetails::loadFeedData() {
  FormFeedDetails::loadFeedData();

  auto* std_feed = feed<StandardFeed>();

  // Load categories.
  m_standardFeedDetails->loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  m_authDetails->m_gbAuthentication->setChecked(std_feed->passwordProtected());
  m_authDetails->m_txtUsername->lineEdit()->setText(std_feed->username());
  m_authDetails->m_txtPassword->lineEdit()->setText(std_feed->password());

  if (m_creatingNew) {
    auto processed_url = qApp->web()->processFeedUriScheme(m_urlToProcess);

    m_standardFeedDetails->prepareForNewFeed(m_parentToSelect, processed_url);
  }
  else {
    m_standardFeedDetails->setExistingFeed(std_feed);
  }
}
