#ifndef FORMWELCOME_H
#define FORMWELCOME_H

#include "ui_formwelcome.h"

#include <QDialog>


namespace Ui {
  class FormWelcome;
}

class FormWelcome : public QDialog {
    Q_OBJECT
    
  public:
    // Constructors and destructors.
    explicit FormWelcome(QWidget *parent = 0);
    virtual ~FormWelcome();
    
  private slots:
    // Opens given link in a default web browser.
    void openLink(const QString &link);

  private:
    Ui::FormWelcome *m_ui;
};

#endif // FORMWELCOME_H
