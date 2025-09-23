// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef THREAD_H
#define THREAD_H

#include <QtGlobal>

RSSGUARD_DLLSPEC qlonglong getThreadID();

#if defined(Q_OS_LINUX)
// Values corresponding to nice values.
enum Priority {
  Lowest = 19,
  Low = 10,
  Normal = 0
};

void setThreadPriority(Priority);
#endif

#endif // THREAD_H
