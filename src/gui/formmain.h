#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QMainWindow>

#include "ui_formmain.h"


class FormMain : public QMainWindow {
    Q_OBJECT
    
  public:
    explicit FormMain(QWidget *parent = 0);
    ~FormMain();

    static FormMain *getInstance();

  protected:
    void createConnections();

#if defined(Q_OS_LINUX)
    bool event(QEvent *event);

    // Sets up proper icons for this widget.
    void setupIcons();
#endif

  public slots:
    void processExecutionMessage(const QString &message);
    void quit();

  protected slots:
    void cleanupResources();
    void showSettings();
    
  private:
    Ui::FormMain *m_ui;

    static FormMain *m_this;
};

#endif // FORMMAIN_H
