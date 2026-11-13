// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CALENDAREVENTPREVIEWER_H
#define CALENDAREVENTPREVIEWER_H

#include "ui_calendareventpreviewer.h"

#include <librssguard/services/abstract/gui/custommessagepreviewer.h>

#include <QPointer>

class GCalendarServiceRoot;

class CalendarEventPreviewer : public CustomMessagePreviewer {
    Q_OBJECT

  public:
    explicit CalendarEventPreviewer(GCalendarServiceRoot* account, QWidget* parent = nullptr);
    virtual WebBrowser* webBrowser() const override;
    virtual void clear() override;
    virtual void loadMessage(const Message& message, RootItem* selected_item, Feed* feed) override;

  private:
    void openLink(const QUrl& url);
    void setStatus(WidgetWithStatus::StatusType status, const QString& text);
    Ui::CalendarEventPreviewer m_ui;
    QPointer<GCalendarServiceRoot> m_account;
    Message m_message;
    QUrl m_googleUrl;
    QUrl m_meetingUrl;
};

#endif // CALENDAREVENTPREVIEWER_H
