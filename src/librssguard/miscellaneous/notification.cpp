// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/notification.h"

Notification::Notification() {}

Notification::Event Notification::event() const {
  return m_event;
}

void Notification::setEvent(const Event& event) {
  m_event = event;
}

QString Notification::soundPath() const {
  return m_soundPath;
}

void Notification::setSoundPath(const QString& sound_path) {
  m_soundPath = sound_path;
}
