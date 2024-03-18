// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITSTANDARDACCOUNT_H
#define FORMEDITSTANDARDACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class StandardAccountDetails;

class FormEditStandardAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditStandardAccount(QWidget* parent = nullptr);
    virtual ~FormEditStandardAccount();

  protected:
    virtual void loadAccountData();

  protected slots:
    virtual void apply();

  private:
    StandardAccountDetails* m_standardDetails;
};

#endif // FORMEDITSTANDARDACCOUNT_H
