// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/notificationseditor.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/notifications/singlenotificationeditor.h"

#include <QVBoxLayout>

NotificationsEditor::NotificationsEditor(QWidget* parent) : QScrollArea(parent), m_layout(new QVBoxLayout(this)) {
  m_ui.setupUi(this);
  setLayout(m_layout);
}

void NotificationsEditor::loadNotifications(const QList<Notification>& notifications) {
  auto all_events = Notification::allEvents();
  auto notif = boolinq::from(notifications);

  for (auto ev : all_events) {
    if (notif.any([ev](auto n) {
      return n.event() == ev;
    })) {
      auto* notif_editor = new SingleNotificationEditor(notif.first([ev](auto n) {
        return n.event() == ev;
      }), this);

      notif_editor->setNotificationEnabled(true);

      m_layout->addWidget(notif_editor);
    }
    else {
      auto* notif_editor = new SingleNotificationEditor(Notification(ev, {}), this);

      notif_editor->setNotificationEnabled(false);
      m_layout->addWidget(notif_editor);

    }
  }

  m_layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}
