#include <QStyleOptionFrameV3>

#include "gui/basewebview.h"
#include "gui/basewebpage.h"


BaseWebView::BaseWebView(QWidget *parent)
  : QWebView(parent), m_page(new BaseWebPage(this)) {
  setPage(m_page);
}

void BaseWebView::paintEvent(QPaintEvent *event) {
  QWebView::paintEvent(event);

  // TODO: Add frame, inspire from QFrame source code - paint event.
}
