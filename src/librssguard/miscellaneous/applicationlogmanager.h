// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APPLICATIONLOGMANAGER_H
#define APPLICATIONLOGMANAGER_H

#include <QObject>
#include <QPointer>

class Application;
class FormLog;
class QMessageLogContext;

class ApplicationLogManager : public QObject {
    Q_OBJECT

  public:
    explicit ApplicationLogManager(Application* application);
    ~ApplicationLogManager() override;

    void updateCliDebugStatus();
    void initializeFileBasedLogging();
    void displayLog();
    void shutdown();

    static void performLogging(QtMsgType type, const QMessageLogContext& context, const QString& message);

  signals:
    void sendLogToDialog(QString message);

  private:
    void displayLogMessageInDialog(const QString& message);

  private:
    static ApplicationLogManager* s_instance;

    Application* m_application;
    QPointer<FormLog> m_logForm;
};

#endif // APPLICATIONLOGMANAGER_H
