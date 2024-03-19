// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockurlinterceptor.h"

#include "definitions/definitions.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrequestinfo.h"

AdBlockUrlInterceptor::AdBlockUrlInterceptor(AdBlockManager* manager) : UrlInterceptor(manager), m_manager(manager) {}

void AdBlockUrlInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info) {
  if (m_manager->block(AdblockRequestInfo(info)).m_blocked) {
    info.block(true);

    qWarningNN << LOGSEC_ADBLOCK << "Blocked request:" << QUOTE_W_SPACE_DOT(info.requestUrl().toString());
  }
}
