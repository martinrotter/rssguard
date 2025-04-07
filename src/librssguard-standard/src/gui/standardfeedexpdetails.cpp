// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/standardfeedexpdetails.h"

#include "src/definitions.h"

#include <librssguard/3rd-party/boolinq/boolinq.h>
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
#include <QTextCodec>
#include <QtGlobal>

StandardFeedExpDetails::StandardFeedExpDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  m_ui.m_helpDontUseRawXml->setHelpText(tr("Turning this setting ON might bring considerable performance boost when "
                                           "fetching this feed, but only in some very specific conditions.\n\n"
                                           "This setting is useful when raw XML parsing of the feed is very slow, this "
                                           "happens for feed which do have very long contents."),
                                        false);

  m_ui.m_cmbEnableHttp2->addItem(tr("Use application settings"),
                                 QVariant::fromValue(int(NetworkFactory::Http2Status::DontSet)));
  m_ui.m_cmbEnableHttp2->addItem(tr("Enabled"), QVariant::fromValue(int(NetworkFactory::Http2Status::Enabled)));
  m_ui.m_cmbEnableHttp2->addItem(tr("Disabled"), QVariant::fromValue(int(NetworkFactory::Http2Status::Disabled)));
}

void StandardFeedExpDetails::setHttp2Status(NetworkFactory::Http2Status status) {
  m_ui.m_cmbEnableHttp2->setCurrentIndex(m_ui.m_cmbEnableHttp2->findData(int(status)));
}

NetworkFactory::Http2Status StandardFeedExpDetails::http2Status() const {
  return static_cast<NetworkFactory::Http2Status>(m_ui.m_cmbEnableHttp2->currentData().toInt());
}
