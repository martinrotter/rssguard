// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsbrowsermail.h"

#include "gui/guiutilities.h"
#include "gui/networkproxydetails.h"
#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QNetworkProxy>

SettingsBrowserMail::SettingsBrowserMail(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_proxyDetails(new NetworkProxyDetails(this)), m_ui(new Ui::SettingsBrowserMail) {
  m_ui->setupUi(this);

  m_ui->m_tabBrowserProxy->addTab(m_proxyDetails, tr("Network proxy"));

  GuiUtilities::setLabelAsNotice(*m_ui->label, false);
  GuiUtilities::setLabelAsNotice(*m_ui->m_lblExternalEmailInfo, false);
  GuiUtilities::setLabelAsNotice(*m_ui->m_lblToolInfo, false);

#if defined(USE_WEBENGINE)
  m_ui->m_checkOpenLinksInExternal->setVisible(false);
#else
  connect(m_ui->m_checkOpenLinksInExternal, &QCheckBox::stateChanged, this, &SettingsBrowserMail::dirtifySettings);
#endif

  m_ui->m_listTools->setHeaderLabels(QStringList() << tr("Executable") << tr("Parameters"));
  m_ui->m_listTools->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

  connect(m_proxyDetails, &NetworkProxyDetails::changed, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_grpCustomExternalBrowser, &QGroupBox::toggled, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_grpCustomExternalEmail, &QGroupBox::toggled, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_txtExternalBrowserArguments, &QLineEdit::textChanged, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_txtExternalBrowserExecutable, &QLineEdit::textChanged, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_txtExternalEmailArguments, &QLineEdit::textChanged, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_txtExternalEmailExecutable, &QLineEdit::textChanged, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_cmbExternalBrowserPreset, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsBrowserMail::changeDefaultBrowserArguments);
  connect(m_ui->m_btnExternalBrowserExecutable, &QPushButton::clicked, this, &SettingsBrowserMail::selectBrowserExecutable);
  connect(m_ui->m_cmbExternalEmailPreset, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &SettingsBrowserMail::changeDefaultEmailArguments);
  connect(m_ui->m_btnExternalEmailExecutable, &QPushButton::clicked, this, &SettingsBrowserMail::selectEmailExecutable);
  connect(m_ui->m_btnAddTool, &QPushButton::clicked, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_btnDeleteTool, &QPushButton::clicked, this, &SettingsBrowserMail::dirtifySettings);
  connect(m_ui->m_btnAddTool, &QPushButton::clicked, this, &SettingsBrowserMail::addExternalTool);
  connect(m_ui->m_btnDeleteTool, &QPushButton::clicked, this, &SettingsBrowserMail::deleteSelectedExternalTool);
  connect(m_ui->m_listTools, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current, QTreeWidgetItem* previous) {
    Q_UNUSED(previous)

    m_ui->m_btnDeleteTool->setEnabled(current != nullptr);
  });
}

SettingsBrowserMail::~SettingsBrowserMail() {
  delete m_ui;
}

void SettingsBrowserMail::changeDefaultBrowserArguments(int index) {
  if (index != 0) {
    m_ui->m_txtExternalBrowserArguments->setText(m_ui->m_cmbExternalBrowserPreset->itemData(index).toString());
  }
}

void SettingsBrowserMail::selectBrowserExecutable() {
  const QString executable_file = QFileDialog::getOpenFileName(this,
                                                               tr("Select web browser executable"),
                                                               qApp->homeFolder(),

                                                               //: File filter for external browser selection dialog.
#if defined(Q_OS_LINUX)
                                                               tr("Executables (*)"));
#else
                                                               tr("Executables (*.*)"));
#endif

  if (!executable_file.isEmpty()) {
    m_ui->m_txtExternalBrowserExecutable->setText(QDir::toNativeSeparators(executable_file));
  }
}

QList<ExternalTool> SettingsBrowserMail::externalTools() const {
  QList<ExternalTool> list;

  for (int i = 0; i < m_ui->m_listTools->topLevelItemCount(); i++) {
    list.append(m_ui->m_listTools->topLevelItem(i)->data(0, Qt::UserRole).value<ExternalTool>());
  }

  return list;
}

void SettingsBrowserMail::setExternalTools(const QList<ExternalTool>& list) {
  for (const ExternalTool& tool : list) {
    QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->m_listTools,
                                                QStringList() << tool.executable() << tool.parameters().join(QL1C(' ')));

    item->setData(0, Qt::UserRole, QVariant::fromValue(tool));

    m_ui->m_listTools->addTopLevelItem(item);
  }
}

void SettingsBrowserMail::changeDefaultEmailArguments(int index) {
  if (index != 0) {
    m_ui->m_txtExternalEmailArguments->setText(m_ui->m_cmbExternalEmailPreset->itemData(index).toString());
  }
}

void SettingsBrowserMail::selectEmailExecutable() {
  QString executable_file = QFileDialog::getOpenFileName(this,
                                                         tr("Select e-mail executable"),
                                                         qApp->homeFolder(),

                                                         //: File filter for external e-mail selection dialog.
#if defined(Q_OS_LINUX)
                                                         tr("Executables (*)"));
#else
                                                         tr("Executables (*.*)"));
#endif

  if (!executable_file.isEmpty()) {
    m_ui->m_txtExternalEmailExecutable->setText(QDir::toNativeSeparators(executable_file));
  }
}

