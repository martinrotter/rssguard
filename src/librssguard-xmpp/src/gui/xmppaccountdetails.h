// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef XMPPACCOUNTDETAILS_H
#define XMPPACCOUNTDETAILS_H

#include "ui_xmppaccountdetails.h"

#include <QNetworkProxy>
#include <QWidget>

class XmppServiceRoot;

class XmppAccountDetails : public QWidget {
    Q_OBJECT

    friend class FormEditXmppAccount;

  public:
    explicit XmppAccountDetails(QWidget* parent = nullptr);

    QStringList additionalServices() const;
    void setAdditionalServices(const QStringList& services);

  private slots:
    void performTest(const QNetworkProxy& proxy);

    void onUsernameEdited();
    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();

  private:
    Ui::XmppAccountDetails m_ui;
};

#endif // XMPPACCOUNTDETAILS_H
