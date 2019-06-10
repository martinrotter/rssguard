// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITACCOUNT_H
#define FORMEDITACCOUNT_H

#include <QDialog>

#include "ui_formeditttrssaccount.h"

namespace Ui {
  class FormEditTtRssAccount;
}

class TtRssServiceRoot;

class FormEditTtRssAccount : public QDialog {
  Q_OBJECT

  public:
    explicit FormEditTtRssAccount(QWidget* parent = 0);
    virtual ~FormEditTtRssAccount();

    TtRssServiceRoot* execForCreate();

    void execForEdit(TtRssServiceRoot* existing_root);

  private slots:
    void displayPassword(bool display);
    void displayHttpPassword(bool display);
    void performTest();
    void onClickedOk();
    void onClickedCancel();

    void onUsernameChanged();
    void onPasswordChanged();
    void onHttpUsernameChanged();
    void onHttpPasswordChanged();
    void onUrlChanged();
    void checkOkButton();

  private:
    QScopedPointer<Ui::FormEditTtRssAccount> m_ui;
    TtRssServiceRoot* m_editableRoot;
    QPushButton* m_btnOk;
};

#endif // FORMEDITACCOUNT_H
