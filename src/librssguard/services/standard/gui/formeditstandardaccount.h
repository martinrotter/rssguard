// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITSTANDARDACCOUNT_H
#define FORMEDITSTANDARDACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class FormEditStandardAccount : public FormAccountDetails {
  public:
    explicit FormEditStandardAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();
};

#endif // FORMEDITSTANDARDACCOUNT_H
