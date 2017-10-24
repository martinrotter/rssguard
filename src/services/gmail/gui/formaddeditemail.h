// For license of this file, see <object-root-folder>/LICENSE.md.

#ifndef FORMADDEDITEMAIL_H
#define FORMADDEDITEMAIL_H

#include <QDialog>

#include "ui_formaddeditemail.h"

namespace Ui {
  class FormAddEditEmail;
}

class GmailServiceRoot;

class FormAddEditEmail : public QDialog {
  Q_OBJECT

  public:
    explicit FormAddEditEmail(GmailServiceRoot* root, QWidget* parent = nullptr);

  public slots:
    void execForAdd();

  private:
    GmailServiceRoot* m_root;

    Ui::FormAddEditEmail m_ui;
};

#endif // FORMADDEDITEMAIL_H
