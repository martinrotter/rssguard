// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITINOREADERACCOUNT_H
#define FORMEDITINOREADERACCOUNT_H

#include "src/gmailnetworkfactory.h"

#include <librssguard/services/abstract/gui/formaccountdetails.h>

class GmailServiceRoot;
class GmailAccountDetails;

class FormEditGmailAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditGmailAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void loadAccountData();

  private:
    GmailAccountDetails* m_details;
};

#endif // FORMEDITINOREADERACCOUNT_H
