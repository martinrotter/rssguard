// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef EMAILPREVIEWER_H
#define EMAILPREVIEWER_H

#include "ui_emailpreviewer.h"

#include <librssguard/gui/webbrowser.h>
#include <librssguard/services/abstract/gui/custommessagepreviewer.h>

#include <QTimer>

class GmailServiceRoot;

class EmailPreviewer : public CustomMessagePreviewer {
    Q_OBJECT

  public:
    explicit EmailPreviewer(GmailServiceRoot* account, QWidget* parent = nullptr);
    virtual ~EmailPreviewer();

    virtual WebBrowser* webBrowser() const;
    virtual void clear();
    virtual void loadMessage(const Message& msg, RootItem* selected_item);

  private slots:
    void loadExtraMessageData();
    void replyToEmail();
    void forwardEmail();
    void downloadAttachment(QAction* act);

  private:
    Ui::EmailPreviewer m_ui;
    GmailServiceRoot* m_account;
    QScopedPointer<WebBrowser> m_webView;
    Message m_message;
    QTimer m_tmrLoadExtraMessageData;
};

#endif // EMAILPREVIEWER_H
