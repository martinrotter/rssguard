#ifndef WEBBROWSERNETWORKACCESSMANAGER_H
#define WEBBROWSERNETWORKACCESSMANAGER_H

#include "core/basenetworkaccessmanager.h"


class WebBrowserNetworkAccessManager : public BaseNetworkAccessManager {
    Q_OBJECT
  public:
    explicit WebBrowserNetworkAccessManager(QObject *parent = 0);
    virtual ~WebBrowserNetworkAccessManager();
    
};

#endif // WEBBROWSERNETWORKACCESSMANAGER_H
