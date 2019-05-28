// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/gui/formowncloudfeeddetails.h"

#include "miscellaneous/application.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"
#include "services/owncloud/owncloudfeed.h"
#include "services/owncloud/owncloudserviceroot.h"

#include <QTimer>

FormOwnCloudFeedDetails::FormOwnCloudFeedDetails(ServiceRoot* service_root, QWidget* parent)
  : FormFeedDetails(service_root, parent) {
  m_ui->m_spinAutoUpdateInterval->setEnabled(false);
  m_ui->m_cmbAutoUpdateType->setEnabled(false);
  m_ui->m_cmbType->setEnabled(false);
  m_ui->m_cmbEncoding->setEnabled(false);
  m_ui->m_btnFetchMetadata->setEnabled(false);
  m_ui->m_btnIcon->setEnabled(false);
  m_ui->m_txtTitle->setEnabled(false);
  m_ui->m_txtUrl->setEnabled(true);
  m_ui->m_txtDescription->setEnabled(false);
}

void FormOwnCloudFeedDetails::apply() {
  if (m_editableFeed != nullptr) {
    bool renamed = false;

    if (m_ui->m_txtTitle->lineEdit()->text() != m_editableFeed->title()) {
      if (!qobject_cast<OwnCloudServiceRoot*>(m_serviceRoot)->network()->renameFeed(m_ui->m_txtTitle->lineEdit()->text(),
                                                                                    m_editableFeed->customId())) {
        qWarning("ownCloud: Problem with feed renaming ID '%s'.", qPrintable(m_editableFeed->customId()));
      }
      else {
        renamed = true;
      }
    }

    // User edited auto-update status. Save it.
    auto* new_feed_data = new OwnCloudFeed();

    new_feed_data->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(
                                                                         m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
    new_feed_data->setAutoUpdateInitialInterval(int(m_ui->m_spinAutoUpdateInterval->value()));
    qobject_cast<OwnCloudFeed*>(m_editableFeed)->editItself(new_feed_data);
    delete new_feed_data;

    if (renamed) {
      QTimer::singleShot(200, m_serviceRoot, SLOT(syncIn()));
    }
  }
  else {
    const RootItem* parent = static_cast<RootItem*>(m_ui->m_cmbParentCategory->itemData(
                                                      m_ui->m_cmbParentCategory->currentIndex()).value<void*>());
    const int category_id = parent->kind() == RootItemKind::ServiceRoot ? 0 : parent->customId().toInt();
    const bool response = qobject_cast<OwnCloudServiceRoot*>(m_serviceRoot)->network()->createFeed(m_ui->m_txtUrl->lineEdit()->text(),
                                                                                                   category_id);

    if (response) {
      // Feed was added online.
      accept();
      qApp->showGuiMessage(tr("Feed added"), tr("Feed was added, triggering sync in now."), QSystemTrayIcon::Information);
      QTimer::singleShot(100, m_serviceRoot, SLOT(syncIn()));
    }
    else {
      reject();
      qApp->showGuiMessage(tr("Cannot add feed"),
                           tr("Feed was not added due to error."),
                           QSystemTrayIcon::Critical, qApp->mainFormWidget(), true);
    }
  }

  accept();
}

void FormOwnCloudFeedDetails::setEditableFeed(Feed* editable_feed) {
  m_ui->m_cmbAutoUpdateType->setEnabled(true);
  FormFeedDetails::setEditableFeed(editable_feed);
  m_ui->m_txtTitle->setEnabled(true);
  m_ui->m_gbAuthentication->setEnabled(false);
  m_ui->m_txtUrl->setEnabled(false);
  m_ui->m_lblParentCategory->setEnabled(false);
  m_ui->m_cmbParentCategory->setEnabled(false);
}
