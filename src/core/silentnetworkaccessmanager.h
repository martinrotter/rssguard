#ifndef SILENTNETWORKACCESSMANAGES_H
#define SILENTNETWORKACCESSMANAGES_H

#include "core/basenetworkaccessmanager.h"


// Network manager used for more communication for feeds.
class SilentNetworkAccessManager : public BaseNetworkAccessManager {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit SilentNetworkAccessManager(QObject *parent = 0);
    virtual ~SilentNetworkAccessManager();
};

#endif // SILENTNETWORKACCESSMANAGES_H
