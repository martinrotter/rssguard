// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/standardfeednetworkdetails.h"

#include "src/definitions.h"

#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/networkexception.h>
#include <librssguard/exceptions/scriptexception.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/services/abstract/category.h>

#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QtGlobal>

StandardFeedNetworkDetails::StandardFeedNetworkDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_helpHttpHeaders
    ->setHelpText(tr("Enter each key/value HTTP header pair on separate line. Note that all spaces are significant "
                     "and that header names are case-sensitive. "
                     "Also, make sure to separate key from value with '=', like the example below:") +
                    QSL("<br/><br/><b>HeaderKey=HeaderValue</b>"),
                  false);

  m_ui.m_wdgNetworkProxy->setup(true, true);

  m_ui.m_cmbEnableHttp2->addItem(tr("Use application settings"),
                                 QVariant::fromValue(int(NetworkFactory::Http2Status::DontSet)));
  m_ui.m_cmbEnableHttp2->addItem(tr("Enabled"), QVariant::fromValue(int(NetworkFactory::Http2Status::Enabled)));
  m_ui.m_cmbEnableHttp2->addItem(tr("Disabled"), QVariant::fromValue(int(NetworkFactory::Http2Status::Disabled)));
}

void StandardFeedNetworkDetails::loadHttpHeaders(const QVariantHash& headers) {
  m_ui.m_txtHttpHeaders->clear();

  for (auto i = headers.cbegin(), end = headers.cend(); i != end; i++) {
    m_ui.m_txtHttpHeaders->appendPlainText(QSL("%1=%2").arg(i.key(), i.value().toString()));
  }
}

void StandardFeedNetworkDetails::setHttp2Status(NetworkFactory::Http2Status status) {
  m_ui.m_cmbEnableHttp2->setCurrentIndex(m_ui.m_cmbEnableHttp2->findData(int(status)));
}

NetworkFactory::Http2Status StandardFeedNetworkDetails::http2Status() const {
  return static_cast<NetworkFactory::Http2Status>(m_ui.m_cmbEnableHttp2->currentData().toInt());
}

QVariantHash StandardFeedNetworkDetails::httpHeaders() const {
  QVariantHash h;

  QRegularExpression exp(QSL("^([^=]+)=(.+)$"), QRegularExpression::PatternOption::MultilineOption);
  auto exp_match = exp.globalMatch(m_ui.m_txtHttpHeaders->toPlainText());

  while (exp_match.hasNext()) {
    auto match = exp_match.next();

    h.insert(match.captured(1).trimmed(), match.captured(2).trimmed());
  }

  return h;
}
