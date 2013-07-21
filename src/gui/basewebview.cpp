#include <QStyleOptionFrameV3>
#include <QAction>
#include <QMenu>
#include <QWebFrame>
#include <QContextMenuEvent>

#include "core/basewebpage.h"
#include "gui/basewebview.h"
#include "gui/themefactory.h"


BaseWebView::BaseWebView(QWidget *parent)
  : QWebView(parent), m_page(new BaseWebPage(this)) {
  setPage(m_page);
  createConnections();
}

BaseWebView::~BaseWebView() {
}

void BaseWebView::onLoadFinished(bool ok) {
  // If page was not loaded, then display custom error page.
  if (!ok) {
    displayErrorPage();
  }
}

void BaseWebView::createConnections() {
  connect(this, &BaseWebView::loadFinished,
          this, &BaseWebView::onLoadFinished);
}

void BaseWebView::displayErrorPage() {
  // TODO: Add better custom error page.
  setHtml("error", url());
}

void BaseWebView::contextMenuEvent(QContextMenuEvent *event) {
  QMenu context_menu(tr("Web browser"), this);
  QMenu image_submenu(tr("Image"), &context_menu);
  QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(event->pos());

  // Obtain needed actions.
  QAction *action_reload = pageAction(QWebPage::Reload);
  action_reload->setText(tr("Reload web page"));
  action_reload->setToolTip(tr("Reload current web page"));
  context_menu.addAction(action_reload);

  if (hit_result.linkUrl().isValid()) {
    QAction *action_copylink = pageAction(QWebPage::CopyLinkToClipboard);
    action_copylink->setText(tr("Copy link url"));
    action_copylink->setToolTip(tr("Copy link url to clipboard"));
    action_copylink->setIcon(ThemeFactory::fromTheme("edit-copy"));
    context_menu.addAction(action_copylink);
  }

  if (!hit_result.pixmap().isNull()) {
    context_menu.addMenu(&image_submenu);

    QAction *action_copyimage = pageAction(QWebPage::CopyImageToClipboard);
    action_copyimage->setText(tr("Copy image"));
    action_copyimage->setToolTip(tr("Copy image to clipboard"));
    action_copyimage->setIcon(ThemeFactory::fromTheme("insert-image"));
    image_submenu.addAction(action_copyimage);
  }

  if (hit_result.imageUrl().isValid()) {
    QAction *action_copyimageurl = pageAction(QWebPage::CopyImageUrlToClipboard);
    action_copyimageurl->setText(tr("Copy image url"));
    action_copyimageurl->setToolTip(tr("Copy image url to clipboard"));
    action_copyimageurl->setIcon(ThemeFactory::fromTheme("edit-copy"));
    image_submenu.addAction(action_copyimageurl);
  }

  // Display the menu.
  context_menu.exec(mapToGlobal(event->pos()));
  context_menu.deleteLater();
}

void BaseWebView::paintEvent(QPaintEvent *event) {
  QWebView::paintEvent(event);

  // Draw additional frame.
  QPainter painter(this);
  QStyleOptionFrameV3 style_option;
  int frame_shape = QFrame::Sunken & QFrame::Shape_Mask;

  style_option.init(this);
  style_option.frameShape = QFrame::Shape(int(style_option.frameShape) |
                                          QFrame::StyledPanel |
                                          frame_shape);
  style_option.rect = rect();
  style_option.lineWidth = 1;
  style_option.midLineWidth = 0;

  style()->drawControl(QStyle::CE_ShapedFrame, &style_option, &painter, this);
}
