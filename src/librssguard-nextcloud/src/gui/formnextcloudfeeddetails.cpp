// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formnextcloudfeeddetails.h"

#include "src/gui/nextcloudfeeddetails.h"
#include "src/nextcloudfeed.h"
#include "src/nextcloudnetworkfactory.h"
#include "src/nextcloudserviceroot.h"

#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/services/abstract/gui/authenticationdetails.h>

#include <QMimeData>
#include <QTimer>

FormNextcloudFeedDetails::FormNextcloudFeedDetails(ServiceRoot* service_root,
                                                   RootItem* parent_to_select,
                                                   const QString& url,
                                                   QWidget* parent)
  : FormFeedDetails(service_root, parent), m_feedDetails(new NextcloudFeedDetails(this)),
    m_parentToSelect(parent_to_select), m_urlToProcess(url) {}

void FormNextcloudFeedDetails::apply() {
  if (!m_creatingNew) {
    // NOTE: We can only edit base properties, therefore
    // base method is fine.
    FormFeedDetails::apply();
  }
  else {
    RootItem* parent = m_feedDetails->ui.m_cmbParentCategory->currentData().value<RootItem*>();
    auto* root = qobject_cast<NextcloudServiceRoot*>(parent->account());
    const int category_id = parent->kind() == RootItem::Kind::ServiceRoot ? 0 : parent->customNumericId();

    root->network()->createFeed(m_feedDetails->ui.m_txtUrl->lineEdit()->text(), category_id, root->networkProxy());

    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Feed added"),
                          tr("Feed was added, refreshing feed tree..."),
                          QSystemTrayIcon::MessageIcon::Information});
    QTimer::singleShot(600, root, &NextcloudServiceRoot::requestSyncIn);
    feed<NextcloudFeed>()->deleteLater();
  }
}

void FormNextcloudFeedDetails::loadFeedData() {
  FormFeedDetails::loadFeedData();

  if (m_creatingNew) {
    removeAllTabs();
    insertCustomTab(m_feedDetails, tr("General"));

    m_feedDetails->loadCategories(m_serviceRoot->getSubTreeCategories(), m_serviceRoot, m_parentToSelect);

    if (!m_urlToProcess.isEmpty()) {
      m_feedDetails->ui.m_txtUrl->lineEdit()->setText(m_urlToProcess);
    }

    m_feedDetails->ui.m_txtUrl->lineEdit()->selectAll();
    m_feedDetails->ui.m_txtUrl->setFocus();
  }
}
