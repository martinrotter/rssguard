// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITINOREADERACCOUNT_H
#define FORMEDITINOREADERACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

#include "services/gmail/network/gmailnetworkfactory.h"

class GmailServiceRoot;
class GmailAccountDetails;

class FormEditGmailAccount : public FormAccountDetails {
  Q_OBJECT

  public:
    explicit FormEditGmailAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void setEditableAccount(ServiceRoot* editable_account);

  private:
    GmailAccountDetails* m_details;
};

#endif // FORMEDITINOREADERACCOUNT_H
