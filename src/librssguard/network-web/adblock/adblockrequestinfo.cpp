// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockrequestinfo.h"

#include "definitions/definitions.h"

AdblockRequestInfo::AdblockRequestInfo(const QWebEngineUrlRequestInfo& webengine_info) {
  initialize(webengine_info);
}

AdblockRequestInfo::AdblockRequestInfo(const QUrl& url) {
  initialize(url);
}

QWebEngineUrlRequestInfo::ResourceType AdblockRequestInfo::resourceType() const {
  return m_resourceType;
}

void AdblockRequestInfo::setResourceType(const QWebEngineUrlRequestInfo::ResourceType& resourceType) {
  m_resourceType = resourceType;
}

QWebEngineUrlRequestInfo::NavigationType AdblockRequestInfo::navigationType() const {
  return m_navigationType;
}

void AdblockRequestInfo::setNavigationType(const QWebEngineUrlRequestInfo::NavigationType& navigationType) {
  m_navigationType = navigationType;
}

QUrl AdblockRequestInfo::requestUrl() const {
  return m_requestUrl;
}

void AdblockRequestInfo::setRequestUrl(const QUrl& requestUrl) {
  m_requestUrl = requestUrl;
}

QUrl AdblockRequestInfo::firstPartyUrl() const {
  return m_firstPartyUrl;
}

void AdblockRequestInfo::setFirstPartyUrl(const QUrl& firstPartyUrl) {
  m_firstPartyUrl = firstPartyUrl;
}

QByteArray AdblockRequestInfo::requestMethod() const {
  return m_requestMethod;
}

void AdblockRequestInfo::setRequestMethod(const QByteArray& requestMethod) {
  m_requestMethod = requestMethod;
}

void AdblockRequestInfo::initialize(const QWebEngineUrlRequestInfo& webengine_info) {
  setFirstPartyUrl(webengine_info.firstPartyUrl());
  setNavigationType(webengine_info.navigationType());
  setRequestMethod(webengine_info.requestMethod());
  setRequestUrl(webengine_info.requestUrl());
  setResourceType(webengine_info.resourceType());
}

void AdblockRequestInfo::initialize(const QUrl& url) {
  setFirstPartyUrl(url);
  setNavigationType(QWebEngineUrlRequestInfo::NavigationType::NavigationTypeTyped);
  setRequestMethod(QSL("GET").toLocal8Bit());
  setRequestUrl(url);
  setResourceType(QWebEngineUrlRequestInfo::ResourceType::ResourceTypeMainFrame);
}
