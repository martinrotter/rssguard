#ifndef DEBUGGING_H
#define DEBUGGING_H

#include <QtGlobal>


class Debugging {
  public:
    // Specifies format of output console messages.
    // NOTE: QT_NO_DEBUG_OUTPUT - disables debug outputs completely!!!
#if QT_VERSION >= 0x050000
    static void debugHandler(QtMsgType type,
                             const QMessageLogContext &placement,
                             const QString &message);
#else
    static void debugHandler(QtMsgType type,
                             const char *message);
#endif

  private:
    // Constructor.
    explicit Debugging();
};

#endif // DEBUGGING_H
