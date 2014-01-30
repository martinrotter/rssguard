#ifndef NETWORKFACTORY_H
#define NETWORKFACTORY_H

#include <QNetworkReply>


class FeedsModelStandardFeed;

class NetworkFactory {
  private:
    // Constructor.
    explicit NetworkFactory();

  public:
    // Performs SYNCHRONOUS download of file with given URL
    // and given timeout.
    static QNetworkReply::NetworkError downloadFeedFile(const QString &url,
                                                        int timeout,
                                                        QByteArray &output,
                                                        FeedsModelStandardFeed *feed);
};

#endif // NETWORKFACTORY_H
