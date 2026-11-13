// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GCALENDARNETWORKFACTORY_H
#define GCALENDARNETWORKFACTORY_H

#include <atomic>
#include <functional>
#include <librssguard/exceptions/applicationexception.h>
#include <memory>

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QNetworkProxy>
#include <QUrl>
#include <QUrlQuery>

class OAuth2Service;

class CalendarApiException : public ApplicationException {
  public:
    CalendarApiException(int status, const QString& message) : ApplicationException(message), m_status(status) {}
    virtual void raise() const override {
      throw *this;
    }
    virtual CalendarApiException* clone() const override {
      return new CalendarApiException(*this);
    }
    int m_status;
};

// A request-scoped value object. OAuth remains on the account's thread; only token snapshots cross threads.
class GCalendarNetworkFactory {
  public:
    struct Context {
        QString m_bearer;
        QNetworkProxy m_proxy;
        QByteArray m_userAgent;
        QByteArray m_language;
        int m_timeout = 30000;
        int m_retryDelay = 1000;
        bool m_http2 = false;
        QUrl m_apiUrl = QUrl(QStringLiteral("https://www.googleapis.com/calendar/v3/"));
        std::shared_ptr<std::atomic_bool> m_cancelled = std::make_shared<std::atomic_bool>(false);
        // Empty requests a usable bearer; otherwise refresh the rejected bearer once.
        std::function<QString(const QString&)> m_authorize;
    };

    struct Request {
        QByteArray m_method;
        QUrl m_url;
        QByteArray m_body;
        QMap<QByteArray, QByteArray> m_headers;
    };

    struct Response {
        int m_status = 0;
        QByteArray m_body;
        QMap<QByteArray, QByteArray> m_headers;
        QString m_error;
    };

    using Transport = std::function<Response(const Request&, const Context&)>;

    explicit GCalendarNetworkFactory(Context context, Transport transport = {});
    static Context networkContext(const QNetworkProxy& proxy);
    static OAuth2Service* createOAuth(const QString& client_id,
                                      const QString& client_secret,
                                      const QNetworkProxy& proxy,
                                      QObject* parent);
    QList<QJsonObject> calendars() const;
    QList<QJsonObject> events(const QString& calendar_id, const QDateTime& from, const QDateTime& until) const;
    QJsonObject event(const QString& calendar_id, const QString& event_id) const;
    QList<QJsonObject> instances(const QString& calendar_id,
                                 const QString& event_id,
                                 const QDateTime& from,
                                 const QDateTime& until) const;
    QJsonObject insertEvent(const QString& calendar_id,
                            const QJsonObject& resource,
                            bool notify_attendees,
                            bool* recovered = nullptr) const;
    QJsonObject updateEvent(const QString& calendar_id,
                            const QString& event_id,
                            const QJsonObject& changes,
                            const QString& etag,
                            bool notify_attendees) const;
    static QString newEventId();

  private:
    QJsonObject request(const QByteArray& method,
                        const QByteArray& path,
                        const QUrlQuery& query = {},
                        const QJsonObject& body = {},
                        const QString& etag = {}) const;
    QList<QJsonObject> collection(const QByteArray& path, QUrlQuery query, const QString& kind) const;
    static QByteArray eventPath(const QString& calendar_id, const QString& event_id = {});
    static Response send(const Request& request, const Context& context);
    void checkCancelled() const;
    void wait(int milliseconds) const;

    Context m_context;
    Transport m_transport;
};

#endif // GCALENDARNETWORKFACTORY_H
