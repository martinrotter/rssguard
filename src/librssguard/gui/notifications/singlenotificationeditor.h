// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SINGLENOTIFICATIONEDITOR_H
#define SINGLENOTIFICATIONEDITOR_H

#include <QGroupBox>

#include "ui_singlenotificationeditor.h"

#include "miscellaneous/notification.h"

class SingleNotificationEditor : public QGroupBox {
  Q_OBJECT

  public:
    explicit SingleNotificationEditor(const Notification& notification, QWidget* parent = nullptr);

    Notification notification() const;

  signals:
    void notificationChanged();

  private slots:
    void selectSoundFile();
    void playSound();

  private:
    void loadNotification(const Notification& notification);

  private:
    Ui::SingleNotificationEditor m_ui;
    Notification::Event m_notificationEvent;
};

#endif // SINGLENOTIFICATIONEDITOR_H
