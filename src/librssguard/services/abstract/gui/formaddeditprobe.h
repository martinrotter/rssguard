// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMADDEDITPROBE_H
#define FORMADDEDITPROBE_H

#include <QDialog>

#include "ui_formaddeditprobe.h"

namespace Ui {
  class FormAddEditProbe;
}

class Search;

class FormAddEditProbe : public QDialog {
    Q_OBJECT

  public:
    explicit FormAddEditProbe(QWidget* parent = nullptr);

  public slots:
    Search* execForAdd();
    bool execForEdit(Search* prb);

  private:
    Ui::FormAddEditProbe m_ui;
    Search* m_editableProbe;
};

#endif // FORMADDEDITPROBE_H
