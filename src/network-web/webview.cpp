// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "network-web/webview.h"

#include "definitions/definitions.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/skinfactory.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webpage.h"
#include "network-web/webfactory.h"
#include "gui/messagebox.h"
#include "gui/formmain.h"

#include <QStyleOptionFrameV3>
#include <QAction>
#include <QMenu>
#include <QDir>
#include <QFile>
#include <QWebFrame>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QClipboard>
#include <QFileDialog>

#if QT_VERSION >= 0x050000
#include <QtPrintSupport/QPrintPreviewDialog>
#else
#include <QPrintPreviewDialog>
#endif


WebView::WebView(QWidget *parent)
  : QWebView(parent), m_page(new WebPage(this)) {
  setPage(m_page);
  setContextMenuPolicy(Qt::CustomContextMenu);
  initializeActions();
  createConnections();
}

WebView::~WebView() {
  qDebug("Destroying WebView.");
}

void WebView::onLoadFinished(bool ok) {
  // If page was not loaded, then display custom error page.
  if (!ok) {
    displayErrorPage();
  }
}

void WebView::copySelectedText() {
  Application::clipboard()->setText(selectedText());
}

void WebView::openLinkInNewTab() {
  emit linkMiddleClicked(m_contextLinkUrl);
}

void WebView::openLinkExternally() {
  WebFactory::instance()->openUrlInExternalBrowser(m_contextLinkUrl.toString());
}

void WebView::openImageInNewTab() {
  emit linkMiddleClicked(m_contextImageUrl);
}

void WebView::saveCurrentPageToFile() {
  QString selected_file;

  if (qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::AlwaysPromptForFilename)).toBool()) {
    QString filter_html = tr("HTML web pages (*.html)");

    QString filter;
    QString selected_filter;
    QString filename_for_prompt = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetExplicitDirectory)).toString() +
                                  QDir::separator() + tr("source_page.html");

    // Add more filters here.
    filter += filter_html;
    selected_file = QFileDialog::getSaveFileName(this, tr("Select destination file for web page"),
                                                 filename_for_prompt, filter, &selected_filter);

    if (!selected_file.isEmpty()) {
      qApp->settings()->setValue(GROUP(Downloads), Downloads::TargetExplicitDirectory,
                                 QDir::toNativeSeparators(QFileInfo(selected_file).absolutePath()));
    }
  }
  else {
    selected_file = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetDirectory)).toString();

    if (!selected_file.endsWith(QDir::separator())) {
      selected_file += QDir::separator();
    }

    selected_file += tr("source_page.html");
  }

  if (!selected_file.isEmpty()) {
    QFile selected_file_handle(selected_file);

    if (selected_file_handle.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
      QString html_text = page()->mainFrame()->toHtml();
      QTextStream str(&selected_file_handle);

      str.setCodec("UTF-16");
      str << html_text;
      selected_file_handle.close();
    }
    else {
      MessageBox::show(this, QMessageBox::Critical, tr("Cannot save web page"),
                       tr("Web page cannot be saved because destination file is not writtable."));
    }
  }
}

void WebView::createConnections() {
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
  connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(popupContextMenu(QPoint)));
  connect(page(), SIGNAL(downloadRequested(QNetworkRequest)), this, SLOT(downloadLink(QNetworkRequest)));

  connect(m_actionSavePageAs, SIGNAL(triggered()), this, SLOT(saveCurrentPageToFile()));
  connect(m_actionPrint, SIGNAL(triggered()), this, SLOT(printCurrentPage()));
  connect(m_actionOpenLinkNewTab, SIGNAL(triggered()), this, SLOT(openLinkInNewTab()));
  connect(m_actionOpenImageNewTab, SIGNAL(triggered()), this, SLOT(openImageInNewTab()));
  connect(m_actionOpenLinkExternally, SIGNAL(triggered()), this, SLOT(openLinkExternally()));
}

void WebView::setupIcons() {
  m_actionPrint->setIcon(qApp->icons()->fromTheme("print-web-page"));
  m_actionReload->setIcon(qApp->icons()->fromTheme("go-refresh"));
  m_actionCopySelectedItem->setIcon(qApp->icons()->fromTheme("edit-copy"));
  m_actionCopyLink->setIcon(qApp->icons()->fromTheme("edit-copy"));
  m_actionCopyImage->setIcon(qApp->icons()->fromTheme("edit-copy-image"));
  m_actionSaveHyperlinkAs->setIcon(qApp->icons()->fromTheme("document-download"));
  m_actionSaveImageAs->setIcon(qApp->icons()->fromTheme("document-download"));

#if QT_VERSION >= 0x040800
  m_actionCopyImageUrl->setIcon(qApp->icons()->fromTheme("edit-copy"));
#endif

  m_actionOpenLinkThisTab->setIcon(qApp->icons()->fromTheme("item-open-internal"));
  m_actionOpenLinkNewTab->setIcon(qApp->icons()->fromTheme("item-open-internal"));
  m_actionOpenLinkExternally->setIcon(qApp->icons()->fromTheme("item-open-external"));
  m_actionOpenImageNewTab->setIcon(qApp->icons()->fromTheme("edit-copy-image"));
}

