// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QApplication>

class QLocalServer;

class SingleApplication : public QApplication {
    Q_OBJECT

  public:
    explicit SingleApplication(const QString& id, int& argc, char** argv);
    virtual ~SingleApplication();

    void finish();

    bool isOtherInstanceRunning(const QString& message = {});
    bool sendMessage(const QString& message);

  signals:
    void messageReceived(const QString& message);

  private slots:
    void processMessageFromOtherInstance();

  private:
    QString m_id;
    QLocalServer* m_server;
};

#endif // SINGLEAPPLICATION_H
