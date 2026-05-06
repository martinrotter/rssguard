// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsnetwork.h"

#include "gui/dialogs/filedialog.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/reusable/networkproxydetails.h"
#include "gui/webbrowser.h"
#include "gui/webviewers/webviewer.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"

#include <QInputDialog>
#include <QNetworkProxy>

SettingsNetwork::SettingsNetwork(Settings* settings, QWidget* parent)
  : SettingsPanel(settings, parent), m_ui(nullptr) {}

void SettingsNetwork::loadUi() {
  m_ui = new Ui::SettingsNetwork();
  m_proxyDetails = new NetworkProxyDetails(this);

  m_ui->setupUi(this);

  m_proxyDetails->setup(false, false);

  m_ui->m_tabBrowserProxy->insertTab(2, m_proxyDetails, tr("Network proxy"));

  connect(m_ui->m_cbFollowHyperlinks, &QCheckBox::STATE_CHANGED, this, &SettingsNetwork::dirtifySettings);
  connect(m_ui->m_cbEnableHttp2, &QCheckBox::STATE_CHANGED, this, &SettingsNetwork::dirtifySettings);
  connect(m_proxyDetails, &NetworkProxyDetails::changed, this, &SettingsNetwork::dirtifySettings);

  connect(m_ui->m_txtUserAgent, &QLineEdit::textChanged, this, &SettingsNetwork::dirtifySettings);
  connect(m_ui->m_txtUserAgent, &QLineEdit::textChanged, this, &SettingsNetwork::requireRestart);

#if !defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  m_ui->m_tabWebBackends->removeTab(0);
#else
  connect(m_ui->m_txtWebEngineFlags, &QLineEdit::textChanged, this, &SettingsNetwork::dirtifySettings);
  connect(m_ui->m_txtWebEngineFlags, &QLineEdit::textChanged, this, &SettingsNetwork::requireRestart);
#endif

  SettingsPanel::loadUi();
}

SettingsNetwork::~SettingsNetwork() {
  if (m_ui != nullptr) {
    delete m_ui;
  }
}

QIcon SettingsNetwork::icon() const {
  return qApp->icons()->fromTheme(QSL("applications-network"), QSL("internet-services"));
}

void SettingsNetwork::loadSettings() {
  onBeginLoadSettings();

  m_ui->m_cbFollowHyperlinks->setChecked(settings()->value(GROUP(Web), SETTING(Web::FollowLinks)).toBool());
  m_ui->m_cbEnableHttp2->setChecked(settings()->value(GROUP(Network), SETTING(Network::EnableHttp2)).toBool());
  m_ui->m_txtUserAgent->setText(settings()->value(GROUP(Network), SETTING(Network::CustomUserAgent)).toString());

  // Load the settings.
  QNetworkProxy::ProxyType selected_proxy_type =
    static_cast<QNetworkProxy::ProxyType>(settings()->value(GROUP(Proxy), SETTING(Proxy::Type)).toInt());

  m_proxyDetails->setProxy(QNetworkProxy(selected_proxy_type,
                                         settings()->value(GROUP(Proxy), SETTING(Proxy::Host)).toString(),
                                         settings()->value(GROUP(Proxy), SETTING(Proxy::Port)).toInt(),
                                         settings()->value(GROUP(Proxy), SETTING(Proxy::Username)).toString(),
                                         settings()->password(GROUP(Proxy), SETTING(Proxy::Password)).toString()));

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  m_ui->m_txtWebEngineFlags->setText(settings()->value(GROUP(Web), SETTING(Web::WebEngineChromiumFlags)).toString());
#endif

  onEndLoadSettings();
}

void SettingsNetwork::saveSettings() {
  onBeginSaveSettings();

  settings()->setValue(GROUP(Web), Web::FollowLinks, m_ui->m_cbFollowHyperlinks->isChecked());
  settings()->setValue(GROUP(Network), Network::EnableHttp2, m_ui->m_cbEnableHttp2->isChecked());
  settings()->setValue(GROUP(Network), Network::CustomUserAgent, m_ui->m_txtUserAgent->text());

  auto proxy = m_proxyDetails->proxy();

  settings()->setValue(GROUP(Proxy), Proxy::Type, int(proxy.type()));
  settings()->setValue(GROUP(Proxy), Proxy::Host, proxy.hostName());
  settings()->setValue(GROUP(Proxy), Proxy::Username, proxy.user());
  settings()->setPassword(GROUP(Proxy), Proxy::Password, proxy.password());
  settings()->setValue(GROUP(Proxy), Proxy::Port, proxy.port());

  qApp->web()->updateProxy();
  qApp->mainForm()->tabWidget()->feedMessageViewer()->webBrowser()->viewer()->reloadNetworkSettings();

#if defined(WEB_ARTICLE_VIEWER_WEBENGINE)
  settings()->setValue(GROUP(Web), Web::WebEngineChromiumFlags, m_ui->m_txtWebEngineFlags->text());
#endif

  onEndSaveSettings();
}
