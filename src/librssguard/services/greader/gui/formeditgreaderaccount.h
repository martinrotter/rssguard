// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITGREADERACCOUNT_H
#define FORMEDITGREADERACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class GreaderAccountDetails;
class GreaderServiceRoot;

class FormEditGreaderAccount : public FormAccountDetails {
  Q_OBJECT

  public:
    explicit FormEditGreaderAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void setEditableAccount(ServiceRoot* editable_account);

  private slots:
    void performTest();

  private:
    GreaderAccountDetails* m_details;
};

#endif // FORMEDITGREADERACCOUNT_H
