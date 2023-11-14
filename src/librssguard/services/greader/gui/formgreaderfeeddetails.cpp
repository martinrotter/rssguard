// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/gui/formgreaderfeeddetails.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "services/greader/definitions.h"
#include "services/greader/greaderfeed.h"
#include "services/greader/greadernetwork.h"
#include "services/greader/greaderserviceroot.h"
#include "services/greader/gui/greaderfeeddetails.h"

#include <QMimeData>
#include <QTimer>

FormGreaderFeedDetails::FormGreaderFeedDetails(ServiceRoot* service_root,
                                               RootItem* parent_to_select,
                                               const QString& url,
                                               QWidget* parent)
  : FormFeedDetails(service_root, parent), m_feedDetails(nullptr), m_parentToSelect(parent_to_select),
    m_urlToProcess(url) {}

void FormGreaderFeedDetails::apply() {
  if (!m_creatingNew) {
    if (!m_isBatchEdit) {
      GreaderFeed* fd = feed<GreaderFeed>();
      RootItem* parent = m_feedDetails->ui.m_cmbParentCategory->currentData().value<RootItem*>();
      const QString category_id = parent->kind() == RootItem::Kind::ServiceRoot ? QString() : parent->customId();

      try {
        fd->serviceRoot()->network()->subscriptionEdit(QSL(GREADER_API_EDIT_SUBSCRIPTION_MODIFY),
                                                       fd->customId(),
                                                       m_feedDetails->ui.m_txtTitle->lineEdit()->text(),
                                                       category_id,
                                                       {},
                                                       m_serviceRoot->networkProxy());
      }
      catch (const ApplicationException& ex) {
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             {tr("Feed NOT updated"),
                              tr("Error: %1").arg(ex.message()),
                              QSystemTrayIcon::MessageIcon::Critical},
                             GuiMessageDestination(true, true));
      }
    }

    // NOTE: We can only edit base properties, therefore
    // base method is fine.
    FormFeedDetails::apply();
  }
  else {
    RootItem* parent = m_feedDetails->ui.m_cmbParentCategory->currentData().value<RootItem*>();
    auto* root = qobject_cast<GreaderServiceRoot*>(parent->getParentServiceRoot());
    const QString category_id = parent->kind() == RootItem::Kind::ServiceRoot ? QString() : parent->customId();

    try {
      root->network()->subscriptionEdit(QSL(GREADER_API_EDIT_SUBSCRIPTION_ADD),
                                        QSL("feed/") + m_feedDetails->ui.m_txtUrl->lineEdit()->text(),
                                        m_feedDetails->ui.m_txtTitle->lineEdit()->text(),
                                        category_id,
                                        {},
                                        m_serviceRoot->networkProxy());

      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Feed added"),
                            tr("Feed was added, obtaining new tree of feeds now."),
                            QSystemTrayIcon::MessageIcon::Information});
      QTimer::singleShot(300, root, &GreaderServiceRoot::syncIn);
    }
    catch (const ApplicationException& ex) {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Feed NOT added"),
                            tr("Error: %1").arg(ex.message()),
                            QSystemTrayIcon::MessageIcon::Critical},
                           GuiMessageDestination(true, true));
    }
  }
}

void FormGreaderFeedDetails::loadFeedData() {
  FormFeedDetails::loadFeedData();

  if (!m_isBatchEdit) {
    m_feedDetails = new GreaderFeedDetails(this);

    insertCustomTab(m_feedDetails, tr("General"), 0);
    activateTab(0);

    GreaderFeed* fd = feed<GreaderFeed>();

    m_feedDetails->loadCategories(m_serviceRoot->getSubTreeCategories(),
                                  m_serviceRoot,
                                  m_creatingNew ? m_parentToSelect : fd->parent());

    if (m_creatingNew) {
      if (!m_urlToProcess.isEmpty()) {
        m_feedDetails->ui.m_txtUrl->lineEdit()->setText(m_urlToProcess);
      }

      m_feedDetails->ui.m_txtUrl->setFocus();
      m_feedDetails->ui.m_txtUrl->lineEdit()->selectAll();
    }
    else {
      m_feedDetails->ui.m_txtTitle->lineEdit()->setText(fd->title());
      m_feedDetails->ui.m_txtUrl->lineEdit()->setText(fd->source());
    }
  }
}
