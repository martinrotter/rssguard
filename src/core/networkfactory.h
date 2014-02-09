#ifndef NETWORKFACTORY_H
#define NETWORKFACTORY_H

#include <QNetworkReply>


class FeedsModelStandardFeed;

class NetworkFactory {
  private:
    // Constructor.
    explicit NetworkFactory();

  public:
    // Performs SYNCHRONOUS download if favicon for the site,
    // given URL belongs to.
    static QNetworkReply::NetworkError downloadIcon(const QString &url,
                                                    int timeout,
                                                    QIcon &output);

    // Performs SYNCHRONOUS download of file with given URL
    // and given timeout.
    static QNetworkReply::NetworkError downloadFeedFile(const QString &url,
                                                        int timeout,
                                                        QByteArray &output,
                                                        bool protected_contents = false,
                                                        const QString &username = QString(),
                                                        const QString &password = QString());
};

#endif // NETWORKFACTORY_H
