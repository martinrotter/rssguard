// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/networkproxydetails.h"

#include "definitions/definitions.h"

#include "ui_networkproxydetails.h"

#include <QNetworkProxy>

NetworkProxyDetails::NetworkProxyDetails(bool account_wide_setting, QWidget* parent)
  : QWidget(parent), m_ui(new Ui::NetworkProxyDetails()) {
  m_ui->setupUi(this);

  m_ui->m_lblProxyInfo->setHelpText(tr("Note that these settings are applied only on newly established connections."),
                                    false);
  m_ui->m_txtProxyPassword->setPasswordMode(true);

  connect(m_ui->m_cmbProxyType,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &NetworkProxyDetails::onProxyTypeChanged);

  m_ui->m_cmbProxyType->addItem(tr("No proxy"), QNetworkProxy::ProxyType::NoProxy);
  m_ui->m_cmbProxyType->addItem(account_wide_setting ? tr("Application-wide proxy") : tr("System proxy"),
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
  QNetworkProxy proxy(static_cast<QNetworkProxy::ProxyType>(m_ui->m_cmbProxyType->currentData().toInt()),
                      m_ui->m_txtProxyHost->text(),
                      m_ui->m_spinProxyPort->value(),
                      m_ui->m_txtProxyUsername->text(),
                      m_ui->m_txtProxyPassword->text());

  return proxy;
}

void NetworkProxyDetails::setProxy(const QNetworkProxy& proxy) {
  m_ui->m_cmbProxyType->setCurrentIndex(m_ui->m_cmbProxyType->findData(proxy.type()));
  m_ui->m_txtProxyHost->setText(proxy.hostName());
  m_ui->m_spinProxyPort->setValue(proxy.port());
  m_ui->m_txtProxyUsername->setText(proxy.user());
  m_ui->m_txtProxyPassword->setText(proxy.password());
}

void NetworkProxyDetails::onProxyTypeChanged(int index) {
  const QNetworkProxy::ProxyType selected_type =
    static_cast<QNetworkProxy::ProxyType>(m_ui->m_cmbProxyType->itemData(index).toInt());
  const bool is_proxy_selected =
    selected_type != QNetworkProxy::ProxyType::NoProxy && selected_type != QNetworkProxy::ProxyType::DefaultProxy;

  m_ui->m_proxyDetails->setEnabled(is_proxy_selected);
}