void WebView::initializeActions() {
  // Create needed actions.
  m_actionReload = pageAction(QWebPage::Reload);
  m_actionReload->setParent(this);

  m_actionPrint = new QAction(tr("Print"), this);
  m_actionPrint->setToolTip(tr("Print current web page."));

  m_actionCopySelectedItem = pageAction(QWebPage::Copy);
  m_actionCopySelectedItem->setParent(this);

#if defined(Q_OS_OS2)
  m_actionCopySelectedItem->setShortcut(QKeySequence::Copy);
  addAction(m_actionCopySelectedItem);
#endif

  m_actionSaveHyperlinkAs = pageAction(QWebPage::DownloadLinkToDisk);
  m_actionSaveHyperlinkAs->setParent(this);

  m_actionCopyLink = pageAction(QWebPage::CopyLinkToClipboard);
  m_actionCopyLink->setParent(this);

  m_actionCopyImage = pageAction(QWebPage::CopyImageToClipboard);
  m_actionCopyImage->setParent(this);

  m_actionSaveImageAs = pageAction(QWebPage::DownloadImageToDisk);
  m_actionSaveImageAs->setParent(this);

  m_actionSavePageAs = new QAction(qApp->icons()->fromTheme("document-download"), tr("Save page as..."), this);

#if QT_VERSION >= 0x040800
  m_actionCopyImageUrl = pageAction(QWebPage::CopyImageUrlToClipboard);
  m_actionCopyImageUrl->setParent(this);
#endif

  m_actionOpenLinkNewTab = pageAction(QWebPage::OpenLinkInNewWindow);
  m_actionOpenLinkNewTab->setParent(this);

  m_actionOpenLinkThisTab = pageAction(QWebPage::OpenLink);
  m_actionOpenLinkThisTab->setParent(this);

  m_actionOpenLinkExternally = new QAction(tr("Open link in external browser"), this);

  m_actionOpenImageNewTab = pageAction(QWebPage::OpenImageInNewWindow);
  m_actionOpenImageNewTab->setParent(this);

}

void WebView::setActionTexts() {
  m_actionReload->setText(tr("Reload web page"));
  m_actionReload->setToolTip(tr("Reload current web page."));
  m_actionCopySelectedItem->setText(tr("Copy selection"));
  m_actionCopySelectedItem->setToolTip(tr("Copies current selection into the clipboard."));
  m_actionSaveHyperlinkAs->setText(tr("Save target as..."));
  m_actionSaveHyperlinkAs->setToolTip(tr("Download content from the hyperlink."));
  m_actionCopyLink->setText(tr("Copy link url"));
  m_actionCopyLink->setToolTip(tr("Copy link url to clipboard."));
  m_actionCopyImage->setText(tr("Copy image"));
  m_actionCopyImage->setToolTip(tr("Copy image to clipboard."));
  m_actionSaveImageAs->setText(tr("Save image as..."));
  m_actionSaveImageAs->setToolTip(tr("Save image to disk."));

#if QT_VERSION >= 0x040800
  m_actionCopyImageUrl->setText(tr("Copy image url"));
  m_actionCopyImageUrl->setToolTip(tr("Copy image url to clipboard."));
#endif

  m_actionOpenLinkNewTab->setText(tr("Open link in new tab"));
  m_actionOpenLinkNewTab->setToolTip(tr("Open this hyperlink in new tab."));
  m_actionOpenLinkThisTab->setText(tr("Follow link"));
  m_actionOpenLinkThisTab->setToolTip(tr("Open the hyperlink in this tab."));
  m_actionOpenLinkExternally->setToolTip(tr("Open the hyperlink in external browser."));
  m_actionOpenImageNewTab->setText(tr("Open image in new tab"));
  m_actionOpenImageNewTab->setToolTip(tr("Open this image in this tab."));
}

void WebView::displayErrorPage() {
  /*
  setHtml(qApp->skins()->currentMarkupLayout().arg(
            tr("Error page"),
            qApp->skins()->currentMarkup().arg(tr("Page not found"),
                                               tr("Check your internet connection or website address"),
                                               QString(),
                                               tr("This failure can be caused by:<br><ul>"
                                                  "<li>non-functional internet connection,</li>"
                                                  "<li>incorrect website address,</li>"
                                                  "<li>bad proxy server settings,</li>"
                                                  "<li>target destination outage,</li>"
                                                  "<li>many other things.</li>"
                                                  "</ul>"),
                                               QDateTime::currentDateTime().toString(Qt::DefaultLocaleShortDate))));
  */
}

