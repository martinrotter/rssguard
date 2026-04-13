// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/autosaver.h"

#include "definitions/definitions.h"

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>

AutoSaver::AutoSaver(QObject* parent, const QString& saving_slot, int max_wait_secs, int periodic_save_secs)
  : QObject(parent), m_maxWaitMsecs(max_wait_secs * 1000), m_periodicSaveMsecs(periodic_save_secs * 1000),
    m_savingSlot(saving_slot) {
  Q_ASSERT(parent);

  connect(&m_timer, &QTimer::timeout, this, &AutoSaver::saveIfNeccessary);
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

  if (m_firstChange.elapsed() > m_maxWaitMsecs) {
    saveIfNeccessary();
  }
  else {
    QMetaObject::invokeMethod(&m_timer, "start", Qt::ConnectionType::AutoConnection, Q_ARG(int, m_periodicSaveMsecs));
  }
}

void AutoSaver::saveIfNeccessary() {
  if (m_timer.isActive()) {
    QMetaObject::invokeMethod(&m_timer, "stop", Qt::ConnectionType::AutoConnection);
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
