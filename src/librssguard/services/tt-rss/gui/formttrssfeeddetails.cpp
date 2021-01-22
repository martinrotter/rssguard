// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/gui/formttrssfeeddetails.h"

#include "miscellaneous/application.h"
#include "services/abstract/feed.h"
#include "services/abstract/gui/authenticationdetails.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/ttrssfeeddetails.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrssserviceroot.h"

#include <QClipboard>
#include <QMimeData>
#include <QTimer>

FormTtRssFeedDetails::FormTtRssFeedDetails(ServiceRoot* service_root, QWidget* parent)
  : FormFeedDetails(service_root, parent), m_feedDetails(new TtRssFeedDetails(this)),
  m_authDetails(new AuthenticationDetails(this)) {}

int FormTtRssFeedDetails::addFeed(RootItem* parent_to_select, const QString& url) {
  clearTabs();
  insertCustomTab(m_feedDetails, tr("General"), 0);
  insertCustomTab(m_authDetails, tr("Network"), 1);
  activateTab(0);

  setWindowTitle(tr("Add new feed"));
  m_feedDetails->loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot, parent_to_select);

  if (!url.isEmpty()) {
    m_feedDetails->ui.m_txtUrl->lineEdit()->setText(url);
  }
  else if (Application::clipboard()->mimeData()->hasText()) {
    m_feedDetails->ui.m_txtUrl->lineEdit()->setText(Application::clipboard()->text());
  }

  m_feedDetails->ui.m_txtUrl->lineEdit()->selectAll();
  m_feedDetails->ui.m_txtUrl->setFocus();

  return exec();
}

void FormTtRssFeedDetails::apply() {
  if (m_editableFeed != nullptr) {
    // NOTE: We can only edit base properties, therefore
    // base method is fine.
    FormFeedDetails::apply();
  }
  else {
    RootItem* parent = static_cast<RootItem*>(m_feedDetails->ui.m_cmbParentCategory->itemData(
                                                m_feedDetails->ui.m_cmbParentCategory->currentIndex()).value<void*>());
    auto* root = qobject_cast<TtRssServiceRoot*>(parent->getParentServiceRoot());
    const int category_id = parent->kind() == RootItem::Kind::ServiceRoot ?
                            0 :
                            parent->customId().toInt();
    const TtRssSubscribeToFeedResponse response = root->network()->subscribeToFeed(m_feedDetails->ui.m_txtUrl->lineEdit()->text(),
                                                                                   category_id,
                                                                                   m_serviceRoot->networkProxy(),
                                                                                   m_authDetails->m_gbAuthentication->isChecked(),
                                                                                   m_authDetails->m_txtUsername->lineEdit()->text(),
                                                                                   m_authDetails->m_txtPassword->lineEdit()->text());

    if (response.code() == STF_INSERTED) {
      // Feed was added online.
      accept();
      qApp->showGuiMessage(tr("Feed added"),
                           tr("Feed was added, obtaining new tree of feeds now."),
                           QSystemTrayIcon::MessageIcon::Information);
      QTimer::singleShot(300, root, &TtRssServiceRoot::syncIn);
    }
    else {
      qApp->showGuiMessage(tr("Cannot add feed"),
                           tr("Feed was not added due to error."),
                           QSystemTrayIcon::MessageIcon::Critical,
                           qApp->mainFormWidget(),
                           true);
    }
  }
}
