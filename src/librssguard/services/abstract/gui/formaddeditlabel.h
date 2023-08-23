// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMADDEDITLABEL_H
#define FORMADDEDITLABEL_H

#include <QDialog>

#include "ui_formaddeditlabel.h"

namespace Ui {
  class FormAddEditLabel;
}

class Label;

class FormAddEditLabel : public QDialog {
  Q_OBJECT

  public:
    explicit FormAddEditLabel(QWidget* parent = nullptr);

  public slots:
    Label* execForAdd();
    bool execForEdit(Label* lbl);

  private:
    Ui::FormAddEditLabel m_ui;
    Label* m_editableLabel;
};

#endif // FORMADDEDITLABEL_H
