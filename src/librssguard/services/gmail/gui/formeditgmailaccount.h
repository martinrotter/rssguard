// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITINOREADERACCOUNT_H
#define FORMEDITINOREADERACCOUNT_H

#include <QDialog>

#include "ui_formeditgmailaccount.h"

#include "services/gmail/network/gmailnetworkfactory.h"

namespace Ui {
  class FormEditGmailAccount;
}

class GmailServiceRoot;

class FormEditGmailAccount : public QDialog {
  Q_OBJECT

  public:
    explicit FormEditGmailAccount(QWidget* parent = nullptr);
    virtual ~FormEditGmailAccount();

    GmailServiceRoot* execForCreate();

    void execForEdit(GmailServiceRoot* existing_root);

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

    Ui::FormEditGmailAccount m_ui;
    OAuth2Service* m_oauth;
    GmailServiceRoot* m_editableRoot;
};

#endif // FORMEDITINOREADERACCOUNT_H
