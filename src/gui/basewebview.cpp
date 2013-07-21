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
  initializeActions();
  createConnections();
}

BaseWebView::~BaseWebView() {
  qDebug("Destroying BaseWebView.");
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

void BaseWebView::setupIcons() {
  m_actionReload->setIcon(ThemeFactory::fromTheme("view-refresh"));
  m_actionCopyLink->setIcon(ThemeFactory::fromTheme("edit-copy"));
  m_actionCopyImage->setIcon(ThemeFactory::fromTheme("insert-image"));
  m_actionCopyImageUrl->setIcon(ThemeFactory::fromTheme("edit-copy"));
}

void BaseWebView::initializeActions() {
  // Create needed actions.
  m_actionReload = pageAction(QWebPage::Reload);
  m_actionReload->setParent(this);
  m_actionReload->setText(tr("Reload web page"));
  m_actionReload->setToolTip(tr("Reload current web page"));

  m_actionCopyLink = pageAction(QWebPage::CopyLinkToClipboard);
  m_actionCopyLink->setParent(this);
  m_actionCopyLink->setText(tr("Copy link url"));
  m_actionCopyLink->setToolTip(tr("Copy link url to clipboard"));


  m_actionCopyImage = pageAction(QWebPage::CopyImageToClipboard);
  m_actionCopyImage->setParent(this);
  m_actionCopyImage->setText(tr("Copy image"));
  m_actionCopyImage->setToolTip(tr("Copy image to clipboard"));


  m_actionCopyImageUrl = pageAction(QWebPage::CopyImageUrlToClipboard);
  m_actionCopyImageUrl->setParent(this);
  m_actionCopyImageUrl->setText(tr("Copy image url"));
  m_actionCopyImageUrl->setToolTip(tr("Copy image url to clipboard"));
}

void BaseWebView::displayErrorPage() {
  // TODO: Add better custom error page.
  setHtml("error", url());
}

void BaseWebView::contextMenuEvent(QContextMenuEvent *event) {
  QMenu context_menu(tr("Web browser"), this);
  QMenu image_submenu(tr("Image"), &context_menu);
  QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(event->pos());

  image_submenu.setIcon(ThemeFactory::fromTheme("image-x-generic"));

  // Assemble the menu from actions.
  context_menu.addAction(m_actionReload);

  if (hit_result.linkUrl().isValid()) {
    context_menu.addAction(m_actionCopyLink);
  }

  if (!hit_result.pixmap().isNull()) {
    // Add 'Image' menu, because if user clicked image it needs to be visible.
    context_menu.addMenu(&image_submenu);

    image_submenu.addAction(m_actionCopyImage);
  }

  if (hit_result.imageUrl().isValid()) {
    image_submenu.addAction(m_actionCopyImageUrl);
  }

  // Display the menu.
  context_menu.exec(mapToGlobal(event->pos()));
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
