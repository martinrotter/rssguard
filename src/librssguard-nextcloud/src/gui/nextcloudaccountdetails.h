// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEXTCLOUDACCOUNTDETAILS_H
#define NEXTCLOUDACCOUNTDETAILS_H

#include "ui_nextcloudaccountdetails.h"

#include <QNetworkProxy>
#include <QWidget>

class NextcloudAccountDetails : public QWidget {
    Q_OBJECT

    friend class FormEditNextcloudAccount;

  public:
    explicit NextcloudAccountDetails(QWidget* parent = nullptr);

  private slots:
    void performTest(const QNetworkProxy& custom_proxy);
    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();

  private:
    Ui::NextcloudAccountDetails m_ui;
};

#endif // NEXTCLOUDACCOUNTDETAILS_H