void SettingsBrowserMail::loadSettings() {
  onBeginLoadSettings();

#if !defined(USE_WEBENGINE)
  m_ui->m_checkOpenLinksInExternal->setChecked(settings()->value(GROUP(Browser),
                                                                 SETTING(Browser::OpenLinksInExternalBrowserRightAway)).toBool());
#endif

  // Load settings of web browser GUI.
  m_ui->m_cmbExternalBrowserPreset->addItem(tr("Opera 12 or older"), QSL("-nosession %1"));
  m_ui->m_txtExternalBrowserExecutable->setText(settings()->value(GROUP(Browser),
                                                                  SETTING(Browser::CustomExternalBrowserExecutable)).toString());
  m_ui->m_txtExternalBrowserArguments->setText(settings()->value(GROUP(Browser),
                                                                 SETTING(Browser::CustomExternalBrowserArguments)).toString());
  m_ui->m_grpCustomExternalBrowser->setChecked(settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalBrowserEnabled)).toBool());

  // Load settings of e-mail.
  m_ui->m_cmbExternalEmailPreset->addItem(tr("Mozilla Thunderbird"), QSL("-compose \"subject='%1',body='%2'\""));
  m_ui->m_txtExternalEmailExecutable->setText(settings()->value(GROUP(Browser),
                                                                SETTING(Browser::CustomExternalEmailExecutable)).toString());
  m_ui->m_txtExternalEmailArguments->setText(settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailArguments)).toString());
  m_ui->m_grpCustomExternalEmail->setChecked(settings()->value(GROUP(Browser), SETTING(Browser::CustomExternalEmailEnabled)).toBool());

  // Load the settings.
  QNetworkProxy::ProxyType selected_proxy_type = static_cast<QNetworkProxy::ProxyType>(settings()->value(GROUP(Proxy),
                                                                                                         SETTING(Proxy::Type)).toInt());

  m_proxyDetails->setProxy(QNetworkProxy(selected_proxy_type,
                                         settings()->value(GROUP(Proxy), SETTING(Proxy::Host)).toString(),
                                         settings()->value(GROUP(Proxy), SETTING(Proxy::Port)).toInt(),
                                         settings()->value(GROUP(Proxy), SETTING(Proxy::Username)).toString(),
                                         settings()->password(GROUP(Proxy), SETTING(Proxy::Password)).toString()));

  setExternalTools(ExternalTool::toolsFromSettings());
  onEndLoadSettings();
}

void SettingsBrowserMail::saveSettings() {
  onBeginSaveSettings();

#if !defined(USE_WEBENGINE)
  settings()->setValue(GROUP(Browser), Browser::OpenLinksInExternalBrowserRightAway, m_ui->m_checkOpenLinksInExternal->isChecked());
#endif

  // Save settings of GUI of web browser.
  settings()->setValue(GROUP(Browser), Browser::CustomExternalBrowserEnabled, m_ui->m_grpCustomExternalBrowser->isChecked());
  settings()->setValue(GROUP(Browser), Browser::CustomExternalBrowserExecutable, m_ui->m_txtExternalBrowserExecutable->text());
  settings()->setValue(GROUP(Browser), Browser::CustomExternalBrowserArguments, m_ui->m_txtExternalBrowserArguments->text());

  // Save settings of e-mail.
  settings()->setValue(GROUP(Browser), Browser::CustomExternalEmailExecutable, m_ui->m_txtExternalEmailExecutable->text());
  settings()->setValue(GROUP(Browser), Browser::CustomExternalEmailArguments, m_ui->m_txtExternalEmailArguments->text());
  settings()->setValue(GROUP(Browser), Browser::CustomExternalEmailEnabled, m_ui->m_grpCustomExternalEmail->isChecked());

  auto proxy = m_proxyDetails->proxy();

  settings()->setValue(GROUP(Proxy), Proxy::Type, int(proxy.type()));
  settings()->setValue(GROUP(Proxy), Proxy::Host, proxy.hostName());
  settings()->setValue(GROUP(Proxy), Proxy::Username, proxy.user());
  settings()->setPassword(GROUP(Proxy), Proxy::Password, proxy.password());
  settings()->setValue(GROUP(Proxy), Proxy::Port, proxy.port());

  auto tools = externalTools();

  ExternalTool::setToolsToSettings(tools);

  qApp->web()->updateProxy();

  // Reload settings for all network access managers.
  qApp->downloadManager()->networkManager()->loadSettings();

  onEndSaveSettings();
}

void SettingsBrowserMail::addExternalTool() {
  QString executable_file = QFileDialog::getOpenFileName(this,
                                                         tr("Select external tool"),
                                                         qApp->homeFolder(),

                                                         //: File filter for external tool selection dialog.
#if defined(Q_OS_LINUX)
                                                         tr("Executables (*)"));
#else
                                                         tr("Executables (*.*)"));
#endif

  if (!executable_file.isEmpty()) {
    executable_file = QDir::toNativeSeparators(executable_file);
    bool ok;
    QString parameters = QInputDialog::getText(this,
                                               tr("Enter parameters"),
                                               tr(
                                                 "Enter (optional) parameters separated by single space to send to executable when opening URLs."),
                                               QLineEdit::EchoMode::Normal,
                                               QString(),
                                               &ok);

    if (ok) {
      QTreeWidgetItem* item = new QTreeWidgetItem(m_ui->m_listTools,
                                                  QStringList() << QDir::toNativeSeparators(executable_file) << parameters);

      item->setData(0, Qt::UserRole, QVariant::fromValue(ExternalTool(executable_file, parameters.split(QSL(" ")))));
      m_ui->m_listTools->addTopLevelItem(item);
    }
  }
}

void SettingsBrowserMail::deleteSelectedExternalTool() {
  if (!m_ui->m_listTools->selectedItems().isEmpty()) {
    m_ui->m_listTools->takeTopLevelItem(m_ui->m_listTools->indexOfTopLevelItem(m_ui->m_listTools->selectedItems().first()));
  }
}
