// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERACCOUNTDETAILS_H
#define GREADERACCOUNTDETAILS_H

#include "src/greaderserviceroot.h"

#include "ui_greaderaccountdetails.h"

#include <QNetworkProxy>
#include <QWidget>

class OAuth2Service;

class GreaderAccountDetails : public QWidget {
    Q_OBJECT

    friend class FormEditGreaderAccount;

  public:
    explicit GreaderAccountDetails(QWidget* parent = nullptr);

    GreaderServiceRoot::Service service() const;
    void setService(GreaderServiceRoot::Service service);

  private slots:
    void performTest(const QNetworkProxy& custom_proxy);
    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();
    void selectedServiceChanged();
    void checkOAuthValue(const QString& value);
    void registerApi();
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();

  private:
    Ui::GreaderAccountDetails m_ui;

    // Testing OAuth service. This object is not ever copied
    // to new living account instance but maybe be copied from it,
    // instead only its properties like tokens are copied.
    OAuth2Service* m_oauth;
    QNetworkProxy m_lastProxy;
};

#endif // GREADERACCOUNTDETAILS_H