void WebView::popupContextMenu(const QPoint &pos) {
  QMenu context_menu(tr("Web browser"), this);
  QMenu image_submenu(tr("Image"), &context_menu);
  QMenu link_submenu(tr("Hyperlink"), this);
  QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(pos);

  image_submenu.setIcon(qApp->icons()->fromTheme("image-generic"));
  link_submenu.setIcon(qApp->icons()->fromTheme("text-html"));

  // Assemble the menu from actions.

  QString current_url = url().toString();

  if (!current_url.isEmpty() && current_url != INTERNAL_URL_EMPTY && current_url != INTERNAL_URL_BLANK) {
    context_menu.addAction(m_actionPrint);

    if (current_url != INTERNAL_URL_NEWSPAPER) {
      context_menu.addAction(m_actionReload);
    }
  }

  context_menu.addAction(m_actionCopySelectedItem);
  context_menu.addAction(m_actionSavePageAs);

  QUrl hit_url = hit_result.linkUrl();
  QUrl hit_image_url = hit_result.imageUrl();

  if (hit_url.isValid()) {
    m_contextLinkUrl = hit_url;

    context_menu.addMenu(&link_submenu);
    link_submenu.addAction(m_actionOpenLinkThisTab);
    link_submenu.addAction(m_actionOpenLinkNewTab);
    link_submenu.addAction(m_actionOpenLinkExternally);
    link_submenu.addAction(m_actionCopyLink);
    link_submenu.addAction(m_actionSaveHyperlinkAs);
  }

  if (!hit_result.pixmap().isNull()) {
    // Add 'Image' menu, because if user clicked image it needs to be visible.
    context_menu.addMenu(&image_submenu);

    if (hit_image_url.isValid()) {
      m_contextImageUrl = hit_image_url;
      image_submenu.addAction(m_actionOpenImageNewTab);

#if QT_VERSION >= 0x040800
      image_submenu.addAction(m_actionCopyImageUrl);
#endif
    }
    image_submenu.addAction(m_actionCopyImage);
    image_submenu.addAction(m_actionSaveImageAs);
  }

  // Display the menu.
  setActionTexts();
  context_menu.exec(mapToGlobal(pos));
}

void WebView::printCurrentPage() {
  QPointer<QPrintPreviewDialog> print_preview = new QPrintPreviewDialog(this);
  connect(print_preview.data(), SIGNAL(paintRequested(QPrinter*)), this, SLOT(print(QPrinter*)));
  print_preview.data()->exec();
}

void WebView::downloadLink(const QNetworkRequest &request) {
  qApp->downloadManager()->download(request);
}

void WebView::mousePressEvent(QMouseEvent *event) {
  if (event->button() & Qt::LeftButton && event->modifiers() & Qt::ControlModifier) {
    QWebHitTestResult hit_result = page()->mainFrame()->hitTestContent(event->pos());

    // Check if user clicked with middle mouse button on some
    // hyperlink.
    QUrl link_url = hit_result.linkUrl();
    QUrl image_url = hit_result.imageUrl();

    if (link_url.isValid()) {
      emit linkMiddleClicked(link_url);
      return;
    }
    else if (image_url.isValid()) {
      emit linkMiddleClicked(image_url);
      return;
    }
  }
  else if (event->button() & Qt::MiddleButton) {
    m_gestureOrigin = event->pos();
    return;
  }

  QWebView::mousePressEvent(event);
}

void WebView::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() & Qt::MiddleButton) {
    bool are_gestures_enabled = qApp->settings()->value(GROUP(Browser), SETTING(Browser::GesturesEnabled)).toBool();

    if (are_gestures_enabled) {
      QPoint release_point = event->pos();
      int left_move = m_gestureOrigin.x() - release_point.x();
      int right_move = -left_move;
      int top_move = m_gestureOrigin.y() - release_point.y();
      int bottom_move = -top_move;
      int total_max = qMax(qMax(qMax(left_move, right_move), qMax(top_move, bottom_move)), 40);

      if (total_max == left_move) {
        back();
      }
      else if (total_max == right_move) {
        forward();
      }
      else if (total_max == top_move) {
        reload();
      }
      else if (total_max == bottom_move) {
        emit newTabRequested();
      }
    }
  }

  QWebView::mouseReleaseEvent(event);
}

void WebView::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() & Qt::ControlModifier) {
    if (event->delta() > 0) {
      increaseWebPageZoom();
      emit zoomFactorChanged();
      return;
    }
    else if (event->delta() < 0) {
      decreaseWebPageZoom();
      emit zoomFactorChanged();
      return;
    }
  }

  QWebView::wheelEvent(event);
}

bool WebView::increaseWebPageZoom() {
  qreal new_factor = zoomFactor() + 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool WebView::decreaseWebPageZoom() {
  qreal new_factor = zoomFactor() - 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool WebView::resetWebPageZoom() {
  qreal new_factor = 1.0;

  if (new_factor != zoomFactor()) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}
