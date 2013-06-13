#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QMainWindow>

#include "ui_formmain.h"


class FormMain : public QMainWindow {
    Q_OBJECT
    
  public:
    explicit FormMain(QWidget *parent = 0);
    ~FormMain();

  protected:
    void createConnections();

  public slots:
    void processExecutionMessage(const QString &message);
    void quit();

  protected slots:
    void cleanupResources();
    
  private:
    Ui::FormMain *m_ui;
};

#endif // FORMMAIN_H
