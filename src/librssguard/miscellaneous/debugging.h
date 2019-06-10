// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DEBUGGING_H
#define DEBUGGING_H

#include <QtGlobal>

#include <QFile>
#include <QString>

class Debugging {
  public:
    explicit Debugging();

    // Specifies format of output console messages.
    // NOTE: QT_NO_DEBUG_OUTPUT - disables debug outputs completely!!!
    static void debugHandler(QtMsgType type, const QMessageLogContext& placement, const QString& message);
    static void performLog(const char* message, QtMsgType type, const char* file = nullptr, const char* function = nullptr, int line = -1);
    static const char* typeToString(QtMsgType type);

    // Returns pointer to global silent network manager
    static Debugging* instance();

    void setTargetFile(const QString& targetFile);
    QString targetFile() const;

    QFile* targetFileHandle();

  private:
    QString m_targetFile;
    QFile* m_targetFileHandle{};
};

#endif // DEBUGGING_H
