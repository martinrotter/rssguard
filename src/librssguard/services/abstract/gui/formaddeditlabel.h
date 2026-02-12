// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMADDEDITLABEL_H
#define FORMADDEDITLABEL_H

#include "ui_formaddeditlabel.h"

#include <QDialog>

namespace Ui {
  class FormAddEditLabel;
}

class Label;
class ServiceRoot;

class RSSGUARD_DLLSPEC FormAddEditLabel : public QDialog {
    Q_OBJECT

  public:
    explicit FormAddEditLabel(QWidget* parent = nullptr);

  public slots:
    Label* execForAdd(ServiceRoot* account);
    bool execForEdit(Label* lbl);

  private:
    Ui::FormAddEditLabel m_ui;
    Label* m_editableLabel;
};

#endif // FORMADDEDITLABEL_H
