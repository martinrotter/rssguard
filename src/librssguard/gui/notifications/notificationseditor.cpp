// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/notificationseditor.h"

#include "definitions/definitions.h"
#include "gui/notifications/singlenotificationeditor.h"
#include "qtlinq/qtlinq.h"

#include <QVBoxLayout>

NotificationsEditor::NotificationsEditor(QWidget* parent) : QWidget(parent), m_layout(new QVBoxLayout(this)) {
  m_ui.setupUi(this);
  setLayout(m_layout);
}

void NotificationsEditor::loadNotifications(const QList<Notification>& notifications) {
  auto all_events = Notification::allEvents();
  auto notif = qlinq::from(notifications);

  for (auto ev : all_events) {
    if (notif.any([ev](auto n) {
          return n.event() == ev;
        })) {
      auto* notif_editor = new SingleNotificationEditor(notif.first([ev](auto n) {
        return n.event() == ev;
      }),
                                                        this);

      connect(notif_editor,
              &SingleNotificationEditor::notificationChanged,
              this,
              &NotificationsEditor::someNotificationChanged);

      m_layout->addWidget(notif_editor);
    }
    else {
      auto* notif_editor = new SingleNotificationEditor(Notification(ev), this);

      connect(notif_editor,
              &SingleNotificationEditor::notificationChanged,
              this,
              &NotificationsEditor::someNotificationChanged);

      m_layout->addWidget(notif_editor);
    }
  }

  m_layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding));
}

QList<Notification> NotificationsEditor::allNotifications() const {
  auto lst = qlinq::from(findChildren<SingleNotificationEditor*>())
               .select([](const SingleNotificationEditor* ed) {
                 return ed->notification();
               })
               .toList();

  return lst;
}
