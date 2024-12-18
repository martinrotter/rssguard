// For license of this file, see <project-root-folder>/LICENSE.md.
//
// This file is heavily inspired by https://github.com/ikskuh/kristall.

#ifndef GEMINICLIENT_H
#define GEMINICLIENT_H

#include <QMimeType>
#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>
#include <QUrl>

//! Cryptographic user identitiy consisting
//! of a key-certificate pair and some user information.
struct CryptoIdentity {
    //! The certificate that is used for cryptography
    QSslCertificate certificate;

    //! The actual private key that is used for cryptography
    QSslKey private_key;

    //! The title with which the identity is presented to the user.
    QString display_name;

    //! Notes that the user can have per identity for improved identity management
    QString user_notes;

    //! True for long-lived identities
    bool is_persistent = false;

    //! If not empty, Kristall will check
    QString host_filter = "";

    //! When this is set to true and the host_filter is not empty,
    //! the certificate will be automatically enabled for hosts matching the filter.
    bool auto_enable = false;

    bool isValid() const;

    //! returns true if a host does not match the filter criterion
    bool isHostFiltered(const QUrl& url) const;

    //! returns true when the identity should be enabled on url
    bool isAutomaticallyEnabledOn(const QUrl& url) const;
};

class GeminiClient : public QObject {
    Q_OBJECT

  public:
    enum class RequestState {
      None = 0,
      Started = 1,
      HostFound = 2,
      Connected = 3,

      StartedWeb = 255,
    };

    enum NetworkError {
      UnknownError,      //!< There was an unhandled network error
      ProtocolViolation, //!< The server responded with something unexpected and violated the protocol
      HostNotFound,      //!< The host was not found by the client
      ConnectionRefused, //!< The host refused connection on that port
      ResourceNotFound,  //!< The requested resource was not found on the server
      BadRequest,        //!< Our client misbehaved and did a request the server cannot understand
      ProxyRequest,      //!< We requested a proxy operation, but the server does not allow that
      InternalServerError,
      InvalidClientCertificate,
      UntrustedHost,  //!< We don't know the host, and we don't trust it
      MistrustedHost, //!< We know the host and it's not the server identity we've seen before
      Unauthorized,   //!< The requested resource could not be accessed.
      TlsFailure,     //!< Unspecified TLS failure
      Timeout,        //!< The network connection timed out.
    };

    enum RequestOptions {
      Default = 0,
      IgnoreTlsErrors = 1,
    };

    explicit GeminiClient(QObject* parent = nullptr);
    virtual ~GeminiClient();

    bool supportsUrl(const QString& url) const;
    bool supportsUrl(const QUrl& url) const;

    bool startRequest(const QUrl& url, RequestOptions options);
    bool inProgress() const;
    bool cancelRequest();

    bool enableClientCertificate(const CryptoIdentity& ident);
    void disableClientCertificate();

    QUrl targetUrl() const;

  signals:
    //! We successfully transferred some bytes from the server
    void requestProgress(qint64 transferred);

    //! The request completed with the given data and mime type
    void requestComplete(const QByteArray& data, const QString& mime);

    //! The state of the request has changed
    void requestStateChange(RequestState state);

    //! Server redirected us to another URL
    void redirected(const QUrl& uri, bool is_permanent);

    //! The server needs some information from the user to process this query.
    void inputRequired(const QString& user_query, bool is_sensitive);

    //! There was an error while processing the request
    void networkError(NetworkError error, const QString& reason);

    //! The server wants us to use a client certificate
    void certificateRequired(const QString& info);

    //! The server uses TLS and has a certificate.
    void hostCertificateLoaded(const QSslCertificate& cert);

  protected:
    void emitNetworkError(QAbstractSocket::SocketError error_code, const QString& textual_description);

  private slots:
    void socketEncrypted();
    void socketReadyRead();
    void socketDisconnected();
    void sslErrors(const QList<QSslError>& errors);
    void socketError(QAbstractSocket::SocketError socketError);

  private:
    bool m_isReceivingBody;
    bool m_suppressSocketTlsErrors;
    bool m_inErrorState;

    QUrl m_targetUrl;
    QSslSocket m_socket;
    QByteArray m_buffer;
    QByteArray m_body;
    QString m_mimeType;
    RequestOptions m_options;
};

#endif // GEMINICLIENT_H
