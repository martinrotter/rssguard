// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMADDEDITPROBE_H
#define FORMADDEDITPROBE_H

#include "ui_formaddeditprobe.h"

#include <QDialog>

namespace Ui {
  class FormAddEditProbe;
}

class Search;
class ServiceRoot;

class RSSGUARD_DLLSPEC FormAddEditProbe : public QDialog {
    Q_OBJECT

  public:
    explicit FormAddEditProbe(QWidget* parent = nullptr);

  public slots:
    Search* execForAdd(ServiceRoot* account);
    bool execForEdit(Search* prb);

  private:
    Ui::FormAddEditProbe m_ui;
    Search* m_editableProbe;
};

#endif // FORMADDEDITPROBE_H
