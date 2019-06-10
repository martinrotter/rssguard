// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITOWNCLOUDACCOUNT_H
#define FORMEDITOWNCLOUDACCOUNT_H

#include <QDialog>

#include "ui_formeditowncloudaccount.h"

namespace Ui {
  class FormEditAccount;
}

class OwnCloudServiceRoot;

class FormEditOwnCloudAccount : public QDialog {
  Q_OBJECT

  public:
    explicit FormEditOwnCloudAccount(QWidget* parent = 0);
    virtual ~FormEditOwnCloudAccount();

    OwnCloudServiceRoot* execForCreate();

    void execForEdit(OwnCloudServiceRoot* existing_root);

  private slots:
    void displayPassword(bool display);
    void performTest();
    void onClickedOk();
    void onClickedCancel();

    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();
    void checkOkButton();

  private:
    QScopedPointer<Ui::FormEditOwnCloudAccount> m_ui;
    OwnCloudServiceRoot* m_editableRoot;
    QPushButton* m_btnOk;
};

#endif // FORMEDITOWNCLOUDACCOUNT_H
