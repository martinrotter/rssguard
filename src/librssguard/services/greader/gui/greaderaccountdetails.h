// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERACCOUNTDETAILS_H
#define GREADERACCOUNTDETAILS_H

#include <QWidget>

#include "ui_greaderaccountdetails.h"

#include "services/greader/greaderserviceroot.h"

#include <QNetworkProxy>

class GreaderAccountDetails : public QWidget {
  Q_OBJECT

  friend class FormEditGreaderAccount;

  public:
    explicit GreaderAccountDetails(QWidget* parent = nullptr);

    GreaderServiceRoot::Service service() const;
    void setService(GreaderServiceRoot::Service service);

  private slots:
    void displayPassword(bool display);
    void performTest(const QNetworkProxy& custom_proxy);
    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();

  private:
    Ui::GreaderAccountDetails m_ui;
};

#endif // GREADERACCOUNTDETAILS_H
