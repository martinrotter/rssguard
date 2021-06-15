// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SINGLENOTIFICATIONEDITOR_H
#define SINGLENOTIFICATIONEDITOR_H

#include <QWidget>

#include "ui_singlenotificationeditor.h"

#include "miscellaneous/notification.h"

class SingleNotificationEditor : public QWidget {
  Q_OBJECT

  public:
    explicit SingleNotificationEditor(const Notification& notification, QWidget* parent = nullptr);

    Notification notification() const;

    bool notificationEnabled() const;
    void setNotificationEnabled(bool enabled);

  private slots:
    void playSound();

  private:
    void loadNotification(const Notification& notification);

  private:
    Ui::SingleNotificationEditor m_ui;
    Notification::Event m_notificationEvent;
};

#endif // SINGLENOTIFICATIONEDITOR_H
