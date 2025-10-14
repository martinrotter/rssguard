// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/networkproxydetails.h"

#include "definitions/definitions.h"

#include "ui_networkproxydetails.h"

#include <QNetworkProxy>

#define USE_ACC_PROXY_IDX 666

NetworkProxyDetails::NetworkProxyDetails(QWidget* parent) : QWidget(parent), m_ui(new Ui::NetworkProxyDetails()) {
  m_ui->setupUi(this);
}

void NetworkProxyDetails::setup(bool feed_specific_setting, bool account_wide_setting) {
  m_ui->m_lblProxyInfo->setHelpText(tr("Note that these settings are applied only on newly established connections."),
                                    false);
  m_ui->m_txtProxyPassword->setPasswordMode(true);

  connect(m_ui->m_cmbProxyType,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &NetworkProxyDetails::onProxyTypeChanged);

  m_ui->m_cmbProxyType->addItem(tr("No proxy"), QNetworkProxy::ProxyType::NoProxy);

  if (feed_specific_setting) {
    m_ui->m_cmbProxyType->addItem(tr("Account proxy"), USE_ACC_PROXY_IDX);
  }

  m_ui->m_cmbProxyType->addItem(account_wide_setting ? tr("Use global app setting") : tr("System proxy"),
                                QNetworkProxy::ProxyType::DefaultProxy);
  m_ui->m_cmbProxyType->addItem(QSL("Socks5"), QNetworkProxy::ProxyType::Socks5Proxy);
  m_ui->m_cmbProxyType->addItem(QSL("Http"), QNetworkProxy::ProxyType::HttpProxy);

  connect(m_ui->m_cmbProxyType,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &NetworkProxyDetails::changed);
  connect(m_ui->m_txtProxyHost, &QLineEdit::textChanged, this, &NetworkProxyDetails::changed);
  connect(m_ui->m_txtProxyPassword, &QLineEdit::textChanged, this, &NetworkProxyDetails::changed);
  connect(m_ui->m_txtProxyUsername, &QLineEdit::textChanged, this, &NetworkProxyDetails::changed);
  connect(m_ui->m_spinProxyPort, QOverload<int>::of(&QSpinBox::valueChanged), this, &NetworkProxyDetails::changed);
}

NetworkProxyDetails::~NetworkProxyDetails() = default;

QNetworkProxy NetworkProxyDetails::proxy() const {
  QNetworkProxy proxy(useAccountProxy()
                        ? QNetworkProxy::ProxyType::DefaultProxy
                        : static_cast<QNetworkProxy::ProxyType>(m_ui->m_cmbProxyType->currentData().toInt()),
                      m_ui->m_txtProxyHost->text(),
                      m_ui->m_spinProxyPort->value(),
                      m_ui->m_txtProxyUsername->text(),
                      m_ui->m_txtProxyPassword->text());

  return proxy;
}

void NetworkProxyDetails::setProxy(const QNetworkProxy& proxy, bool use_account_proxy) {
  m_ui->m_cmbProxyType->setCurrentIndex(m_ui->m_cmbProxyType->findData(use_account_proxy ? USE_ACC_PROXY_IDX
                                                                                         : proxy.type()));
  m_ui->m_txtProxyHost->setText(proxy.hostName());
  m_ui->m_spinProxyPort->setValue(proxy.port());
  m_ui->m_txtProxyUsername->setText(proxy.user());
  m_ui->m_txtProxyPassword->setText(proxy.password());
}

bool NetworkProxyDetails::useAccountProxy() const {
  return m_ui->m_cmbProxyType->currentData().toInt() == USE_ACC_PROXY_IDX;
}

void NetworkProxyDetails::onProxyTypeChanged(int index) {
  int dat_num = m_ui->m_cmbProxyType->itemData(index).toInt();
  const QNetworkProxy::ProxyType selected_type = static_cast<QNetworkProxy::ProxyType>(dat_num);
  const bool is_proxy_selected = selected_type != QNetworkProxy::ProxyType::NoProxy &&
                                 selected_type != QNetworkProxy::ProxyType::DefaultProxy &&
                                 dat_num != USE_ACC_PROXY_IDX;

  m_ui->m_proxyDetails->setEnabled(is_proxy_selected);
}
