#ifndef FORMMAIN_H
#define FORMMAIN_H

#include <QMainWindow>

#include "ui_formmain.h"


class FormMain : public QMainWindow {
    Q_OBJECT
    
  public:
    explicit FormMain(QWidget *parent = 0);
    ~FormMain();

  public slots:
    void processExecutionMessage(const QString &message);
    
  private:
    Ui::FormMain *m_ui;
};

#endif // FORMMAIN_H
