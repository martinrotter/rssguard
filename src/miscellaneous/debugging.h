// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DEBUGGING_H
#define DEBUGGING_H

#include <QtGlobal>

class Debugging {
  public:

    // Specifies format of output console messages.
    // NOTE: QT_NO_DEBUG_OUTPUT - disables debug outputs completely!!!
    static void debugHandler(QtMsgType type, const QMessageLogContext& placement, const QString& message);
    static void performLog(const char* message, QtMsgType type, const char* file = 0, const char* function = 0, int line = -1);
    static const char* typeToString(QtMsgType type);

  private:

    // Constructor.
    explicit Debugging();
};

#endif // DEBUGGING_H
