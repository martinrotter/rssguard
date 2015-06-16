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

#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include "gui/tabcontent.h"

#include "core/messagesmodel.h"
#include "network-web/webview.h"
#include "gui/locationlineedit.h"
#include "network-web/discoverfeedsbutton.h"

#include <QWidget>
#include <QWidgetAction>
#include <QUrl>
#include <QToolBar>


class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QProgressBar;
class QMenu;
class QLabel;
class WebBrowserNetworkAccessManager;
class TabWidget;

class WebBrowser : public TabContent {
    Q_OBJECT
    
  public:
    // Constructors and destructors.
    explicit WebBrowser(QWidget *parent = 0);
    virtual ~WebBrowser();

    // Reloads icons for all buttons.
    void setupIcons();

    // Returns icon associated with currently loaded website.
    inline QIcon icon() {
      return m_webView->icon();
    }

    inline WebView *view() {
      return m_webView;
    }

    // Sets this WebBrowser instance as focused.
    inline void setFocus(Qt::FocusReason reason) {
      m_txtLocation->setFocus(reason);
    }

    // Returns this instance.
    // NOTE: This is needed due to TabContent interface.
    inline WebBrowser *webBrowser() {
      return this;
    }

    // Returns global menu for this web browser.
    inline virtual QList<QAction*> globalMenu() {
      QList<QAction*> browser_menu;

      // Add needed actions into the menu.
      browser_menu.append(m_actionZoom);

      return browser_menu;
    }

    // Returns list of all running web browsers.
    static inline QList<WebBrowser*> runningWebBrowsers() {
      return m_runningWebBrowsers;
    }

  public slots:
    // Switches visibility of navigation bar.
    inline void setNavigationBarVisible(bool visible) {
      m_toolBar->setVisible(visible);
    }

    // Loads new url into the web browser.
    void navigateToUrl(const QString &url);
    void navigateToUrl(const QUrl &url);

    // Navigates to messages, used also as "newspaper" view.
    void navigateToMessages(const QList<Message> &messages);

    // Clears contents.
    inline void clear() {
      if (m_webView->url() != QUrl()) {
        m_webView->setHtml("<html><body></body></html>", QUrl(INTERNAL_URL_EMPTY));
      }
    }

    // Zoom manipulators.
    void increaseZoom();
    void decreaseZoom();
    void resetZoom();

  protected:
    // Creates necessary connections.
    void createConnections();

    // Initializes all buttons and widgets, which are needed for "Zoom" menu item.
    void initializeZoomWidget();

    // Initializes layout.
    void initializeLayout();

  protected slots:
    void onLoadingStarted();
    void onLoadingProgress(int progress);
    void onLoadingFinished(bool success);

    // Updates zoom-related gui.
    void updateZoomGui();

    // Updates url (for example on location text box).
    void updateUrl(const QUrl &url);

    // Title/icon is changed.
    void onTitleChanged(const QString &new_title);
    void onIconChanged();

    // User selected any feed from website to add to reader.
    // This copies feed link to clipboard and triggers "add feed" dialog.
    void addFeedFromWebsite(const QString &feed_link);

  signals:
    // User requests opening of new tab or clicks the link
    // with middle mouse button
    void newTabRequested();
    void linkMiddleClicked(const QUrl &link_url);

    // Title/icon is changed.
    void iconChanged(int index, const QIcon &icon);
    void titleChanged(int index, const QString &title);

  private:
    QVBoxLayout *m_layout;
    QToolBar *m_toolBar;
    WebView *m_webView;
    LocationLineEdit *m_txtLocation;
    QWidget *m_zoomButtons;
    QToolButton *m_btnResetZoom;
    QHBoxLayout *m_loadingLayout;
    QProgressBar *m_loadingProgress;
    QLabel *m_lblProgress;
    DiscoverFeedsButton *m_btnDiscoverFeeds;

    QWidgetAction *m_actionZoom;
    QAction *m_actionBack;
    QAction *m_actionForward;
    QAction *m_actionReload;
    QAction *m_actionStop;

    static QList<WebBrowser*> m_runningWebBrowsers;
};

#endif // WEBBROWSER_H
