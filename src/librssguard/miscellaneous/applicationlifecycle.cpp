// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/applicationlifecycle.h"

#include "database/databasefactory.h"
#include "definitions/globals.h"
#include "exceptions/applicationexception.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "network-web/cookiejar.h"
#include "network-web/webfactory.h"

#include <QSessionManager>

ApplicationLifecycle::ApplicationLifecycle(Application* application)
  : QObject(), m_application(application), m_quitLogicDone(false) {}

void ApplicationLifecycle::onCommitData(QSessionManager& manager) {
  qDebugNN << LOGSEC_CORE << "OS asked application to commit its data.";

  onAboutToQuit();

  manager.setRestartHint(QSessionManager::RestartHint::RestartNever);
  manager.release();
}

void ApplicationLifecycle::onSaveState(QSessionManager& manager) {
  qDebugNN << LOGSEC_CORE << "OS asked application to save its state.";

  manager.setRestartHint(QSessionManager::RestartHint::RestartNever);
  manager.release();
}

void ApplicationLifecycle::onAboutToQuit() {
  if (m_quitLogicDone) {
    qWarningNN << LOGSEC_CORE << "On-close logic is already done.";
    return;
  }

  m_quitLogicDone = true;

  // Obtain the write lock before trying to quit so critical actions finish first.
  const bool locked_safely = m_application->feedUpdateLock()->tryLock(4 * CLOSE_LOCK_TIMEOUT);

  qDebugNN << LOGSEC_CORE << "Cleaning up resources and saving application state.";

  if (locked_safely) {
    qDebugNN << LOGSEC_CORE << "Close lock was obtained safely.";
    m_application->feedUpdateLock()->unlock();
  }
  else {
    qWarningNN << LOGSEC_CORE << "Close lock timed-out.";
  }

  m_application->feedReader()->quit();

  try {
    m_application->database()->driver()->saveDatabase();
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Error when saving DB:" << QUOTE_W_SPACE_DOT(ex.message());
  }

  if (m_application->mainForm() != nullptr) {
    m_application->mainForm()->saveSize();
  }

  m_application->web()->cookieJar()->saveCookies();
  m_application->settings()->sync();
}
