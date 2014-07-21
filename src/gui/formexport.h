#ifndef FORMEXPORT_H
#define FORMEXPORT_H

#include <QDialog>

namespace Ui {
class FormExport;
}

class FormExport : public QDialog
{
    Q_OBJECT

  public:
    explicit FormExport(QWidget *parent = 0);
    ~FormExport();

  private:
    Ui::FormExport *ui;
};

#endif // FORMEXPORT_H
