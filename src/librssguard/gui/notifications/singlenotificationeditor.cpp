// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/singlenotificationeditor.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

SingleNotificationEditor::SingleNotificationEditor(const Notification& notification, QWidget* parent)
  : QWidget(parent), m_notificationEvent(Notification::Event::NoEvent) {
  m_ui.setupUi(this);

  m_ui.m_btnBrowseSound->setIcon(qApp->icons()->fromTheme(QSL("document-open")));
  m_ui.m_btnPlaySound->setIcon(qApp->icons()->fromTheme(QSL("media-playback-start")));

  loadNotification(notification);

  connect(m_ui.m_btnPlaySound, &QPushButton::clicked, this, &SingleNotificationEditor::playSound);
  connect(m_ui.m_txtSound, &QLineEdit::textChanged, this, &SingleNotificationEditor::notificationChanged);
  connect(m_ui.m_cbBalloon, &QCheckBox::toggled, this, &SingleNotificationEditor::notificationChanged);
}

Notification SingleNotificationEditor::notification() const {
  return Notification(m_notificationEvent, m_ui.m_cbBalloon->isChecked(), m_ui.m_txtSound->text());
}

void SingleNotificationEditor::playSound() {
  Notification({}, {}, m_ui.m_txtSound->text()).playSound(qApp);
}

void SingleNotificationEditor::loadNotification(const Notification& notification) {
  m_ui.m_txtSound->setText(notification.soundPath());
  m_ui.m_cbBalloon->setChecked(notification.balloonEnabled());
  m_ui.m_gbNotification->setTitle(Notification::nameForEvent(notification.event()));

  m_notificationEvent = notification.event();
}
