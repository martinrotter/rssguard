// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/formstandardfeeddetails.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/gui/authenticationdetails.h"
#include "services/abstract/serviceroot.h"
#include "services/standard/gui/standardfeeddetails.h"
#include "services/standard/standardfeed.h"

#include <QFileDialog>
#include <QTextCodec>

FormStandardFeedDetails::FormStandardFeedDetails(ServiceRoot* service_root, QWidget* parent)
  : FormFeedDetails(service_root, parent), m_standardFeedDetails(new StandardFeedDetails(this)),
  m_authDetails(new AuthenticationDetails(this)) {
  insertCustomTab(m_standardFeedDetails, tr("General"), 0);
  insertCustomTab(m_authDetails, tr("Network"), 2);
  activateTab(0);

  connect(m_standardFeedDetails->ui.m_btnFetchMetadata, &QPushButton::clicked, this, &FormStandardFeedDetails::guessFeed);
  connect(m_standardFeedDetails->m_actionFetchIcon, &QAction::triggered, this, &FormStandardFeedDetails::guessIconOnly);
}

int FormStandardFeedDetails::addEditFeed(StandardFeed* input_feed, RootItem* parent_to_select, const QString& url) {
  // Load categories.
  m_standardFeedDetails->loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot);

  if (input_feed == nullptr) {
    // User is adding new feed.
    setWindowTitle(tr("Add new feed"));
    m_standardFeedDetails->prepareForNewFeed(parent_to_select, url);
  }
  else {
    setEditableFeed(input_feed);
  }

  // Run the dialog.
  return exec();
}

void FormStandardFeedDetails::guessFeed() {
  m_standardFeedDetails->guessFeed(m_standardFeedDetails->ui.m_txtUrl->lineEdit()->text(),
                                   m_authDetails->m_txtUsername->lineEdit()->text(),
                                   m_authDetails->m_txtPassword->lineEdit()->text());
}

void FormStandardFeedDetails::guessIconOnly() {
  m_standardFeedDetails->guessIconOnly(m_standardFeedDetails->ui.m_txtUrl->lineEdit()->text(),
                                       m_authDetails->m_txtUsername->lineEdit()->text(),
                                       m_authDetails->m_txtPassword->lineEdit()->text());
}

void FormStandardFeedDetails::apply() {
  RootItem* parent =
    static_cast<RootItem*>(m_standardFeedDetails->ui.m_cmbParentCategory->itemData(
                             m_standardFeedDetails->ui.m_cmbParentCategory->currentIndex()).value<void*>());

  StandardFeed::Type type =
    static_cast<StandardFeed::Type>(m_standardFeedDetails->ui.m_cmbType->itemData(m_standardFeedDetails->ui.m_cmbType->currentIndex()).value<int>());
  auto* new_feed = new StandardFeed();

  // Setup data for new_feed.
  new_feed->setTitle(m_standardFeedDetails->ui.m_txtTitle->lineEdit()->text());
  new_feed->setCreationDate(QDateTime::currentDateTime());
  new_feed->setDescription(m_standardFeedDetails->ui.m_txtDescription->lineEdit()->text());
  new_feed->setIcon(m_standardFeedDetails->ui.m_btnIcon->icon());
  new_feed->setEncoding(m_standardFeedDetails->ui.m_cmbEncoding->currentText());
  new_feed->setType(type);
  new_feed->setUrl(m_standardFeedDetails->ui.m_txtUrl->lineEdit()->text());
  new_feed->setPasswordProtected(m_authDetails->m_gbAuthentication->isChecked());
  new_feed->setUsername(m_authDetails->m_txtUsername->lineEdit()->text());
  new_feed->setPassword(m_authDetails->m_txtPassword->lineEdit()->text());
  new_feed->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(
                                                                  m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
  new_feed->setAutoUpdateInitialInterval(int(m_ui->m_spinAutoUpdateInterval->value()));

  if (m_editableFeed == nullptr) {
    // Add the feed.
    if (new_feed->addItself(parent)) {
      m_serviceRoot->requestItemReassignment(new_feed, parent);
      accept();
    }
    else {
      delete new_feed;
      qApp->showGuiMessage(tr("Cannot add feed"),
                           tr("Feed was not added due to error."),
                           QSystemTrayIcon::MessageIcon::Critical, this, true);
    }
  }
  else {
    new_feed->setParent(parent);

    // Edit the feed.
    bool edited = qobject_cast<StandardFeed*>(m_editableFeed)->editItself(new_feed);

    if (edited) {
      m_serviceRoot->requestItemReassignment(m_editableFeed, new_feed->parent());
      accept();
    }
    else {
      qApp->showGuiMessage(tr("Cannot edit feed"),
                           tr("Feed was not edited due to error."),
                           QSystemTrayIcon::MessageIcon::Critical, this, true);
    }

    delete new_feed;
  }
}

void FormStandardFeedDetails::setEditableFeed(Feed* editable_feed) {
  FormFeedDetails::setEditableFeed(editable_feed);

  m_standardFeedDetails->setExistingFeed(qobject_cast<StandardFeed*>(editable_feed));
  m_authDetails->m_gbAuthentication->setChecked(editable_feed->passwordProtected());
  m_authDetails->m_txtUsername->lineEdit()->setText(editable_feed->username());
  m_authDetails->m_txtPassword->lineEdit()->setText(editable_feed->password());
}
