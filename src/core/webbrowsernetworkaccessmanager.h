#ifndef WEBBROWSERNETWORKACCESSMANAGER_H
#define WEBBROWSERNETWORKACCESSMANAGER_H

#include "core/basenetworkaccessmanager.h"


// This is network access manager for web browsers.
class WebBrowserNetworkAccessManager : public BaseNetworkAccessManager {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit WebBrowserNetworkAccessManager(QObject *parent = 0);
    virtual ~WebBrowserNetworkAccessManager();
};

#endif // WEBBROWSERNETWORKACCESSMANAGER_H
