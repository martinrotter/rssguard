// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "core/message.h"
#include "network-web/webpage.h"
#include "services/abstract/rootitem.h"

#include <QPointer>
#include <QToolBar>


class QToolButton;
class QVBoxLayout;
class QHBoxLayout;
class QProgressBar;
class QMenu;
class QLabel;
class TabWidget;
class WebViewer;
class LocationLineEdit;
class DiscoverFeedsButton;

class WebBrowser : public TabContent {
    Q_OBJECT

  public:
    explicit WebBrowser(QWidget *parent = 0);
    virtual ~WebBrowser();

    WebBrowser *webBrowser() const {
      return const_cast<WebBrowser*>(this);
    }

    WebViewer *viewer() const {
      return m_webView;
    }

    void reloadFontSettings();

  public slots:
    void increaseZoom();
    void decreaseZoom();
    void resetZoom();

    void clear();
    void loadUrl(const QString &url);
    void loadUrl(const QUrl &url);
    void loadMessages(const QList<Message> &messages, RootItem *root);
    void loadMessage(const Message &message, RootItem *root);

    // Switches visibility of navigation bar.
    inline void setNavigationBarVisible(bool visible) {
      m_toolBar->setVisible(visible);
    }

  private slots:
    void updateUrl(const QUrl &url);

    void onLoadingStarted();
    void onLoadingProgress(int progress);
    void onLoadingFinished(bool success);

    void receiveMessageStatusChangeRequest(int message_id, WebPage::MessageStatusChange change);

    void onTitleChanged(const QString &new_title);

#if QT_VERSION >= 0x050700
    void onIconChanged(const QIcon &icon);
#endif

  signals:
    // Title/icon is changed.
    void iconChanged(int index, const QIcon &icon);
    void titleChanged(int index, const QString &title);

    void markMessageRead(int id, RootItem::ReadStatus read);
    void markMessageImportant(int id, RootItem::Importance important);
    void requestMessageListReload(bool mark_current_as_read);

  private:
    void initializeLayout();
    Message *findMessage(int id);
    void markMessageAsRead(int id, bool read);
    void switchMessageImportance(int id, bool checked);
    void createConnections();

    QVBoxLayout *m_layout;
    QToolBar *m_toolBar;
    WebViewer *m_webView;
    LocationLineEdit *m_txtLocation;
    DiscoverFeedsButton *m_btnDiscoverFeeds;
    QProgressBar *m_loadingProgress;

    QAction *m_actionBack;
    QAction *m_actionForward;
    QAction *m_actionReload;
    QAction *m_actionStop;

    QList<Message> m_messages;
    QPointer<RootItem> m_root;
};

#endif // WEBBROWSER_H
