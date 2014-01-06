#include "core/webpage.h"

#include "core/webbrowsernetworkaccessmanager.h"
#include "gui/webbrowser.h"

#include <QNetworkReply>
#include <QWebFrame>


WebPage::WebPage(QObject *parent)
  : QWebPage(parent) {
  // Setup global network access manager.
  // NOTE: This makes network settings easy for all web browsers.
  setNetworkAccessManager(WebBrowser::globalNetworkManager());
}

WebPage::~WebPage() {
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame,
                                      const QNetworkRequest &request,
                                      QWebPage::NavigationType type) {
  if (type == QWebPage::NavigationTypeLinkClicked &&
      frame == mainFrame()) {
    // Make sure that appropriate signal is emitted even if
    // no delegation is enabled.
    emit linkClicked(request.url());
  }

  return QWebPage::acceptNavigationRequest(frame, request, type);
}
