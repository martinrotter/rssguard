// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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
#include "miscellaneous/textfactory.h"
#include "network-web/webpage.h"
#include "network-web/webfactory.h"
#include "gui/messagebox.h"
#include "gui/dialogs/formmain.h"

#include <QStyleOptionFrameV3>
#include <QAction>
#include <QMenu>
#include <QDir>
#include <QFile>
#include <QWebEnginePage>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QClipboard>
#include <QFileDialog>
#include <QtPrintSupport/QPrintPreviewDialog>


WebView::WebView(QWidget *parent)
  : QWebEngineView(parent), m_page(new WebPage(this)) {
  setPage(m_page);
  initializeActions();
  createConnections();
}

WebView::~WebView() {
  qDebug("Destroying WebView.");
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

void WebView::searchTextViaGoogle() {
  emit linkMiddleClicked(QString(GOOGLE_SEARCH_URL).arg((selectedText())));
}

void WebView::saveCurrentPageToFile() {
  QString selected_file;
  const QString implicit_file_base_name = tr("source_page");

  // NOTE: It is good to always ask for destination here, since download manager
  // is not displaying afterwards because this is *not* real download actually.
  //if (qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::AlwaysPromptForFilename)).toBool()) {
  const QString filter_html = tr("HTML web pages (*.html)");

  QString filter;
  QString selected_filter;
  const QString filename_for_prompt = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetExplicitDirectory)).toString() +
                                      QDir::separator() + implicit_file_base_name + QL1S(".html");

  // Add more filters here.
  filter += filter_html;
  selected_file = QFileDialog::getSaveFileName(this, tr("Select destination file for web page"),
                                               filename_for_prompt, filter, &selected_filter);

  if (!selected_file.isEmpty()) {
    qApp->settings()->setValue(GROUP(Downloads), Downloads::TargetExplicitDirectory,
                               QDir::toNativeSeparators(QFileInfo(selected_file).absolutePath()));
  }
  /*}
  else {
    QString base_folder = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetDirectory)).toString();

    if (!base_folder.endsWith(QDir::separator())) {
      base_folder += QDir::separator();
    }

    selected_file = base_folder + implicit_file_base_name + QL1S(".html");

    if (QFile::exists(selected_file)) {
      int file_suffix = 0;
      QString subsequent_file_name;

      do {
        subsequent_file_name = base_folder + QString(implicit_file_base_name + QL1S("-%1.html")).arg(file_suffix++);
      } while (QFile::exists(subsequent_file_name));

      selected_file = subsequent_file_name;
    }
  }*/

  if (!selected_file.isEmpty()) {
    page()->toHtml([this, selected_file](const QString &html) {
      QFile selected_file_handle(selected_file);

      if (selected_file_handle.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        QTextStream str(&selected_file_handle);

        str.setCodec("UTF-16");
        str << html;
        selected_file_handle.close();
      }
      else {
        MessageBox::show(this, QMessageBox::Critical, tr("Cannot save web page"),
                         tr("Web page cannot be saved because destination file is not writtable."));
      }
    });
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
  connect(m_actionLookupText, SIGNAL(triggered()), this, SLOT(searchTextViaGoogle()));
}

void WebView::setupIcons() {
  m_actionPrint->setIcon(qApp->icons()->fromTheme(QSL("print-web-page")));
  m_actionCopySelectedItem->setIcon(qApp->icons()->fromTheme(QSL("edit-copy")));
  m_actionCopyLink->setIcon(qApp->icons()->fromTheme(QSL("edit-copy")));
  m_actionCopyImage->setIcon(qApp->icons()->fromTheme(QSL("edit-copy-image")));
  m_actionSaveHyperlinkAs->setIcon(qApp->icons()->fromTheme(QSL("document-download")));
  m_actionSaveImageAs->setIcon(qApp->icons()->fromTheme(QSL("document-download")));
  m_actionCopyImageUrl->setIcon(qApp->icons()->fromTheme(QSL("edit-copy")));
  m_actionOpenLinkThisTab->setIcon(qApp->icons()->fromTheme(QSL("item-open-internal")));
  m_actionOpenLinkNewTab->setIcon(qApp->icons()->fromTheme(QSL("item-open-internal")));
  m_actionOpenLinkExternally->setIcon(qApp->icons()->fromTheme(QSL("item-open-external")));
  m_actionOpenImageNewTab->setIcon(qApp->icons()->fromTheme(QSL("edit-copy-image")));

  m_actionLookupText->setIcon(qApp->icons()->fromTheme(QSL("item-search-google")));
}

