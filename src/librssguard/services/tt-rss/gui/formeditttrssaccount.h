// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITACCOUNT_H
#define FORMEDITACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class RootItem;
class TtRssServiceRoot;
class TtRssAccountDetails;

class FormEditTtRssAccount : public FormAccountDetails {
  Q_OBJECT

  public:
    explicit FormEditTtRssAccount(QWidget* parent = nullptr);

    TtRssServiceRoot* addEditAccount(TtRssServiceRoot* account_to_edit = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void setEditableAccount(ServiceRoot* editable_account);

  private:
    TtRssServiceRoot* ttRssAccount() const;

  private:
    TtRssAccountDetails* m_details;
};

#endif // FORMEDITACCOUNT_H
