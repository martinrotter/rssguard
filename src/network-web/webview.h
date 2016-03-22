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

#ifndef BASEWEBVIEW_H
#define BASEWEBVIEW_H

#include <QWebEngineView>


class QAction;
class QPaintEvent;
class WebPage;

class WebView : public QWebEngineView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit WebView(QWidget *parent = 0);
    virtual ~WebView();

    // Page accessor.
    inline WebPage *page() const {
      return m_page;
    }

    void setupIcons();

  signals:
    // Is emitted if user wants to open some hyperlink in new
    // web browser tab.
    void linkMiddleClicked(const QUrl &link_url);

    // User wants to open new empty web browser tab.
    void newTabRequested();

    // Emitted if user changes zoom factor via CTRL + mouse wheel combo.
    void zoomFactorChanged();

  public slots:
    // Page zoom modifiers.
    bool increaseWebPageZoom();
    bool decreaseWebPageZoom();
    bool resetWebPageZoom();

    // Executes if loading of any page is done.
    void onLoadFinished(bool ok);

    void copySelectedText();
    void openLinkInNewTab();
    void openLinkExternally();
    void openImageInNewTab();
    void searchTextViaGoogle();
    void saveCurrentPageToFile();
    void printCurrentPage();

    // Provides custom context menu.
    void popupContextMenu(const QPoint &pos);

  private slots:
    void downloadLink(const QNetworkRequest &request);

  protected:
    // Initializes all actions.
    void initializeActions();

    void setActionTexts();

    // Creates necessary connections.
    void createConnections();

    // Displays custom error page.
    void displayErrorPage();

    // Customize mouse wheeling.
    void wheelEvent(QWheelEvent *event);

    // Provides custom mouse actions.
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

  private:
    WebPage *m_page;

    QAction *m_actionReload;
    QAction *m_actionPrint;
    QAction *m_actionCopySelectedItem;
    QAction *m_actionCopyLink;
    QAction *m_actionCopyImage;
    QAction *m_actionSavePageAs;
    QAction *m_actionSaveHyperlinkAs;
    QAction *m_actionSaveImageAs;
    QAction *m_actionCopyImageUrl;
    QAction *m_actionOpenLinkThisTab;
    QAction *m_actionOpenLinkNewTab;
    QAction *m_actionOpenLinkExternally;
    QAction *m_actionOpenImageNewTab;
    QAction *m_actionLookupText;

    QPoint m_gestureOrigin;
    QUrl m_contextLinkUrl;
    QUrl m_contextImageUrl;
};

#endif // BASEWEBVIEW_H
