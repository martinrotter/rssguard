// For license of this file, see <project-root-folder>/LICENSE.md.

#include "definitions/definitions.h"
#include "miscellaneous/thread.h"

#include <QThread>

#if defined(Q_OS_LINUX)
#include <sched.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

// Returns the thread ID of the caller
qlonglong getThreadID() {
  #if defined(Q_OS_LINUX)
  return qlonglong(gettid());
  #else
  return qlonglong(QThread::currentThreadId());
  #endif
}

#if defined(Q_OS_LINUX)
// On Linux QThread priorities do nothing with the default scheduler SCHED_OTHER
// Set the nice value manually in this case until Qt supports nice values
void setThreadPriority(Priority prio) {
  int current_policy = sched_getscheduler(0);
  if (current_policy != -1) {
    // If the current scheduling policy is neither of these the QThread priority should be working
    if (current_policy != SCHED_BATCH && current_policy != SCHED_OTHER) {
      return;
    }

    // Set the scheduler to SCHED_BATCH if needed, indicating that this process is non-interactive
    if (current_policy == SCHED_OTHER) {
      struct sched_param p = {0};
      if (sched_setscheduler(0, SCHED_BATCH, &p) != 0) {
        qDebugNN << "Setting the scheduler to SCHED_BATCH for thread"
                 << QUOTE_W_SPACE(getThreadID())
                 << "failed with error" << QUOTE_W_SPACE_DOT(errno);
        // We can still try to set the nice value
      }
    }

    errno = 0; // Clear errno since -1 is a legitimate return value
    int current_priority = getpriority(PRIO_PROCESS, 0);
    if (errno != 0) {
      qDebugNN << "Getting the priority for thread"
               << QUOTE_W_SPACE(getThreadID())
               << "failed with error" << QUOTE_W_SPACE_DOT(errno);
    } else {
      if (current_priority != prio) {
        setpriority(PRIO_PROCESS, 0, prio);
        if (errno != 0) {
          qDebugNN << "Setting the priority for thread"
                   << QUOTE_W_SPACE(getThreadID())
                   << "failed with error" << QUOTE_W_SPACE_DOT(errno);
        }
      }
    }
  } else {
    qDebugNN << "Getting the priority for thread"
             << QUOTE_W_SPACE(getThreadID())
             << "failed with error" << QUOTE_W_SPACE_DOT(errno);
  }
}
#endif
