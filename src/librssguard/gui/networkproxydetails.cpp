// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/networkproxydetails.h"

#include "gui/guiutilities.h"

#include <QNetworkProxy>

NetworkProxyDetails::NetworkProxyDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);
  GuiUtilities::setLabelAsNotice(*m_ui.m_lblProxyInfo, false);

  connect(m_ui.m_cmbProxyType, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          &NetworkProxyDetails::onProxyTypeChanged);
  connect(m_ui.m_checkShowPassword, &QCheckBox::stateChanged, this, &NetworkProxyDetails::displayProxyPassword);

  m_ui.m_cmbProxyType->addItem(tr("No proxy"), QNetworkProxy::ProxyType::NoProxy);
  m_ui.m_cmbProxyType->addItem(tr("System proxy"), QNetworkProxy::ProxyType::DefaultProxy);
  m_ui.m_cmbProxyType->addItem(tr("Socks5"), QNetworkProxy::ProxyType::Socks5Proxy);
  m_ui.m_cmbProxyType->addItem(tr("Http"), QNetworkProxy::ProxyType::HttpProxy);

  displayProxyPassword(Qt::CheckState::Unchecked);
}

void NetworkProxyDetails::displayProxyPassword(int state) {
  if (state == Qt::CheckState::Checked) {
    m_ui.m_txtProxyPassword->setEchoMode(QLineEdit::EchoMode::Normal);
  }
  else {
    m_ui.m_txtProxyPassword->setEchoMode(QLineEdit::EchoMode::PasswordEchoOnEdit);
  }
}

void NetworkProxyDetails::onProxyTypeChanged(int index) {
  const QNetworkProxy::ProxyType selected_type = static_cast<QNetworkProxy::ProxyType>(m_ui.m_cmbProxyType->itemData(index).toInt());
  const bool is_proxy_selected = selected_type != QNetworkProxy::NoProxy && selected_type != QNetworkProxy::DefaultProxy;

  m_ui.m_proxyDetails->setEnabled(is_proxy_selected);
}
