// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef THREAD_H
#define THREAD_H

qlonglong getThreadID();

#if defined(Q_OS_LINUX)
// Values corresponding to nice values
enum Priority {
  LOWEST = 19,
  LOW    = 10,
  NORMAL = 0
};

void setThreadPriority(Priority);
#endif

#endif // THREAD_H
