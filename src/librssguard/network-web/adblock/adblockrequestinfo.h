// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ADBLOCKREQUESTINFO_H
#define ADBLOCKREQUESTINFO_H

#if defined(USE_WEBENGINE)
#include <QWebEngineUrlRequestInfo>
#endif

#include <QUrl>

class AdblockRequestInfo {
  public:
#if defined(USE_WEBENGINE)
    explicit AdblockRequestInfo(const QWebEngineUrlRequestInfo& webengine_info);
#endif

    explicit AdblockRequestInfo(const QUrl& url);

    QString resourceType() const;
    void setResourceType(const QString& resource_type);

    QUrl requestUrl() const;
    void setRequestUrl(const QUrl& request_url);

    QUrl firstPartyUrl() const;
    void setFirstPartyUrl(const QUrl& first_party_url);

    QByteArray requestMethod() const;
    void setRequestMethod(const QByteArray& request_method);

  private:
    void initialize(const QUrl& url);

#if defined(USE_WEBENGINE)
    void initialize(const QWebEngineUrlRequestInfo& webengine_info);
    QString convertResourceType(QWebEngineUrlRequestInfo::ResourceType rt) const;
#endif

  private:
    QString m_resourceType;
    QUrl m_requestUrl;
    QUrl m_firstPartyUrl;
    QUrl m_initiator;
    QByteArray m_requestMethod;
};

#endif // ADBLOCKREQUESTINFO_H
