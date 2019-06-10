// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/autosaver.h"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>

#define AUTOSAVE_IN  (1000 * 3)  // seconds
#define MAXWAIT      (1000 * 15) // seconds

AutoSaver::AutoSaver(QObject* parent) : QObject(parent) {
  Q_ASSERT(parent);
}

AutoSaver::~AutoSaver() {
  if (m_timer.isActive()) {
    qWarning("AutoSaver: still active when destroyed, changes not saved.");

    if (parent() != nullptr && parent()->metaObject() != nullptr) {
      qWarning("Should call saveIfNeccessary.");
    }
  }
}

void AutoSaver::changeOccurred() {
  if (m_firstChange.isNull()) {
    m_firstChange.start();
  }

  if (m_firstChange.elapsed() > MAXWAIT) {
    saveIfNeccessary();
  }
  else {
    m_timer.start(AUTOSAVE_IN, this);
  }
}

void AutoSaver::timerEvent(QTimerEvent* event) {
  if (event->timerId() == m_timer.timerId()) {
    saveIfNeccessary();
  }
  else {
    QObject::timerEvent(event);
  }
}

void AutoSaver::saveIfNeccessary() {
  if (m_timer.isActive()) {
    m_timer.stop();
    m_firstChange = QTime();

    if (!QMetaObject::invokeMethod(parent(), "save", Qt::DirectConnection)) {
      qWarning("AutoSaver: error invoking slot save() on parent.");
    }
  }
}
