// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OWNCLOUDACCOUNTDETAILS_H
#define OWNCLOUDACCOUNTDETAILS_H

#include "ui_owncloudaccountdetails.h"

#include <QNetworkProxy>
#include <QWidget>

class OwnCloudAccountDetails : public QWidget {
    Q_OBJECT

    friend class FormEditOwnCloudAccount;

  public:
    explicit OwnCloudAccountDetails(QWidget* parent = nullptr);

  private slots:
    void performTest(const QNetworkProxy& custom_proxy);
    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();

  private:
    Ui::OwnCloudAccountDetails m_ui;
};

#endif // OWNCLOUDACCOUNTDETAILS_H
