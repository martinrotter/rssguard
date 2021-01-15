// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITINOREADERACCOUNT_H
#define FORMEDITINOREADERACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class InoreaderServiceRoot;
class InoreaderAccountDetails;

class FormEditInoreaderAccount : public FormAccountDetails {
  Q_OBJECT

  public:
    explicit FormEditInoreaderAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void setEditableAccount(ServiceRoot* editable_account);

  private:
    InoreaderAccountDetails* m_details;
};

#endif // FORMEDITINOREADERACCOUNT_H
