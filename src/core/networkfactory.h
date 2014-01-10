#ifndef NETWORKFACTORY_H
#define NETWORKFACTORY_H

#include <QNetworkReply>


class NetworkFactory {
  private:
    // Constructor.
    explicit NetworkFactory();

  public:
    // Performs SYNCHRONOUS download of file with given URL
    // and given timeout.
    static QNetworkReply::NetworkError downloadFile(const QString &url,
                                                    int timeout,
                                                    QByteArray &output);
};

#endif // NETWORKFACTORY_H
