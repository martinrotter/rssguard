#include "gui/feedmessageviewer.h"
#include "gui/webbrowser.h"


FeedMessageViewer::FeedMessageViewer(QWidget *parent) : TabContent(parent)
{
}

FeedMessageViewer::~FeedMessageViewer() {
  qDebug("Destroying FeedMessageViewer instance.");
}

WebBrowser *FeedMessageViewer::webBrowser() {
  return NULL;
}
