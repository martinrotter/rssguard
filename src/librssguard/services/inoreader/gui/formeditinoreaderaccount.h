// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITINOREADERACCOUNT_H
#define FORMEDITINOREADERACCOUNT_H

#include <QDialog>

#include "ui_formeditinoreaderaccount.h"

#include "services/inoreader/network/inoreadernetworkfactory.h"

namespace Ui {
  class FormEditInoreaderAccount;
}

class InoreaderServiceRoot;

class FormEditInoreaderAccount : public QDialog {
  Q_OBJECT

  public:
    explicit FormEditInoreaderAccount(QWidget* parent = nullptr);
    virtual ~FormEditInoreaderAccount();

    InoreaderServiceRoot* execForCreate();

    void execForEdit(InoreaderServiceRoot* existing_root);

  private slots:
    void registerApi();
    void testSetup();
    void onClickedOk();
    void onClickedCancel();
    void checkOAuthValue(const QString& value);
    void checkUsername(const QString& username);
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();

    Ui::FormEditInoreaderAccount m_ui;
    OAuth2Service* m_oauth;
    InoreaderServiceRoot* m_editableRoot;
};

#endif // FORMEDITINOREADERACCOUNT_H
