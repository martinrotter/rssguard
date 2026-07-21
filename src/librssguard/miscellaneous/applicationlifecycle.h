// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APPLICATIONLIFECYCLE_H
#define APPLICATIONLIFECYCLE_H

#include <QObject>

class Application;
class QSessionManager;

class ApplicationLifecycle : public QObject {
  public:
    explicit ApplicationLifecycle(Application* application);

    void onCommitData(QSessionManager& manager);
    void onSaveState(QSessionManager& manager);
    void onAboutToQuit();

  private:
    Application* m_application;
    bool m_quitLogicDone;
};

#endif // APPLICATIONLIFECYCLE_H
