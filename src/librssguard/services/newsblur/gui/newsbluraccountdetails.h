// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEWSBLURACCOUNTDETAILS_H
#define NEWSBLURACCOUNTDETAILS_H

#include <QWidget>

#include "ui_newsbluraccountdetails.h"

#include "services/newsblur/newsblurserviceroot.h"

#include <QNetworkProxy>

class NewsBlurAccountDetails : public QWidget {
  Q_OBJECT

  friend class FormEditNewsBlurAccount;

  public:
    explicit NewsBlurAccountDetails(QWidget* parent = nullptr);

  private slots:
    void performTest(const QNetworkProxy& custom_proxy);
    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();

  private:
    Ui::NewsBlurAccountDetails m_ui;
    QNetworkProxy m_lastProxy;
};

#endif // NEWSBLURACCOUNTDETAILS_H
