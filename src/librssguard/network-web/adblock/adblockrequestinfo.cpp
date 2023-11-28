// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockrequestinfo.h"

#include "definitions/definitions.h"

#if defined(NO_LITE)
AdblockRequestInfo::AdblockRequestInfo(const QWebEngineUrlRequestInfo& webengine_info) {
  initialize(webengine_info);
}
#endif

AdblockRequestInfo::AdblockRequestInfo(const QUrl& url) {
  initialize(url);
}

QString AdblockRequestInfo::resourceType() const {
  return m_resourceType;
}

void AdblockRequestInfo::setResourceType(const QString& resource_type) {
  m_resourceType = resource_type;
}

QUrl AdblockRequestInfo::requestUrl() const {
  return m_requestUrl;
}

void AdblockRequestInfo::setRequestUrl(const QUrl& request_url) {
  m_requestUrl = request_url;
}

QUrl AdblockRequestInfo::firstPartyUrl() const {
  return m_firstPartyUrl;
}

void AdblockRequestInfo::setFirstPartyUrl(const QUrl& first_party_url) {
  m_firstPartyUrl = first_party_url;
}

QByteArray AdblockRequestInfo::requestMethod() const {
  return m_requestMethod;
}

void AdblockRequestInfo::setRequestMethod(const QByteArray& request_method) {
  m_requestMethod = request_method;
}

#if defined(NO_LITE)
void AdblockRequestInfo::initialize(const QWebEngineUrlRequestInfo& webengine_info) {
  setFirstPartyUrl(webengine_info.firstPartyUrl());
  setRequestMethod(webengine_info.requestMethod());
  setRequestUrl(webengine_info.requestUrl());
  setResourceType(convertResourceType(webengine_info.resourceType()));
}

QString AdblockRequestInfo::convertResourceType(QWebEngineUrlRequestInfo::ResourceType rt) const {
  switch (rt) {
    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeStylesheet:
      return QSL("stylesheet");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeScript:
      return QSL("script");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeImage:
      return QSL("image");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeFontResource:
      return QSL("object");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeSubResource:
      return QSL("object");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeObject:
      return QSL("object");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeMedia:
      return QSL("image");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeFavicon:
      return QSL("image");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeXhr:
      return QSL("xmlhttprequest");

    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeSubFrame:
    case QWebEngineUrlRequestInfo::ResourceType::ResourceTypeMainFrame:
      return QSL("main_frame");

    default:
      return {};
  }
}

#endif

void AdblockRequestInfo::initialize(const QUrl& url) {
  setFirstPartyUrl(url);
  setRequestMethod(QSL("GET").toLocal8Bit());
  setRequestUrl(url);

#if defined(NO_LITE)
  setResourceType(convertResourceType(QWebEngineUrlRequestInfo::ResourceType::ResourceTypeMainFrame));
#else
  setResourceType(QSL("main_frame"));
#endif
}
