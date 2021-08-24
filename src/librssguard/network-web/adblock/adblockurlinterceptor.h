// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ADBLOCKURLINTERCEPTOR_H
#define ADBLOCKURLINTERCEPTOR_H

#include "network-web/urlinterceptor.h"

class AdBlockManager;

class AdBlockUrlInterceptor : public UrlInterceptor {
  Q_OBJECT

  public:
    explicit AdBlockUrlInterceptor(AdBlockManager* manager);

    void interceptRequest(QWebEngineUrlRequestInfo& info);

  private:
    AdBlockManager* m_manager;
};

#endif // ADBLOCKURLINTERCEPTOR_H
