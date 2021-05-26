// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/notification.h"

#include "miscellaneous/application.h"

#include <QDir>
#include <QSound>

Notification::Notification() {}

Notification::Notification(Notification::Event event, const QString& sound_path) : m_event(event), m_soundPath(sound_path) {}

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

void Notification::playSound(Application* app) const {
  QSound::play(QDir::toNativeSeparators(app->replaceDataUserDataFolderPlaceholder(m_soundPath)));
}
