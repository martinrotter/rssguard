// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formgreaderfeeddetails.h"

#include "src/definitions.h"
#include "src/greaderfeed.h"
#include "src/greadernetwork.h"
#include "src/greaderserviceroot.h"
#include "src/gui/greaderfeeddetails.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/miscellaneous/application.h>

#include <QMimeData>
#include <QTimer>

FormGreaderFeedDetails::FormGreaderFeedDetails(ServiceRoot* service_root,
                                               RootItem* parent_to_select,
                                               const QString& url,
                                               QWidget* parent)
  : FormFeedDetails(service_root, parent), m_feedDetails(nullptr), m_parentToSelect(parent_to_select),
    m_urlToProcess(url) {}

void FormGreaderFeedDetails::apply() {
  GreaderFeed* fd = feed<GreaderFeed>();
  GreaderServiceRoot* root = qobject_cast<GreaderServiceRoot*>(m_serviceRoot);
  RootItem* parent =
    m_feedDetails != nullptr ? m_feedDetails->ui.m_cmbParentCategory->currentData().value<RootItem*>() : nullptr;

  if (m_creatingNew || !m_isBatchEdit) {
    QString feed_id = m_creatingNew ? (QSL("feed/") + m_feedDetails->ui.m_txtUrl->lineEdit()->text()) : fd->customId();
    QString category_to_add = parent->kind() == RootItem::Kind::ServiceRoot ? QString() : parent->customId();
    QString category_to_remove =
      (!m_creatingNew && fd->parent()->customId() != category_to_add) ? fd->parent()->customId() : QString();

    try {
      // Change online
      root->network()->subscriptionEdit(m_creatingNew ? QSL(GREADER_API_EDIT_SUBSCRIPTION_ADD)
                                                      : QSL(GREADER_API_EDIT_SUBSCRIPTION_MODIFY),
                                        feed_id,
                                        m_feedDetails->ui.m_txtTitle->lineEdit()->text(),
                                        QUrl::toPercentEncoding(category_to_add),
                                        QUrl::toPercentEncoding(category_to_remove),
                                        m_serviceRoot->networkProxy());

      if (m_creatingNew) {
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             {tr("Feed added"),
                              tr("Feed was added, refreshing feed tree..."),
                              QSystemTrayIcon::MessageIcon::Information});
        QTimer::singleShot(300, root, &GreaderServiceRoot::syncIn);

        // NOTE: Feed object was not used, delete.
        fd->deleteLater();
        return;
      }
      else {
        fd->setTitle(m_feedDetails->ui.m_txtTitle->lineEdit()->text());
      }
    }
    catch (const ApplicationException& ex) {
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Feed NOT updated or added"),
                            tr("Error: %1").arg(ex.message()),
                            QSystemTrayIcon::MessageIcon::Critical},
                           GuiMessageDestination(true, true));
      return;
    }
  }

  FormFeedDetails::apply();

  if (!m_isBatchEdit) {
    try {
      QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
      DatabaseQueries::createOverwriteFeed(database, fd, m_serviceRoot->accountId(), parent->id());
    }
    catch (const ApplicationException& ex) {
      qFatal("Cannot save feed: '%s'.", qPrintable(ex.message()));
    }

    m_serviceRoot->requestItemReassignment(fd, parent);
    m_serviceRoot->itemChanged({fd});
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

      // NOTE: User cannot edit URL.
      m_feedDetails->ui.m_lblUrl->hide();
      m_feedDetails->ui.m_txtUrl->hide();
    }
  }
}
