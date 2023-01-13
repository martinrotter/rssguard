// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/autosaver.h"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>

#include "definitions/definitions.h"

AutoSaver::AutoSaver(QObject* parent, const QString& saving_slot, int max_wait_secs, int periodic_save_secs)
  : QObject(parent), m_savingSlot(saving_slot), m_maxWaitSecs(max_wait_secs), m_periodicSaveSecs(periodic_save_secs) {
  Q_ASSERT(parent);
}

AutoSaver::~AutoSaver() {
  if (m_timer.isActive()) {
    qWarningNN << LOGSEC_CORE << "AutoSaver still active when destroyed, changes not saved.";

    if (parent() != nullptr && parent()->metaObject() != nullptr) {
      qDebugNN << LOGSEC_CORE << "Should call saveIfNeccessary.";
    }
  }
}

void AutoSaver::changeOccurred() {
  if (!m_firstChange.isValid()) {
    m_firstChange.start();
  }

  if (m_firstChange.elapsed() > m_maxWaitSecs) {
    saveIfNeccessary();
  }
  else {
    m_timer.start(m_periodicSaveSecs, this);
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
    m_firstChange.invalidate();

    if (!QMetaObject::invokeMethod(parent(), qPrintable(m_savingSlot), Qt::ConnectionType::DirectConnection)) {
      qCriticalNN << LOGSEC_CORE << "AutoSaver error invoking saving slot on parent.";
    }
    else {
      qDebugNN << LOGSEC_CORE << "Saved data with auto-saver for"
               << QUOTE_W_SPACE(parent()->metaObject()->className()) "and method" << QUOTE_W_SPACE_DOT(m_savingSlot);
    }
  }
}
