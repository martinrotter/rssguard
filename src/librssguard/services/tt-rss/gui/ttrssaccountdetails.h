// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSACCOUNTDETAILS_H
#define TTRSSACCOUNTDETAILS_H

#include "ui_ttrssaccountdetails.h"

#include <QNetworkProxy>
#include <QWidget>

class TtRssServiceRoot;

class TtRssAccountDetails : public QWidget {
    Q_OBJECT

    friend class FormEditTtRssAccount;

  public:
    explicit TtRssAccountDetails(QWidget* parent = nullptr);

  private slots:
    void performTest(const QNetworkProxy& proxy);

    void onUsernameChanged();
    void onPasswordChanged();
    void onHttpUsernameChanged();
    void onHttpPasswordChanged();
    void onUrlChanged();

  private:
    Ui::TtRssAccountDetails m_ui;
};

#endif // TTRSSACCOUNTDETAILS_H
