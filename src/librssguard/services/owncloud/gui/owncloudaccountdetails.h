// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OWNCLOUDACCOUNTDETAILS_H
#define OWNCLOUDACCOUNTDETAILS_H

#include <QWidget>

#include "ui_owncloudaccountdetails.h"

class OwnCloudAccountDetails : public QWidget {
  Q_OBJECT

  friend class FormEditOwnCloudAccount;

  public:
    explicit OwnCloudAccountDetails(QWidget* parent = nullptr);

  private slots:
    void displayPassword(bool display);
    void performTest();
    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();

  private:
    Ui::OwnCloudAccountDetails m_ui;
};

#endif // OWNCLOUDACCOUNTDETAILS_H
