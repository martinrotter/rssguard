// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef THREADPREVIEWER_H
#define THREADPREVIEWER_H

#include "ui_threadpreviewer.h"

#include <librssguard/gui/webbrowser.h>
#include <librssguard/services/abstract/gui/custommessagepreviewer.h>

#include <QTimer>

class RedditServiceRoot;

class ThreadWebBrowser : public WebBrowser {
    Q_OBJECT

  public:
    explicit ThreadWebBrowser(WebViewer* viewer = nullptr, QWidget* parent = nullptr);
    virtual ~ThreadWebBrowser();

  protected slots:
    virtual void onLinkMouseHighlighted(const QUrl& url);
    virtual void onLinkMouseClicked(const QUrl& url);
};

class ThreadPreviewer : public CustomMessagePreviewer {
    Q_OBJECT

  public:
    explicit ThreadPreviewer(RedditServiceRoot* account, QWidget* parent = nullptr);
    virtual ~ThreadPreviewer();

    virtual WebBrowser* webBrowser() const;
    virtual void clear();
    virtual void loadMessage(const Message& msg, RootItem* selected_item);

  private slots:
    void fetchComments();

  private:
    Ui::ThreadPreviewer m_ui;
    RedditServiceRoot* m_account;
    QScopedPointer<ThreadWebBrowser> m_webView;
    Message m_message;
    RootItem* m_selectedItem;
    QTimer m_tmrLoadExtraMessageData;
};

#endif // THREADPREVIEWER_H