void WebView::initializeActions() {  
  m_actionPrint = new QAction(tr("Print"), this);
  m_actionPrint->setToolTip(tr("Print current web page."));

  m_actionCopySelectedItem = pageAction(QWebEnginePage::Copy);
  m_actionCopySelectedItem->setParent(this);

  m_actionSaveHyperlinkAs = pageAction(QWebEnginePage::DownloadLinkToDisk);
  m_actionSaveHyperlinkAs->setParent(this);

  m_actionCopyLink = pageAction(QWebEnginePage::CopyLinkToClipboard);
  m_actionCopyLink->setParent(this);

  m_actionCopyImage = pageAction(QWebEnginePage::CopyImageToClipboard);
  m_actionCopyImage->setParent(this);

  m_actionSaveImageAs = pageAction(QWebEnginePage::DownloadImageToDisk);
  m_actionSaveImageAs->setParent(this);
  m_actionSavePageAs = new QAction(qApp->icons()->fromTheme(QSL("document-download")), tr("Save page as..."), this);

  m_actionCopyImageUrl = pageAction(QWebEnginePage::CopyImageUrlToClipboard);
  m_actionCopyImageUrl->setParent(this);

  m_actionOpenLinkNewTab = pageAction(QWebEnginePage::OpenLinkInNewTab);
  m_actionOpenLinkNewTab->setParent(this);

  m_actionOpenLinkThisTab = pageAction(QWebEnginePage::OpenLinkInThisWindow);
  m_actionOpenLinkThisTab->setParent(this);

  m_actionOpenLinkExternally = new QAction(tr("Open link in external browser"), this);

  // TODO: bude fungovat?
  m_actionOpenImageNewTab = pageAction(QWebEnginePage::OpenLinkInNewTab);
  m_actionOpenImageNewTab->setParent(this);

  m_actionLookupText = new QAction("", this);
}

void WebView::setActionTexts() {
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

  m_actionCopyImageUrl->setText(tr("Copy image url"));
  m_actionCopyImageUrl->setToolTip(tr("Copy image url to clipboard."));

  m_actionOpenLinkNewTab->setText(tr("Open link in new tab"));
  m_actionOpenLinkNewTab->setToolTip(tr("Open this hyperlink in new tab."));
  m_actionOpenLinkThisTab->setText(tr("Follow link"));
  m_actionOpenLinkThisTab->setToolTip(tr("Open the hyperlink in this tab."));
  m_actionOpenLinkExternally->setToolTip(tr("Open the hyperlink in external browser."));
  m_actionOpenImageNewTab->setText(tr("Open image in new tab"));
  m_actionOpenImageNewTab->setToolTip(tr("Open this image in this tab."));
}

void WebView::printCurrentPage() {
  QScopedPointer<QPrintPreviewDialog> print_preview(new QPrintPreviewDialog(this));
  connect(print_preview.data(), SIGNAL(paintRequested(QPrinter*)), this, SLOT(print(QPrinter*)));
  print_preview.data()->exec();
}

void WebView::downloadLink(const QNetworkRequest &request) {
  qApp->downloadManager()->download(request);
}

void WebView::mousePressEvent(QMouseEvent *event) {
  if (event->button() & Qt::MiddleButton) {
    m_gestureOrigin = event->pos();
  }
  else {
    QWebEngineView::mousePressEvent(event);
  }
}

void WebView::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() & Qt::MiddleButton) {
    const bool are_gestures_enabled = qApp->settings()->value(GROUP(Browser), SETTING(Browser::GesturesEnabled)).toBool();

    if (are_gestures_enabled) {
      const QPoint release_point = event->pos();
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

  QWebEngineView::mouseReleaseEvent(event);
}

void WebView::contextMenuEvent(QContextMenuEvent *event) {
  QWebEngineView::contextMenuEvent(event);
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

  QWebEngineView::wheelEvent(event);
}

bool WebView::increaseWebPageZoom() {
  const qreal new_factor = zoomFactor() + 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool WebView::decreaseWebPageZoom() {
  const qreal new_factor = zoomFactor() - 0.1;

  if (new_factor >= 0.0 && new_factor <= MAX_ZOOM_FACTOR) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}

bool WebView::resetWebPageZoom() {
  const qreal new_factor = 1.0;

  if (new_factor != zoomFactor()) {
    setZoomFactor(new_factor);
    return true;
  }
  else {
    return false;
  }
}
