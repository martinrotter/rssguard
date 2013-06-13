#ifndef DEBUGGING_H
#define DEBUGGING_H

#include <QtGlobal>


class Debugging {
  public:
    // Specifies format of output console messages.
    // Macros:
    //    QT_NO_DEBUG_OUTPUT - disables debug outputs completely!!!
    static void debugHandler(QtMsgType type,
                             const QMessageLogContext &placement,
                             const QString &message);
};

#endif // DEBUGGING_H
