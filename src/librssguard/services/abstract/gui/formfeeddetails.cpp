// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formfeeddetails.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/baselineedit.h"
#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/rootitem.h"
#include "services/standard/standardfeed.h"
#include "services/standard/standardserviceroot.h"

#include <QMenu>
#include <QNetworkReply>
#include <QPair>
#include <QPushButton>
#include <QTextCodec>

FormFeedDetails::FormFeedDetails(ServiceRoot* service_root, QWidget* parent)
  : QDialog(parent), m_editableFeed(nullptr), m_serviceRoot(service_root) {
  initialize();
  createConnections();
}

int FormFeedDetails::editBaseFeed(Feed* input_feed) {
  setEditableFeed(input_feed);
  return QDialog::exec();
}

void FormFeedDetails::activateTab(int index) {
  m_ui->m_tabWidget->setCurrentIndex(index);
}

void FormFeedDetails::clearTabs() {
  m_ui->m_tabWidget->clear();
}

void FormFeedDetails::insertCustomTab(QWidget* custom_tab, const QString& title, int index) {
  m_ui->m_tabWidget->insertTab(index, custom_tab, title);
}

void FormFeedDetails::apply() {
  Feed new_feed;

  // Setup data for new_feed.
  new_feed.setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(
                                                                 m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
  new_feed.setAutoUpdateInitialInterval(int(m_ui->m_spinAutoUpdateInterval->value()));

  if (m_editableFeed != nullptr) {
    // Edit the feed.
    bool edited = m_editableFeed->editItself(&new_feed);

    if (edited) {
      accept();
    }
    else {
      qApp->showGuiMessage(tr("Cannot edit feed"),
                           tr("Feed was not edited due to error."),
                           QSystemTrayIcon::MessageIcon::Critical, this, true);
    }
  }
}

void FormFeedDetails::onAutoUpdateTypeChanged(int new_index) {
  Feed::AutoUpdateType auto_update_type = static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(new_index).toInt());

  switch (auto_update_type) {
    case Feed::AutoUpdateType::DontAutoUpdate:
    case Feed::AutoUpdateType::DefaultAutoUpdate:
      m_ui->m_spinAutoUpdateInterval->setEnabled(false);
      break;

    default:
      m_ui->m_spinAutoUpdateInterval->setEnabled(true);
  }
}

void FormFeedDetails::createConnections() {
  // General connections.
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormFeedDetails::apply);
  connect(m_ui->m_cmbAutoUpdateType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &FormFeedDetails::onAutoUpdateTypeChanged);
}

void FormFeedDetails::setEditableFeed(Feed* editable_feed) {
  setWindowTitle(tr("Edit '%1'").arg(editable_feed->title()));

  m_editableFeed = editable_feed;

  m_ui->m_cmbAutoUpdateType->setCurrentIndex(m_ui->m_cmbAutoUpdateType->findData(QVariant::fromValue((int) editable_feed->autoUpdateType())));
  m_ui->m_spinAutoUpdateInterval->setValue(editable_feed->autoUpdateInitialInterval());
}

void FormFeedDetails::initialize() {
  m_ui.reset(new Ui::FormFeedDetails());
  m_ui->setupUi(this);

  // Set flags and attributes.
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("application-rss+xml")));

  // Setup auto-update options.
  m_ui->m_spinAutoUpdateInterval->setValue(DEFAULT_AUTO_UPDATE_INTERVAL);
  m_ui->m_cmbAutoUpdateType->addItem(tr("Download messages using global interval"),
                                     QVariant::fromValue(int(Feed::AutoUpdateType::DefaultAutoUpdate)));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Download messages every"),
                                     QVariant::fromValue(int(Feed::AutoUpdateType::SpecificAutoUpdate)));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Do not download messages at all"),
                                     QVariant::fromValue(int(Feed::AutoUpdateType::DontAutoUpdate)));
}
