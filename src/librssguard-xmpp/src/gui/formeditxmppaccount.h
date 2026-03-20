// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITACCOUNT_H
#define FORMEDITACCOUNT_H

#include <librssguard/services/abstract/gui/formaccountdetails.h>

class RootItem;
class XmppServiceRoot;
class XmppAccountDetails;

class FormEditXmppAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditXmppAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();
    virtual void rollBack();

  protected:
    virtual void loadAccountData();

  private slots:
    void performTest();

  private:
    XmppAccountDetails* m_details;
};

#endif // FORMEDITACCOUNT_H
