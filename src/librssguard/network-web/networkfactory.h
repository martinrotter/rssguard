// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NETWORKFACTORY_H
#define NETWORKFACTORY_H

#include "definitions/typedefs.h"
#include "network-web/httpresponse.h"
#include "services/abstract/feed.h"

#include <QCoreApplication>
#include <QHttpPart>
#include <QNetworkCookie>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPair>
#include <QVariant>

struct RSSGUARD_DLLSPEC NetworkResult {
    QNetworkReply::NetworkError m_networkError;
    int m_httpCode;
    QString m_contentType;
    QList<QNetworkCookie> m_cookies;
    QMap<QString, QString> m_headers;
    QUrl m_url;

    explicit NetworkResult();
    explicit NetworkResult(QNetworkReply::NetworkError err,
                           int http_code,
                           const QString& ct,
                           const QList<QNetworkCookie>& cook);
};

class Downloader;

class RSSGUARD_DLLSPEC NetworkFactory {
    Q_DECLARE_TR_FUNCTIONS(NetworkFactory)

  private:
    explicit NetworkFactory() = default;

  public:
    enum class NetworkAuthentication {
      NoAuthentication = 0,
      Basic = 1,
      Token = 2
    };

    static QStringList extractFeedLinksFromHtmlPage(const QUrl& url, const QString& html);
    static QPair<QByteArray, QByteArray> generateBasicAuthHeader(NetworkAuthentication protection,
                                                                 const QString& username,
                                                                 const QString& password);

    // Returns human readable text for given network error.
    static QString networkErrorText(QNetworkReply::NetworkError error_code);
    static QString sanitizeUrl(const QString& url);

    // Performs SYNCHRONOUS favicon download for the site,
    // given URL belongs to.
    static QNetworkReply::NetworkError downloadIcon(const QList<IconLocation>& urls,
                                                    int timeout,
                                                    QPixmap& output,
                                                    const QList<QPair<QByteArray, QByteArray>>& additional_headers,
                                                    const QNetworkProxy& custom_proxy =
                                                      QNetworkProxy::ProxyType::DefaultProxy);
    static NetworkResult performNetworkOperation(const QString& url,
                                                 int timeout,
                                                 const QByteArray& input_data,
                                                 QByteArray& output,
                                                 QNetworkAccessManager::Operation operation,
                                                 const QList<QPair<QByteArray, QByteArray>>& additional_headers =
                                                   QList<QPair<QByteArray, QByteArray>>(),
                                                 bool protected_contents = false,
                                                 const QString& username = QString(),
                                                 const QString& password = QString(),
                                                 const QNetworkProxy& custom_proxy =
                                                   QNetworkProxy::ProxyType::DefaultProxy);
    static NetworkResult performNetworkOperation(const QString& url,
                                                 int timeout,
                                                 QHttpMultiPart* input_data,
                                                 QList<HttpResponse>& output,
                                                 QNetworkAccessManager::Operation operation,
                                                 const QList<QPair<QByteArray, QByteArray>>& additional_headers =
                                                   QList<QPair<QByteArray, QByteArray>>(),
                                                 bool protected_contents = false,
                                                 const QString& username = QString(),
                                                 const QString& password = QString(),
                                                 const QNetworkProxy& custom_proxy =
                                                   QNetworkProxy::ProxyType::DefaultProxy);
};

Q_DECLARE_METATYPE(NetworkFactory::NetworkAuthentication)

#endif // NETWORKFACTORY_H
