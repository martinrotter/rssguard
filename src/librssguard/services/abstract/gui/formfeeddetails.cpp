// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formfeeddetails.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "gui/baselineedit.h"
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
  : QDialog(parent),
  m_editableFeed(nullptr),
  m_serviceRoot(service_root) {
  initialize();
  createConnections();

  // Initialize that shit.
  onUsernameChanged(QString());
  onPasswordChanged(QString());
}

int FormFeedDetails::editBaseFeed(Feed* input_feed) {
  setEditableFeed(input_feed);

  // Run the dialog.
  return QDialog::exec();
}

void FormFeedDetails::activateTab(int index) {
  m_ui->m_tabWidget->setCurrentIndex(index);
}

void FormFeedDetails::insertCustomTab(QWidget* custom_tab, const QString& title, int index) {
  m_ui->m_tabWidget->insertTab(index, custom_tab, title);
}

void FormFeedDetails::apply() {
  Feed new_feed;

  // Setup data for new_feed.
  new_feed.setPasswordProtected(m_ui->m_gbAuthentication->isChecked());
  new_feed.setUsername(m_ui->m_txtUsername->lineEdit()->text());
  new_feed.setPassword(m_ui->m_txtPassword->lineEdit()->text());
  new_feed.setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(
                                                                 m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
  new_feed.setAutoUpdateInitialInterval(int(m_ui->m_spinAutoUpdateInterval->value()));

  if (m_editableFeed != nullptr) {
    // NOTE: Co s tim?
    //new_feed->setParent(m_editableFeed->parent());

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

void FormFeedDetails::onUsernameChanged(const QString& new_username) {
  bool is_username_ok = !m_ui->m_gbAuthentication->isChecked() || !new_username.simplified().isEmpty();

  m_ui->m_txtUsername->setStatus(is_username_ok ?
                                 LineEditWithStatus::StatusType::Ok :
                                 LineEditWithStatus::StatusType::Warning,
                                 is_username_ok ?
                                 tr("Username is ok or it is not needed.") :
                                 tr("Username is empty."));
}

void FormFeedDetails::onPasswordChanged(const QString& new_password) {
  bool is_password_ok = !m_ui->m_gbAuthentication->isChecked() || !new_password.simplified().isEmpty();

  m_ui->m_txtPassword->setStatus(is_password_ok ?
                                 LineEditWithStatus::StatusType::Ok :
                                 LineEditWithStatus::StatusType::Warning,
                                 is_password_ok ?
                                 tr("Password is ok or it is not needed.") :
                                 tr("Password is empty."));
}

void FormFeedDetails::onAuthenticationSwitched() {
  onUsernameChanged(m_ui->m_txtUsername->lineEdit()->text());
  onPasswordChanged(m_ui->m_txtPassword->lineEdit()->text());
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
  connect(m_ui->m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormFeedDetails::onUsernameChanged);
  connect(m_ui->m_txtPassword->lineEdit(), &BaseLineEdit::textChanged, this, &FormFeedDetails::onPasswordChanged);
  connect(m_ui->m_gbAuthentication, &QGroupBox::toggled, this, &FormFeedDetails::onAuthenticationSwitched);
  connect(m_ui->m_cmbAutoUpdateType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &FormFeedDetails::onAutoUpdateTypeChanged);
}

void FormFeedDetails::setEditableFeed(Feed* editable_feed) {
  setWindowTitle(tr("Edit feed '%1'").arg(editable_feed->title()));

  m_editableFeed = editable_feed;

  m_ui->m_gbAuthentication->setChecked(editable_feed->passwordProtected());
  m_ui->m_txtUsername->lineEdit()->setText(editable_feed->username());
  m_ui->m_txtPassword->lineEdit()->setText(editable_feed->password());

  m_ui->m_cmbAutoUpdateType->setCurrentIndex(m_ui->m_cmbAutoUpdateType->findData(QVariant::fromValue((int) editable_feed->autoUpdateType())));
  m_ui->m_spinAutoUpdateInterval->setValue(editable_feed->autoUpdateInitialInterval());
}

void FormFeedDetails::initialize() {
  m_ui.reset(new Ui::FormFeedDetails());
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("application-rss+xml")));

  // Set text boxes.
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username"));
  m_ui->m_txtUsername->lineEdit()->setToolTip(tr("Set username to access the feed."));
  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password"));
  m_ui->m_txtPassword->lineEdit()->setToolTip(tr("Set password to access the feed."));

  // Setup auto-update options.
  m_ui->m_spinAutoUpdateInterval->setValue(DEFAULT_AUTO_UPDATE_INTERVAL);
  m_ui->m_cmbAutoUpdateType->addItem(tr("Auto-update using global interval"),
                                     QVariant::fromValue(int(Feed::AutoUpdateType::DefaultAutoUpdate)));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Auto-update every"),
                                     QVariant::fromValue(int(Feed::AutoUpdateType::SpecificAutoUpdate)));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Do not auto-update at all"),
                                     QVariant::fromValue(int(Feed::AutoUpdateType::DontAutoUpdate)));
}
