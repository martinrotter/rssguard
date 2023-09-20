// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOASTNOTIFICATION_H
#define TOASTNOTIFICATION_H

#include "gui/notifications/basetoastnotification.h"

#include "miscellaneous/application.h"

#include "ui_toastnotification.h"

class ToastNotification : public BaseToastNotification {
    Q_OBJECT

  public:
    explicit ToastNotification(Notification::Event event,
                               const GuiMessage& msg,
                               const GuiAction& action,
                               QWidget* parent = nullptr);

  private:
    void setupHeading();
    void loadNotification(Notification::Event event, const GuiMessage& msg, const GuiAction& action);
    QIcon iconForType(QSystemTrayIcon::MessageIcon icon) const;

  private:
    Ui::ToastNotification m_ui;
};

#endif // TOASTNOTIFICATION_H
