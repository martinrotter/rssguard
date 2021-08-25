// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/gui/formttrssfeeddetails.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "services/abstract/feed.h"
#include "services/abstract/gui/authenticationdetails.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/ttrssfeeddetails.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceroot.h"

#include <QClipboard>
#include <QMimeData>
#include <QTimer>

FormTtRssFeedDetails::FormTtRssFeedDetails(ServiceRoot* service_root, RootItem* parent_to_select,
                                           const QString& url, QWidget* parent)
  : FormFeedDetails(service_root, parent), m_feedDetails(new TtRssFeedDetails(this)),
  m_authDetails(new AuthenticationDetails(this)), m_parentToSelect(parent_to_select),
  m_urlToProcess(url) {}

void FormTtRssFeedDetails::apply() {
  if (!m_creatingNew) {
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
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           tr("Feed added"),
                           tr("Feed was added, obtaining new tree of feeds now."),
                           QSystemTrayIcon::MessageIcon::Information);
      QTimer::singleShot(300, root, &TtRssServiceRoot::syncIn);
    }
    else {
      throw ApplicationException(tr("API returned error code %1").arg(QString::number(response.code())));
    }
  }
}

void FormTtRssFeedDetails::loadFeedData() {
  FormFeedDetails::loadFeedData();

  if (m_creatingNew) {
    insertCustomTab(m_feedDetails, tr("General"), 0);
    insertCustomTab(m_authDetails, tr("Network"), 1);
    activateTab(0);

    m_feedDetails->loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot, m_parentToSelect);

    if (!m_urlToProcess.isEmpty()) {
      m_feedDetails->ui.m_txtUrl->lineEdit()->setText(m_urlToProcess);
    }
    else if (Application::clipboard()->mimeData()->hasText()) {
      m_feedDetails->ui.m_txtUrl->lineEdit()->setText(Application::clipboard()->text());
    }

    m_feedDetails->ui.m_txtUrl->lineEdit()->selectAll();
    m_feedDetails->ui.m_txtUrl->setFocus();
  }
}
