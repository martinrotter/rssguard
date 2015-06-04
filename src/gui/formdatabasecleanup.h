#ifndef FORMDATABASECLEANUP_H
#define FORMDATABASECLEANUP_H

#include <QDialog>

#include "ui_formdatabasecleanup.h"


namespace Ui {
  class FormDatabaseCleanup;
}

class FormDatabaseCleanup : public QDialog {
    Q_OBJECT

  public:
    explicit FormDatabaseCleanup(QWidget *parent = 0);
    virtual ~FormDatabaseCleanup();

  private slots:
    void updateDaysSuffix(int number);

  private:
    Ui::FormDatabaseCleanup *m_ui;
};

#endif // FORMDATABASECLEANUP_H
